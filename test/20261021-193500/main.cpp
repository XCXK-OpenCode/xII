// Screen::RGBA8888 的拷贝/移动/赋值语义测试。
//
// 背景：RGBA8888::operator= 内部会 _resize()（delete[] 旧缓冲 + new[] 新缓冲）。
// 这要求：
//   1. _data / _size 在默认构造时被初始化（否则 delete[] 野指针 → abort）
//   2. 自赋值被排除（否则 _resize 后 copy 的是未初始化数据）
//   3. 析构函数释放缓冲（否则每次都泄漏）
// 本测试用计数版 operator new[]/delete[] 直接量化内存，不依赖 ASan。

#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <new>
#include <string>
#include <utility>

#include "Camera/Standard.h"
#include "Item/Base.h"
#include "Item/Image.h"
#include "Item/Shape/Rectangle.h"
#include "Screen/Screen.h"
#include "Space/Offset.h"
#include "Space/OffsetPoint.h"
#include "Space/Point.h"
#include "Time/OffsetPoint.h"
#include "Time/Point.h"

// ---------------------------------------------------------------- 内存计数
// RGBA8888 的像素缓冲全部来自 new Pixel[...]，所以统计 new[] / delete[] 的
// 净差就能判断是否泄漏。这里统一走 malloc/free，保证配对。
namespace {
    std::atomic<long> g_array_allocs{0};
    std::atomic<long> g_array_frees{0};
} // namespace

void* operator new[](std::size_t size) {
    ++g_array_allocs;
    if (void* p = std::malloc(size ? size : 1))
        return p;
    throw std::bad_alloc();
}

void operator delete[](void* ptr) noexcept {
    if (ptr != nullptr) {
        ++g_array_frees;
        std::free(ptr);
    }
}

void operator delete[](void* ptr, std::size_t) noexcept {
    operator delete[](ptr);
}

namespace {
    using xII::Screen::PoSize;
    using xII::Screen::RGBA8888;
    using xII::Screen::Screen;

    using Val = xII::Base::Quantity::Base_Value_Type;

    constexpr int SCREEN_W = 320;
    constexpr int SCREEN_H = 180;

    int g_failures = 0;

    void report(const char* name, const bool ok, const std::string& detail) {
        std::printf("[check] %-32s %s   %s\n", name, ok ? "PASS" : "FAIL", detail.c_str());
        if (!ok)
            ++g_failures;
    }

    bool samePixel(const RGBA8888::Pixel& a, const RGBA8888::Pixel& b) {
        return a.A == b.A && a.R == b.R && a.G == b.G && a.B == b.B;
    }

    std::string sizeText(const RGBA8888& s) {
        return std::to_string(s.getSize().x) + "x" + std::to_string(s.getSize().y);
    }

    std::string pxText(const RGBA8888::Pixel& p) {
        return "(" + std::to_string(p.A) + "," + std::to_string(p.R) + "," +
               std::to_string(p.G) + "," + std::to_string(p.B) + ")";
    }

    // 每个像素都不同，便于发现“拷错/没拷”的情况。
    Screen makePattern(const int w, const int h, const uint8_t base = 0) {
        Screen img(PoSize{w, h});
        for (int y = 0; y < h; ++y)
            for (int x = 0; x < w; ++x)
                img.at(PoSize{x, y}) = RGBA8888::Pixel{
                    static_cast<uint8_t>(255),
                    static_cast<uint8_t>(base + x * 7),
                    static_cast<uint8_t>(base + y * 11),
                    static_cast<uint8_t>(base + x + y)};
        return img;
    }

    long liveArrays() {
        return g_array_allocs.load() - g_array_frees.load();
    }
} // namespace

int main() {
    std::printf("Test 20261021-193500: Screen::RGBA8888 copy/move/assign semantics\n");
    std::printf("sizeof(RGBA8888)=%zu bytes\n\n", sizeof(RGBA8888));

    const long baseline = liveArrays();

    // ---------------------------------------------------------------- 1
    // 默认构造后赋值：曾经因为 _data 未初始化而在 _resize 里 delete[] 野指针崩溃。
    {
        RGBA8888 dest;                       // 默认构造（_data 应为 nullptr）
        RGBA8888 src = makePattern(4, 4, 3);

        const long before = liveArrays();
        dest = src;
        const long after = liveArrays();

        bool same = true;
        for (int y = 0; y < 4 && same; ++y)
            for (int x = 0; x < 4 && same; ++x)
                same = samePixel(dest.at(PoSize{x, y}), src.at(PoSize{x, y}));

        // 期望：目标多了一块自己的缓冲（+1），源不变
        const bool ok = dest.getSize().x == 4 && dest.getSize().y == 4 && same &&
                        (after - before) == 1;
        report("default-construct then assign", ok,
               "dest=" + sizeText(dest) + " (0,0)=" + pxText(dest.at(PoSize{0, 0})) +
                   " allocs+=" + std::to_string(after - before) + " (expect +1)");
    }

    // ---------------------------------------------------------------- 2
    // 自赋值：曾经把数据洗成未初始化的垃圾（实测 190,190,190,190）。
    {
        RGBA8888 a = makePattern(4, 4, 5);
        const auto before = a.at(PoSize{0, 0});
        const auto before_corner = a.at(PoSize{3, 3});
        const long before_live = liveArrays();

        a = a;

        const long after_live = liveArrays();
        const bool ok = samePixel(a.at(PoSize{0, 0}), before) &&
                        samePixel(a.at(PoSize{3, 3}), before_corner) &&
                        a.getSize().x == 4 &&
                        after_live == before_live; // 不应重新分配
        report("self assignment", ok,
               "(0,0)=" + pxText(a.at(PoSize{0, 0})) + " expect " + pxText(before) +
                   " live arrays " + std::to_string(before_live) + "->" + std::to_string(after_live));
    }

    // ---------------------------------------------------------------- 3
    // 尺寸不同的赋值：operator= 必须先 resize，否则数据会被截断/越界。
    {
        RGBA8888 small = makePattern(2, 2, 1);
        RGBA8888 big = makePattern(8, 6, 40);

        small = big;
        const bool grow_ok = small.getSize().x == 8 && small.getSize().y == 6 &&
                             samePixel(small.at(PoSize{7, 5}), big.at(PoSize{7, 5}));

        RGBA8888 back;
        back = small;
        RGBA8888 tiny = makePattern(3, 3, 90);
        back = tiny;
        const bool shrink_ok = back.getSize().x == 3 && back.getSize().y == 3 &&
                               samePixel(back.at(PoSize{2, 2}), tiny.at(PoSize{2, 2}));

        report("assign with size change", grow_ok && shrink_ok,
               "2x2<-8x6 -> " + sizeText(small) + " | 8x6<-3x3 -> " + sizeText(back));
    }

    // ---------------------------------------------------------------- 4
    // 拷贝构造：深拷贝，双方互不影响。
    {
        RGBA8888 src = makePattern(5, 5, 10);
        RGBA8888 copy(src);

        const auto src_px = src.at(PoSize{1, 1});
        const auto copy_px = copy.at(PoSize{1, 1});

        src.at(PoSize{1, 1}) = RGBA8888::Pixel{255, 200, 201, 202};
        const bool src_edit_isolated = samePixel(copy.at(PoSize{1, 1}), copy_px);

        copy.at(PoSize{2, 2}) = RGBA8888::Pixel{255, 210, 211, 212};
        const bool copy_edit_isolated = samePixel(src.at(PoSize{2, 2}),
                                                  makePattern(5, 5, 10).at(PoSize{2, 2}));

        const bool ok = copy.getSize().x == 5 && samePixel(src_px, copy_px) &&
                        src_edit_isolated && copy_edit_isolated;
        report("copy constructor deep copy", ok,
               "size=" + sizeText(copy) + " src-edit isolated=" +
                   std::to_string(src_edit_isolated ? 1 : 0) + " copy-edit isolated=" +
                   std::to_string(copy_edit_isolated ? 1 : 0));
    }

    // ---------------------------------------------------------------- 5
    // 移动构造：接管缓冲，源被清空，且不会重复释放。
    {
        RGBA8888 src = makePattern(6, 4, 20);
        const auto expected = src.at(PoSize{3, 2});

        RGBA8888 moved(std::move(src));
        const bool ok = moved.getSize().x == 6 && moved.getSize().y == 4 &&
                        samePixel(moved.at(PoSize{3, 2}), expected) &&
                        src.getSize().x == 0 && src.getSize().y == 0 &&
                        src.getData() == nullptr;
        report("move constructor", ok,
               "moved=" + sizeText(moved) + " src=" + sizeText(src) + " src data=" +
                   (src.getData() == nullptr ? "null" : "non-null"));
    }

    // ---------------------------------------------------------------- 6
    // 重复赋值到同一个对象：不应泄漏（每次都要释放上一块缓冲）。
    {
        RGBA8888 dest;
        const long before = liveArrays();

        for (int i = 0; i < 64; ++i) {
            RGBA8888 tmp = makePattern(4 + (i % 3), 4, static_cast<uint8_t>(i));
            dest = tmp; // tmp 析构时释放自己的缓冲，dest 释放上一块
        }

        const long after = liveArrays();
        // dest 保持 1 块，循环内 tmp 的缓冲都已释放
        const bool ok = (after - before) == 1;
        report("repeated assign no leak", ok,
               "live " + std::to_string(before) + "->" + std::to_string(after) +
                   " (expect +1, dest 自己的缓冲)");
    }

    // ---------------------------------------------------------------- 7
    // 析构必须释放缓冲：整个作用域结束后净分配应回到基线。
    {
        RGBA8888 a = makePattern(16, 16, 1);
        RGBA8888 b(a);
        RGBA8888 c;
        c = b;
        RGBA8888 d(std::move(c));
        b = a;
        (void)d;
    }
    report("destructor releases buffers", liveArrays() == baseline,
           "live=" + std::to_string(liveArrays()) + " baseline=" + std::to_string(baseline));

    // ---------------------------------------------------------------- 8
    // Screen::copy 的整幅/局部重载（不涉及重新分配）。
    {
        RGBA8888 src = makePattern(4, 4, 30);
        RGBA8888 dest(PoSize{4, 4});
        dest.fill(RGBA8888::Pixel{255, 0, 0, 0});

        Screen::copy(src, dest);
        const bool whole_ok = samePixel(dest.at(PoSize{3, 3}), src.at(PoSize{3, 3}));

        RGBA8888 dest2(PoSize{4, 4});
        dest2.fill(RGBA8888::Pixel{255, 0, 0, 0});
        Screen::copy(src, dest2, PoSize{1, 1}, PoSize{2, 2}, PoSize{2, 2});
        const bool region_ok = samePixel(dest2.at(PoSize{2, 2}), src.at(PoSize{1, 1})) &&
                               samePixel(dest2.at(PoSize{3, 3}), src.at(PoSize{2, 2})) &&
                               samePixel(dest2.at(PoSize{0, 0}), RGBA8888::Pixel{255, 0, 0, 0}) &&
                               samePixel(dest2.at(PoSize{2, 0}), RGBA8888::Pixel{255, 0, 0, 0});

        report("Screen::copy overloads", whole_ok && region_ok,
               "whole(3,3)=" + pxText(dest.at(PoSize{3, 3})) + " region(2,2)=" +
                   pxText(dest2.at(PoSize{2, 2})) + " expect " + pxText(src.at(PoSize{1, 1})) +
                   " untouched(2,0)=" + pxText(dest2.at(PoSize{2, 0})));
    }

    // ---------------------------------------------------------------- 8b
    // 局部拷贝：非正方形缓冲（行宽必须是 _size.x，用 _size.y 会整块错位），
    // 以及越界位置必须被裁剪而不是写坏内存。
    {
        // 源 3x2，目标 5x4（宽 != 高，能暴露行宽 bug）
        RGBA8888 src = makePattern(3, 2, 60);
        RGBA8888 dest(PoSize{5, 4});
        dest.fill(RGBA8888::Pixel{255, 0, 0, 0});

        Screen::copy(src, dest, PoSize{0, 0}, PoSize{1, 1}, PoSize{3, 2});

        bool all_ok = true;
        for (int y = 0; y < 2; ++y)
            for (int x = 0; x < 3; ++x)
                all_ok = all_ok && samePixel(dest.at(PoSize{1 + x, 1 + y}), src.at(PoSize{x, y}));

        // 目标区域之外必须保持原样：(0,0) 与 (4,3) 都在拷贝区外
        all_ok = all_ok && samePixel(dest.at(PoSize{0, 0}), RGBA8888::Pixel{255, 0, 0, 0}) &&
                 samePixel(dest.at(PoSize{4, 3}), RGBA8888::Pixel{255, 0, 0, 0});

        // 越界：源位置超出尺寸 -> 一个像素都不该写
        RGBA8888 dest2(PoSize{4, 4});
        dest2.fill(RGBA8888::Pixel{255, 0, 0, 0});
        Screen::copy(src, dest2, PoSize{10, 10}, PoSize{0, 0}, PoSize{2, 2});
        bool untouched = true;
        for (int y = 0; y < 4; ++y)
            for (int x = 0; x < 4; ++x)
                untouched = untouched && samePixel(dest2.at(PoSize{x, y}), RGBA8888::Pixel{255, 0, 0, 0});

        report("region copy non-square/out-of-range", all_ok && untouched,
               "3x2 -> 5x4@(1,1) ok=" + std::to_string(all_ok ? 1 : 0) +
                   " src>>size untouched=" + std::to_string(untouched ? 1 : 0));
    }

    // ---------------------------------------------------------------- 9
    // Item::Image::setScreen 走 operator= 之后，渲染出的像素应来自新图。
    {
        using namespace xII;

        auto root = std::make_shared<Space::Point>();
        auto time_root = std::make_shared<Time::Point>();

        Camera::Standard camera(
            Item::Shape::Rectangle(
                std::make_shared<Space::OffsetPoint>(root, Space::Offset(0, 0)),
                static_cast<Val>(320), static_cast<Val>(180)),
            {SCREEN_W, SCREEN_H},
            std::make_shared<Item::Base>(),
            std::make_shared<Time::OffsetPoint>(time_root, Time::Offset(0)));

        // 源图 1x1，颜色 A；替换成 1x1 颜色 B。目标矩形覆盖整个视口。
        RGBA8888 first(PoSize{1, 1});
        first.fill(RGBA8888::Pixel{255, 10, 20, 30});

        auto image = std::make_shared<Item::Image>(
            first,
            Item::Shape::Rectangle(
                std::make_shared<Space::OffsetPoint>(root, Space::Offset(0, 0)),
                static_cast<Val>(320), static_cast<Val>(180)));

        camera.getObject()->addItem(image);
        camera.getScreenR().clear();
        camera.Shoot();
        const auto before = camera.getScreenR().at(PoSize{SCREEN_W / 2, SCREEN_H / 2});

        RGBA8888 second(PoSize{1, 1});
        second.fill(RGBA8888::Pixel{255, 200, 100, 50});
        image->setScreen(second); // operator= : resize + 深拷贝

        camera.getScreenR().clear();
        camera.Shoot();
        const auto after = camera.getScreenR().at(PoSize{SCREEN_W / 2, SCREEN_H / 2});

        const bool ok = before.R == 10 && before.G == 20 && before.B == 30 &&
                        after.R == 200 && after.G == 100 && after.B == 50 &&
                        image->getScreenRef().getSize().x == 1;
        report("Image::setScreen uses operator=", ok,
               "before=" + pxText(before) + " after=" + pxText(after) +
                   " src size=" + sizeText(image->getScreenRef()));
    }

    // ---------------------------------------------------------------- 10
    // 空图来源：默认构造（0x0）赋给已有内容的屏幕，应变成 0x0 且不崩溃。
    {
        RGBA8888 dest = makePattern(4, 4, 1);
        RGBA8888 empty;
        dest = empty;

        const bool ok = dest.getSize().x == 0 && dest.getSize().y == 0;
        report("assign from empty screen", ok,
               "dest=" + sizeText(dest) + " data=" +
                   (dest.getData() == nullptr ? "null" : "non-null"));
    }

    const long final_live = liveArrays();
    report("no leaks overall", final_live == baseline,
           "live=" + std::to_string(final_live) + " baseline=" + std::to_string(baseline) +
               " (allocs=" + std::to_string(g_array_allocs.load()) +
               " frees=" + std::to_string(g_array_frees.load()) + ")");

    std::printf("\n%s\n", g_failures == 0 ? "ALL SCREEN CHECKS PASS" : "SOME SCREEN CHECKS FAILED");
    return g_failures == 0 ? 0 : 1;
}
