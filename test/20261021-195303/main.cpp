// Screen::RGBA8888 拷贝/移动/赋值语义 —— 可视测试（仿照 test/20261021-190336 的样式）
//
// 做法：把每种拷贝方式得到的结果图交给 Item::Image 显示在屏幕上，
// 同时逐像素校验它与“原图”是否完全一致。所以窗口里看到的每一块，
// 就是一次 copy/assign 的真实产物；格子对不上/变黑就说明语义错了。
//
// 覆盖：
//   1. 拷贝构造                    Screen copy(src)
//   2. 默认构造后赋值（曾经崩）    Screen dst; dst = src;
//   3. 移动构造                    Screen dst(std::move(src));
//   4. 自赋值（曾经洗成垃圾）      a = a;
//   5. 尺寸不同赋值                small = big / big = small
//   6. 空图赋值                    empty = sized
//   7. 局部拷贝                    copy(src, dst, src_pos, dest_pos, size)
//   8. 局部拷贝（非正方形/越界）   行宽与越界裁剪
//   9. 重复赋值不泄漏              for(...) dst = tmp;
//
// 环境变量：
//   XII_SEMANTICS_HEADLESS=1      只跑校验，不开窗口
//   XII_SEMANTICS_DUMP=/path.bmp  保存一帧
//   XII_SEMANTICS_DUMP_ONLY=1     保存后立即退出
// 交互：Space 切换“动态拉伸”，Esc 退出

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <memory>
#include <new>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "Camera/Outline.h"
#include "Camera/Standard.h"
#include "Item/Base.h"
#include "Item/Image.h"
#include "Item/Shape/Rectangle.h"
#include "SDL3/SDL.h"
#include "Screen/Screen.h"
#include "Space/Offset.h"
#include "Space/OffsetPoint.h"
#include "Space/Point.h"
#include "Time/OffsetPoint.h"
#include "Time/Point.h"

// ------------------------------------------------------------------ 内存计数
// RGBA8888 的像素缓冲全部来自 new Pixel[...]，统计 new[] / delete[] 的净差
// 就能判断拷贝/赋值是否泄漏。统一走 malloc/free 以保证配对。
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

// ------------------------------------------------------------------ 常量
constexpr int SCREEN_WIDTH = 1280;
constexpr int SCREEN_HEIGHT = 720;
constexpr double RANGE_W = 1920.0;
constexpr double RANGE_H = 1080.0;

using namespace xII;

// 帧监控
std::atomic<size_t> frame_count{0};
std::atomic<bool> is_done{false};

void fps_ptf() {
    using namespace std::chrono;

    auto last_time = steady_clock::now();
    size_t last_count = 0;

    while (!is_done.load(std::memory_order_relaxed)) {
        std::this_thread::sleep_for(milliseconds(100));

        auto now = steady_clock::now();
        auto elapsed = duration_cast<milliseconds>(now - last_time).count();

        if (elapsed >= 1000) {
            size_t cur = frame_count.load(std::memory_order_relaxed);
            double fps = static_cast<double>(cur - last_count) * 1000.0 / static_cast<double>(elapsed);

            std::cout << "\rFPS: " << std::fixed << std::setprecision(1) << fps
                      << "   frames: " << cur
                      << "        " << std::flush;

            last_count = cur;
            last_time = now;
        }
    }
    std::cout << std::endl;
}

// ------------------------------------------------------------------ 工具
namespace {
    using Val = Base::Quantity::Base_Value_Type;

    int g_failures = 0;

    void report(const char* name, const bool ok, const std::string& detail) {
        std::cout << "[check] " << std::left << std::setw(34) << name
                  << (ok ? "PASS" : "FAIL") << "   " << detail << std::endl;
        if (!ok)
            ++g_failures;
    }

    bool samePixel(const Screen::Screen::Pixel& a, const Screen::Screen::Pixel& b) {
        return a.A == b.A && a.R == b.R && a.G == b.G && a.B == b.B;
    }

    std::string sizeText(const Screen::Screen& s) {
        return std::to_string(s.getSize().x) + "x" + std::to_string(s.getSize().y);
    }

    std::string pxText(const Screen::Screen::Pixel& p) {
        return "(" + std::to_string(p.A) + "," + std::to_string(p.R) + "," +
               std::to_string(p.G) + "," + std::to_string(p.B) + ")";
    }

    long liveArrays() {
        return g_array_allocs.load() - g_array_frees.load();
    }

    // 每个像素都不同的图案：四象限色 + 对角条纹 + 白框，便于肉眼看出
    // “拷错位置 / 拷成垃圾 / 只拷了一部分”。
    Screen::Screen makePattern(const int w, const int h, const uint8_t base = 0) {
        Screen::Screen img(Screen::PoSize{w, h});

        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                Screen::Screen::Pixel p{255, 0, 0, 0};

                // 四象限
                if (x < w / 2 && y < h / 2)
                    p = {255, 60, 200, 60};
                else if (x >= w / 2 && y < h / 2)
                    p = {255, 220, 60, 60};
                else if (x < w / 2)
                    p = {255, 60, 70, 220};
                else
                    p = {255, 230, 210, 50};

                // 每 8 像素加深一格，形成棋盘，拉伸后能看出采样块
                if (((x / 8) + (y / 8)) % 2 == 0) {
                    p.R = static_cast<uint8_t>(std::min(255, p.R + 25));
                    p.G = static_cast<uint8_t>(std::min(255, p.G + 25));
                    p.B = static_cast<uint8_t>(std::min(255, p.B + 25));
                }

                // 用 base 微调，让不同卡片有可区分的细微色差
                p.G = static_cast<uint8_t>(std::min(255, p.G + base));

                // 对角线与中心十字
                if (x == y || x == w - 1 - y || x == w / 2 || y == h / 2)
                    p = {255, 255, 255, 255};

                // 白框
                if (x == 0 || y == 0 || x == w - 1 || y == h - 1)
                    p = {255, 255, 255, 255};

                img.at(Screen::PoSize{x, y}) = p;
            }
        }
        return img;
    }

    // 逐像素比较（尺寸必须相同）
    bool identical(const Screen::Screen& a, const Screen::Screen& b, size_t* mismatch = nullptr) {
        if (a.getSize().x != b.getSize().x || a.getSize().y != b.getSize().y) {
            if (mismatch)
                *mismatch = static_cast<size_t>(-1);
            return false;
        }

        size_t diff = 0;
        for (int64_t y = 0; y < a.getSize().y; ++y)
            for (int64_t x = 0; x < a.getSize().x; ++x)
                if (!samePixel(a.at(Screen::PoSize{x, y}), b.at(Screen::PoSize{x, y})))
                    ++diff;

        if (mismatch)
            *mismatch = diff;
        return diff == 0;
    }

    struct CameraBundle {
        std::shared_ptr<Space::Point> root = std::make_shared<Space::Point>();
        std::shared_ptr<Time::Point> time_root = std::make_shared<Time::Point>();

        Camera::Standard standard{
            Item::Shape::Rectangle(
                std::make_shared<Space::OffsetPoint>(root, Space::Offset(0, 0)),
                static_cast<Val>(RANGE_W), static_cast<Val>(RANGE_H)),
            {SCREEN_WIDTH, SCREEN_HEIGHT},
            std::make_shared<Item::Base>(),
            std::make_shared<Time::OffsetPoint>(time_root, Time::Offset(0))};
    };

    // 屏幕上的一个“卡片”：一张源图 + 它在场景中的目标位置/尺寸。
    struct Card {
        std::shared_ptr<Item::Image> image;
        std::string name;
        double center_x = 0.0;
        double center_y = 0.0;
        double base_w = 240.0; // 屏幕像素
        double base_h = 240.0;
    };
} // namespace

int main() {
    std::cout << "Test 20261021-195303: Screen::RGBA8888 copy/move/assign (visual)" << std::endl;
    std::cout << "sizeof(RGBA8888) = " << sizeof(Screen::Screen) << " bytes" << std::endl;

    const long baseline = liveArrays();
    std::cout << "baseline live allocations = " << baseline << std::endl;

    CameraBundle scene;

    // ---------------------------------------------------------------- 源图
    // 64x64 的图案；所有卡片都应该显示成“一模一样”的这张图（局部拷贝那张除外）。
    const Screen::Screen original = makePattern(64, 64, 0);

    // ---------------------------------------------------------------- 1) 拷贝构造
    const Screen::Screen copy_ctor(original);
    {
        size_t mismatch = 0;
        const bool ok = copy_ctor.getSize().x == 64 && copy_ctor.getSize().y == 64 &&
                        identical(copy_ctor, original, &mismatch);
        report("copy constructor", ok,
               "size=" + sizeText(copy_ctor) + " mismatch=" + std::to_string(mismatch));
    }

    // ---------------------------------------------------------------- 2) 默认构造后赋值
    // 曾经因为 _data 未初始化，_resize 里 delete[] 野指针直接 abort。
    Screen::Screen assigned;
    {
        const long before = liveArrays();
        assigned = original;
        const long after = liveArrays();

        size_t mismatch = 0;
        const bool ok = identical(assigned, original, &mismatch) && (after - before) == 1;
        report("default-construct then assign", ok,
               "size=" + sizeText(assigned) + " mismatch=" + std::to_string(mismatch) +
                   " allocs+=" + std::to_string(after - before) + " (expect +1)");
    }

    // ---------------------------------------------------------------- 3) 移动构造
    // 注意：moved = std::move(tmp) 走的是拷贝赋值（operator= 只接受 const&），
    // 真正的移动构造必须用构造语法，否则测不到 move ctor。
    bool move_source_cleared = false;
    Screen::Screen moved = [&] {
        Screen::Screen tmp(original);
        Screen::Screen out(std::move(tmp));
        move_source_cleared = (tmp.getData() == nullptr &&
                               tmp.getSize().x == 0 && tmp.getSize().y == 0);
        return out; // NRVO，不会再额外移动
    }();
    {
        const bool ok = identical(moved, original) && move_source_cleared &&
                        samePixel(moved.at(Screen::PoSize{5, 7}), original.at(Screen::PoSize{5, 7}));
        report("move constructor", ok,
               "moved=" + sizeText(moved) + " source cleared=" +
                   std::to_string(move_source_cleared ? 1 : 0));
    }

    // ---------------------------------------------------------------- 4) 自赋值
    Screen::Screen self_assigned(original);
    {
        const auto before_px = self_assigned.at(Screen::PoSize{0, 0});
        const long before_live = liveArrays();

        self_assigned = self_assigned; // 曾经把数据洗成未初始化垃圾

        const long after_live = liveArrays();
        size_t mismatch = 0;
        const bool ok = identical(self_assigned, original, &mismatch) &&
                        samePixel(self_assigned.at(Screen::PoSize{0, 0}), before_px) &&
                        after_live == before_live;
        report("self assignment", ok,
               "mismatch=" + std::to_string(mismatch) + " (0,0)=" +
                   pxText(self_assigned.at(Screen::PoSize{0, 0})) +
                   " live " + std::to_string(before_live) + "->" + std::to_string(after_live));
    }

    // ---------------------------------------------------------------- 5) 尺寸不同赋值
    Screen::Screen resized_small;
    Screen::Screen resized_big;
    {
        Screen::Screen small = makePattern(8, 8, 40);
        Screen::Screen big = makePattern(64, 64, 90);

        resized_small = big; // 8x8 <- 64x64（放大）
        resized_big = small; // 64x64 <- 8x8（缩小）

        const bool ok = identical(resized_small, big) && identical(resized_big, small);
        report("assign with size change", ok,
               "8x8<-64x64 -> " + sizeText(resized_small) + " | 64x64<-8x8 -> " +
                   sizeText(resized_big));
    }

    // ---------------------------------------------------------------- 6) 空图赋值
    Screen::Screen from_empty;
    {
        Screen::Screen empty;
        from_empty = original;

        const bool shrink_to_zero = [&] {
            Screen::Screen t = original;
            t = empty;
            return t.getSize().x == 0 && t.getSize().y == 0;
        }();

        const bool ok = identical(from_empty, original) && shrink_to_zero;
        report("assign from empty screen", ok,
               "sized<-sized ok=" + std::to_string(identical(from_empty, original) ? 1 : 0) +
                   " sized<-empty->" + std::to_string(shrink_to_zero ? 1 : 0));
    }

    // ---------------------------------------------------------------- 7) 局部拷贝
    // 64x64 原图拷到 96x96 的 (16,16)，四周应保持黑色。
    // 曾经的 bug：源/目标 x 偏移丢失（总是贴第 0 列）、行宽误用 _size.y（非正方形错位）。
    Screen::Screen region;
    {
        region = Screen::Screen(Screen::PoSize{96, 96});
        region.fill(Screen::Screen::Pixel{255, 0, 0, 0});
        Screen::Screen::copy(original, region,
                             Screen::PoSize{0, 0}, Screen::PoSize{16, 16},
                             Screen::PoSize{64, 64});

        size_t mismatch = 0;
        for (int y = 0; y < 64; ++y)
            for (int x = 0; x < 64; ++x)
                if (!samePixel(region.at(Screen::PoSize{16 + x, 16 + y}),
                               original.at(Screen::PoSize{x, y})))
                    ++mismatch;

        // 边框必须没有被写过
        size_t outside = 0;
        for (int64_t y = 0; y < 96; ++y)
            for (int64_t x = 0; x < 96; ++x) {
                if (x >= 16 && x < 80 && y >= 16 && y < 80)
                    continue;
                const auto p = region.at(Screen::PoSize{x, y});
                if (p.R || p.G || p.B)
                    ++outside;
            }

        report("region copy (offsets)", mismatch == 0 && outside == 0,
               "size=" + sizeText(region) + " mismatch=" + std::to_string(mismatch) +
                   " ink outside=" + std::to_string(outside));
    }

    // ---------------------------------------------------------------- 8) 非正方形 + 越界
    Screen::Screen region_odd;
    {
        // 源 40x20，目标 96x48（宽 != 高，能暴露行宽 bug）
        Screen::Screen src = makePattern(40, 20, 30);
        region_odd = Screen::Screen(Screen::PoSize{96, 48});
        region_odd.fill(Screen::Screen::Pixel{255, 0, 0, 0});
        Screen::Screen::copy(src, region_odd,
                             Screen::PoSize{4, 2}, Screen::PoSize{20, 10},
                             Screen::PoSize{32, 16});

        size_t mismatch = 0;
        for (int y = 0; y < 16; ++y)
            for (int x = 0; x < 32; ++x)
                if (!samePixel(region_odd.at(Screen::PoSize{20 + x, 10 + y}),
                               src.at(Screen::PoSize{4 + x, 2 + y})))
                    ++mismatch;

        // 越界：源位置在缓冲之外，一个像素都不该写
        Screen::Screen oob(Screen::PoSize{32, 32});
        oob.fill(Screen::Screen::Pixel{255, 0, 0, 0});
        Screen::Screen::copy(src, oob, Screen::PoSize{100, 100},
                             Screen::PoSize{0, 0}, Screen::PoSize{8, 8});

        size_t oob_ink = 0;
        for (int64_t y = 0; y < 32; ++y)
            for (int64_t x = 0; x < 32; ++x) {
                const auto p = oob.at(Screen::PoSize{x, y});
                if (p.R || p.G || p.B)
                    ++oob_ink;
            }

        report("region copy non-square/oob", mismatch == 0 && oob_ink == 0,
               "40x20 -> 96x48@(20,10) mismatch=" + std::to_string(mismatch) +
                   " out-of-range ink=" + std::to_string(oob_ink));
    }

    // ---------------------------------------------------------------- 9) 重复赋值不泄漏
    {
        Screen::Screen dst;
        const long before = liveArrays();

        for (int i = 0; i < 64; ++i) {
            Screen::Screen tmp = makePattern(8 + (i % 3), 8, static_cast<uint8_t>(i));
            dst = tmp;
        }

        const long after = liveArrays();
        const bool ok = (after - before) == 1 && identical(dst, makePattern(8 + (63 % 3), 8, 63));
        report("repeated assign no leak", ok,
               "live " + std::to_string(before) + "->" + std::to_string(after) +
                   " (expect +1)");
    }

    // ---------------------------------------------------------------- 10) 析构释放
    // 只比较这个作用域前后的净分配：main 里其它存活对象的缓冲不计入。
    {
        const long before = liveArrays();
        {
            Screen::Screen a(original);
            Screen::Screen b(a);
            Screen::Screen c;
            c = b;
            Screen::Screen d(std::move(c));
            b = a;
            (void)d;
        }
        report("destructor releases buffers", liveArrays() == before,
               "live " + std::to_string(before) + "->" + std::to_string(liveArrays()) + " (expect equal)");
    }

    std::cout << (g_failures == 0 ? "ALL SCREEN COPY CHECKS PASS" : "SOME SCREEN COPY CHECKS FAILED")
              << std::endl;

    // ---------------------------------------------------------------- 可视演示
    // 把每种方式的结果交给 Item::Image 显示，格子之间只应有“局部拷贝”的黑边差异。
    std::vector<Card> cards;

    // 屏幕像素 -> 场景单位（相机 1920x1080 映射到 1280x720，比例恒为 1.5）
    constexpr double SCENE_PER_PX = RANGE_W / SCREEN_WIDTH;

    const auto add_card = [&](const Screen::Screen& screen, const std::string& name,
                              const double cx_px, const double cy_px) {
        Card card;
        card.name = name;
        card.center_x = cx_px * SCENE_PER_PX;
        card.center_y = cy_px * SCENE_PER_PX;
        card.image = std::make_shared<Item::Image>(
            screen,
            Item::Shape::Rectangle(
                std::make_shared<Space::OffsetPoint>(
                    scene.root,
                    Space::Offset(static_cast<Val>(card.center_x),
                                  static_cast<Val>(card.center_y))),
                static_cast<Val>(card.base_w * SCENE_PER_PX),
                static_cast<Val>(card.base_h * SCENE_PER_PX)));
        scene.standard.getObject()->addItem(card.image);
        cards.push_back(card);
    };

    // 2 行 x 4 列，按屏幕像素给出中心位置
    constexpr double COL[] = {-450.0, -150.0, 150.0, 450.0};
    constexpr double ROW[] = {-180.0, 180.0};

    add_card(copy_ctor,     "copy ctor",     COL[0], ROW[0]);
    add_card(assigned,      "assign",        COL[1], ROW[0]);
    add_card(moved,         "move",          COL[2], ROW[0]);
    add_card(self_assigned, "self assign",   COL[3], ROW[0]);
    add_card(region,        "region (pad)",  COL[0], ROW[1]);
    add_card(region_odd,    "region odd",    COL[1], ROW[1]);
    add_card(resized_small, "8->64 grow",    COL[2], ROW[1]);
    add_card(resized_big,   "64->8 shrink",  COL[3], ROW[1]);
    // 无窗口模式：只跑校验
    if (std::getenv("XII_SEMANTICS_HEADLESS") != nullptr) {
        std::cout << "(headless: skip window)" << std::endl;
        return g_failures == 0 ? 0 : 1;
    }

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return -1;
    }

    SDL_Window* window = SDL_CreateWindow("xII Screen Copy Semantics  [Space: stretch]",
                                          SCREEN_WIDTH, SCREEN_HEIGHT, 0);
    if (!window) {
        std::cerr << "create window failed: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return -1;
    }

    SDL_Surface* window_surface = SDL_GetWindowSurface(window);
    SDL_Surface* surface = SDL_CreateSurfaceFrom(
        SCREEN_WIDTH, SCREEN_HEIGHT, SDL_PIXELFORMAT_BGRA8888,
        scene.standard.getScreen()->getData(), SCREEN_WIDTH * 4);
    if (!window_surface || !surface) {
        std::cerr << "surface failed: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }
    SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_NONE);

    std::thread fps_thread(fps_ptf);
    std::cout << "Space: toggle animated stretch / Esc: quit" << std::endl;

    bool animate = false;
    bool dumped = false;
    double t = 0.0;

    while (!is_done.load(std::memory_order_relaxed)) {
        SDL_Event ev{};
        while (SDL_PollEvent(&ev) != 0) {
            if (ev.type == SDL_EVENT_QUIT) {
                is_done.store(true);
            } else if (ev.type == SDL_EVENT_KEY_DOWN && !ev.key.repeat) {
                if (ev.key.key == SDLK_ESCAPE) {
                    is_done.store(true);
                } else if (ev.key.key == SDLK_SPACE) {
                    animate = !animate;
                    std::cout << "\nmode: " << (animate ? "animated stretch" : "static") << std::endl;
                }
            }
        }

        t += 0.016;
        for (auto& card : cards) {
            // 拉伸只改目标矩形，源图（拷贝结果）本身不变 —— 窗口里内容不应变化
            const double fx = animate ? (1.0 + 0.25 * std::sin(t)) : 1.0;
            const double fy = animate ? (1.0 + 0.25 * std::cos(t * 1.3)) : 1.0;
            card.image->setShape(Item::Shape::Rectangle(
                card.image->getShapeRef().getCenter(),
                static_cast<Val>(card.base_w * fx * (RANGE_W / SCREEN_WIDTH)),
                static_cast<Val>(card.base_h * fy * (RANGE_H / SCREEN_HEIGHT))));
        }

        scene.standard.getTimePoint()->add();
        scene.standard.getScreen()->clear();
        scene.standard.Shoot();

        SDL_BlitSurface(surface, nullptr, window_surface, nullptr);
        SDL_UpdateWindowSurface(window);
        frame_count.fetch_add(1, std::memory_order_relaxed);

        // 无交互环境下的可视验证
        if (!dumped) {
            if (const char* dump_path = std::getenv("XII_SEMANTICS_DUMP"); dump_path != nullptr) {
                if (SDL_SaveBMP(surface, dump_path))
                    std::cout << "dumped frame to " << dump_path << std::endl;
                else
                    std::cerr << "SDL_SaveBMP failed: " << SDL_GetError() << std::endl;
                dumped = true;
                if (const char* only = std::getenv("XII_SEMANTICS_DUMP_ONLY");
                    only != nullptr && std::string(only) == "1")
                    is_done.store(true);
            }
        }
    }

    is_done.store(true, std::memory_order_relaxed);
    fps_thread.join();

    SDL_DestroySurface(surface);
    SDL_DestroyWindow(window);
    SDL_Quit();

    std::cout << (g_failures == 0 ? "ALL SCREEN COPY CHECKS PASS" : "SOME SCREEN COPY CHECKS FAILED")
              << std::endl;
    return g_failures == 0 ? 0 : 1;
}
