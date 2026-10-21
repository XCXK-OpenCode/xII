// Item::Image 测试：源图 -> 场景矩形（拉伸/缩放）的最近邻采样。
// 先做无窗口的逐像素校验，再开窗口做可视演示（空格切换拉伸方式）。

#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
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

constexpr int SCREEN_WIDTH = 1280;
constexpr int SCREEN_HEIGHT = 720;
constexpr double RANGE_W = 1920.0;
constexpr double RANGE_H = 1080.0;

using namespace xII;
using Val = xII::Base::Quantity::Base_Value_Type;

namespace {
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
                          << "   frames: " << cur << "        " << std::flush;
                last_count = cur;
                last_time = now;
            }
        }
        std::cout << std::endl;
    }

    // 生成源图：四象限色块 + 棋盘格 + 白色边框 + 对角线。
    // 每个像素的颜色都是确定的，便于逐像素校验采样映射。
    Screen::Screen makeSource(const int w, const int h) {
        Screen::Screen img({w, h});
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                Screen::Screen::Pixel p{255, 0, 0, 0};

                // 四象限（右上红、左上绿、左下蓝、右下黄）
                if (x < w / 2 && y < h / 2)      p = {255, 40, 200, 60};   // 绿
                else if (x >= w / 2 && y < h / 2) p = {255, 220, 40, 40};  // 红
                else if (x < w / 2)               p = {255, 40, 60, 220};  // 蓝
                else                              p = {255, 230, 210, 40}; // 黄

                // 棋盘格：隔 4 像素加亮，拉伸时能看出采样块大小
                if (((x / 4) + (y / 4)) % 2 == 0) {
                    p.R = static_cast<uint8_t>(std::min(255, p.R + 30));
                    p.G = static_cast<uint8_t>(std::min(255, p.G + 30));
                    p.B = static_cast<uint8_t>(std::min(255, p.B + 30));
                }

                // 白色边框
                if (x == 0 || y == 0 || x == w - 1 || y == h - 1)
                    p = {255, 255, 255, 255};

                // 对角线与中心十字
                if (x == y || x == w - 1 - y || x == w / 2 || y == h / 2)
                    p = {255, 255, 255, 255};

                img.at({x, y}) = p;
            }
        }
        return img;
    }

    // 每个像素都不同的“坐标编码图”，用于精确校验采样下标。
    Screen::Screen makeCoordinateSource(const int w, const int h, const uint8_t alpha = 255) {
        Screen::Screen img({w, h});
        for (int y = 0; y < h; ++y)
            for (int x = 0; x < w; ++x)
                img.at({x, y}) = {alpha,
                                  static_cast<uint8_t>(10 + x * 20),
                                  static_cast<uint8_t>(10 + y * 20),
                                  static_cast<uint8_t>(100 + x * 5 + y * 3)};
        return img;
    }

    // 与 Image::Render 的采样公式一致（像素中心 + floor）。
    struct Mapping {
        double left = 0, top = 0, right = 0, bottom = 0;
        int src_w = 0, src_h = 0;

        void sample(const int64_t x, const int64_t y, int64_t& sx, int64_t& sy) const {
            const double u = (static_cast<double>(x) + 0.5 - left) / (right - left);
            const double v = (static_cast<double>(y) + 0.5 - top) / (bottom - top);
            sx = static_cast<int64_t>(std::floor(u * src_w));
            sy = static_cast<int64_t>(std::floor(v * src_h));
            sx = std::clamp<int64_t>(sx, 0, src_w - 1);
            sy = std::clamp<int64_t>(sy, 0, src_h - 1);
        }
    };

    struct CameraBundle {
        std::shared_ptr<Space::Point> root = std::make_shared<Space::Point>();
        std::shared_ptr<Time::Point> time_root = std::make_shared<Time::Point>();

        Camera::Standard standard{
            Item::Shape::Rectangle(
                std::make_shared<Space::OffsetPoint>(root, Space::Offset(0, 0)),
                static_cast<Val>(RANGE_W), static_cast<Val>(RANGE_H)),
            {SCREEN_WIDTH, SCREEN_HEIGHT}, std::make_shared<Item::Base>(),
            std::make_shared<Time::OffsetPoint>(time_root, Time::Offset(0))};

        Camera::Outline outline{
            Item::Shape::Rectangle(
                std::make_shared<Space::OffsetPoint>(root, Space::Offset(0, 0)),
                static_cast<Val>(RANGE_W), static_cast<Val>(RANGE_H)),
            {SCREEN_WIDTH, SCREEN_HEIGHT}, std::make_shared<Item::Base>(),
            std::make_shared<Time::OffsetPoint>(time_root, Time::Offset(0)),
            Screen::Screen::Pixel{255, 255, 255, 255}, 2};

        std::shared_ptr<Space::OffsetPoint> at(const double x, const double y) {
            return std::make_shared<Space::OffsetPoint>(
                root, Space::Offset(static_cast<Val>(x), static_cast<Val>(y)));
        }
    };

    bool samePixel(const Screen::Screen::Pixel& a, const Screen::Screen::Pixel& b) {
        return a.A == b.A && a.R == b.R && a.G == b.G && a.B == b.B;
    }

    size_t countInk(const Screen::Screen& screen) {
        const auto* data = screen.getData();
        const auto size = screen.getSize();
        size_t n = 0;
        for (int64_t i = 0; i < size.x * size.y; ++i)
            if (data[i].R || data[i].G || data[i].B)
                ++n;
        return n;
    }
} // namespace

int main() {
    std::cout << "Test 20261021-190336: Item::Image" << std::endl;

    bool checks_ok = true;
    const auto report = [&](const char* name, const bool ok, const std::string& detail) {
        std::cout << "[check] " << name << ": " << detail << "  " << (ok ? "PASS" : "FAIL") << std::endl;
        checks_ok = checks_ok && ok;
    };

    CameraBundle scene;

    // 相机 1920x1080 -> 屏幕 1280x720，缩放 2/3。
    // 目标矩形尺寸 = 场景单位 * 2/3，所以 12 场景单位 = 8 屏幕像素。
    constexpr double PX = 1.5; // 1 屏幕像素 = 1.5 场景单位

    // ---------------- 1) 1:1 拷贝 ----------------
    {
        const auto src = makeCoordinateSource(8, 8);
        auto image = std::make_shared<Item::Image>(
            src, Item::Shape::Rectangle(scene.at(0, 0), static_cast<Val>(8 * PX), static_cast<Val>(8 * PX)));

        const Item::Image::ScreenRect rect = image->getScreenRect(scene.standard);

        scene.standard.getObject()->clear();
        scene.standard.getObject()->addItem(image);
        scene.standard.getScreenR().clear();
        scene.standard.Shoot();

        const auto& out = scene.standard.getScreenR();
        const int64_t left = static_cast<int64_t>(rect.left);
        const int64_t top = static_cast<int64_t>(rect.top);

        size_t mismatch = 0;
        for (int y = 0; y < 8; ++y)
            for (int x = 0; x < 8; ++x)
                if (!samePixel(out.at({left + x, top + y}), src.at({x, y})))
                    ++mismatch;

        const bool ok = rect.valid && std::abs(rect.width() - 8.0) < 0.01 &&
                        std::abs(rect.height() - 8.0) < 0.01 && mismatch == 0;
        report("1:1 copy", ok,
               "dest=" + std::to_string(rect.width()) + "x" + std::to_string(rect.height()) +
                   " px, mismatch=" + std::to_string(mismatch));
    }

    // ---------------- 2) 2x 放大：每个源像素 -> 2x2 ----------------
    {
        const auto src = makeCoordinateSource(8, 8);
        auto image = std::make_shared<Item::Image>(
            src, Item::Shape::Rectangle(scene.at(0, 0), static_cast<Val>(16 * PX), static_cast<Val>(16 * PX)));

        const Item::Image::ScreenRect rect = image->getScreenRect(scene.standard);

        scene.standard.getObject()->clear();
        scene.standard.getObject()->addItem(image);
        scene.standard.getScreenR().clear();
        scene.standard.Shoot();

        const auto& out = scene.standard.getScreenR();
        const int64_t left = static_cast<int64_t>(rect.left);
        const int64_t top = static_cast<int64_t>(rect.top);

        size_t mismatch = 0;
        Mapping map{rect.left, rect.top, rect.right, rect.bottom, 8, 8};
        for (int y = 0; y < 16; ++y) {
            for (int x = 0; x < 16; ++x) {
                int64_t sx = 0, sy = 0;
                map.sample(left + x, top + y, sx, sy);
                if (!samePixel(out.at({left + x, top + y}), src.at({sx, sy})))
                    ++mismatch;
            }
        }

        // 额外确认“块”特性：每 2x2 内的 4 个像素完全相同。
        size_t block_mismatch = 0;
        for (int y = 0; y < 16; y += 2)
            for (int x = 0; x < 16; x += 2) {
                const auto p = out.at({left + x, top + y});
                if (!samePixel(p, out.at({left + x + 1, top + y})) ||
                    !samePixel(p, out.at({left + x, top + y + 1})) ||
                    !samePixel(p, out.at({left + x + 1, top + y + 1})))
                    ++block_mismatch;
            }

        const bool ok = rect.valid && std::abs(rect.width() - 16.0) < 0.01 &&
                        mismatch == 0 && block_mismatch == 0;
        report("2x upscale", ok,
               "dest=" + std::to_string(rect.width()) + "x" + std::to_string(rect.height()) +
                   " px, mismatch=" + std::to_string(mismatch) +
                   ", 2x2 block mismatch=" + std::to_string(block_mismatch));
    }

    // ---------------- 3) 非等比拉伸：横向 4x、纵向 1x ----------------
    {
        const auto src = makeCoordinateSource(8, 8);
        // 32 屏幕像素宽 x 8 屏幕像素高
        auto image = std::make_shared<Item::Image>(
            src, Item::Shape::Rectangle(scene.at(0, 0), static_cast<Val>(32 * PX), static_cast<Val>(8 * PX)));

        const Item::Image::ScreenRect rect = image->getScreenRect(scene.standard);

        scene.standard.getObject()->clear();
        scene.standard.getObject()->addItem(image);
        scene.standard.getScreenR().clear();
        scene.standard.Shoot();

        const auto& out = scene.standard.getScreenR();
        const int64_t left = static_cast<int64_t>(rect.left);
        const int64_t top = static_cast<int64_t>(rect.top);

        size_t mismatch = 0;
        Mapping map{rect.left, rect.top, rect.right, rect.bottom, 8, 8};
        for (int y = 0; y < 8; ++y) {
            for (int x = 0; x < 32; ++x) {
                int64_t sx = 0, sy = 0;
                map.sample(left + x, top + y, sx, sy);
                if (!samePixel(out.at({left + x, top + y}), src.at({sx, sy})))
                    ++mismatch;
            }
        }

        // 横向每 4 个像素应采样同一列
        size_t column_mismatch = 0;
        for (int y = 0; y < 8; ++y)
            for (int x = 0; x + 3 < 32; x += 4) {
                const auto p = out.at({left + x, top + y});
                for (int k = 1; k < 4; ++k)
                    if (!samePixel(p, out.at({left + x + k, top + y})))
                        ++column_mismatch;
            }

        const bool ok = rect.valid && std::abs(rect.width() - 32.0) < 0.01 &&
                        std::abs(rect.height() - 8.0) < 0.01 &&
                        mismatch == 0 && column_mismatch == 0;
        report("non-uniform stretch", ok,
               "dest=" + std::to_string(rect.width()) + "x" + std::to_string(rect.height()) +
                   " px, mismatch=" + std::to_string(mismatch) +
                   ", column mismatch=" + std::to_string(column_mismatch));
    }

    // ---------------- 4) 缩小：8x8 -> 4x4，采样应落在奇数行列 ----------------
    {
        const auto src = makeCoordinateSource(8, 8);
        auto image = std::make_shared<Item::Image>(
            src, Item::Shape::Rectangle(scene.at(0, 0), static_cast<Val>(4 * PX), static_cast<Val>(4 * PX)));

        const Item::Image::ScreenRect rect = image->getScreenRect(scene.standard);

        scene.standard.getObject()->clear();
        scene.standard.getObject()->addItem(image);
        scene.standard.getScreenR().clear();
        scene.standard.Shoot();

        const auto& out = scene.standard.getScreenR();
        const int64_t left = static_cast<int64_t>(rect.left);
        const int64_t top = static_cast<int64_t>(rect.top);

        size_t mismatch = 0;
        for (int y = 0; y < 4; ++y)
            for (int x = 0; x < 4; ++x) {
                const int64_t sx = x * 2 + 1;
                const int64_t sy = y * 2 + 1;
                if (!samePixel(out.at({left + x, top + y}), src.at({sx, sy})))
                    ++mismatch;
            }

        const bool ok = rect.valid && std::abs(rect.width() - 4.0) < 0.01 && mismatch == 0;
        report("downscale 8->4", ok,
               "dest=" + std::to_string(rect.width()) + "x" + std::to_string(rect.height()) +
                   " px, mismatch=" + std::to_string(mismatch) +
                   " (expected samples odd src rows/cols 1,3,5,7)");
    }

    // ---------------- 5) 源像素 alpha 与背景混合 ----------------
    {
        const auto src = makeCoordinateSource(4, 4, 128);
        auto image = std::make_shared<Item::Image>(
            src, Item::Shape::Rectangle(scene.at(0, 0), static_cast<Val>(4 * PX), static_cast<Val>(4 * PX)));

        const Item::Image::ScreenRect rect = image->getScreenRect(scene.standard);

        scene.standard.getObject()->clear();
        scene.standard.getObject()->addItem(image);
        scene.standard.getScreenR().clear();
        scene.standard.Shoot();

        const int64_t left = static_cast<int64_t>(rect.left);
        const int64_t top = static_cast<int64_t>(rect.top);

        // 源 A=128 覆盖在黑色不透明背景上：out_c ≈ c * 128 / 255
        size_t mismatch = 0;
        int worst = 0;
        for (int y = 0; y < 4; ++y)
            for (int x = 0; x < 4; ++x) {
                const auto s = src.at({x, y});
                const auto d = scene.standard.getScreenR().at({left + x, top + y});
                const auto expect = [](uint8_t c) {
                    return static_cast<int>(std::lround(static_cast<double>(c) * 128.0 / 255.0));
                };
                const int dr = std::abs(static_cast<int>(d.R) - expect(s.R));
                const int dg = std::abs(static_cast<int>(d.G) - expect(s.G));
                const int db = std::abs(static_cast<int>(d.B) - expect(s.B));
                worst = std::max({worst, dr, dg, db});
                if (dr > 1 || dg > 1 || db > 1)
                    ++mismatch;
            }

        report("source alpha", mismatch == 0 && worst <= 1,
               "mismatch=" + std::to_string(mismatch) + ", worst channel delta=" + std::to_string(worst) +
                   " (alpha 128 over opaque black)");
    }

    // ---------------- 6) Renderer::Args 的 alpha（SetAlpha/Fade） ----------------
    {
        const auto src = makeCoordinateSource(4, 4, 255);
        auto image = std::make_shared<Item::Image>(
            src, Item::Shape::Rectangle(scene.at(0, 0), static_cast<Val>(4 * PX), static_cast<Val>(4 * PX)));

        const Item::Image::ScreenRect rect = image->getScreenRect(scene.standard);

        Item::Renderer::Args args;
        args.setAlpha(128);

        scene.standard.getObject()->clear();
        scene.standard.getObject()->addItem(image);
        scene.standard.getScreenR().clear();
        scene.standard.getObject()->Render(scene.standard, args);

        const int64_t left = static_cast<int64_t>(rect.left);
        const int64_t top = static_cast<int64_t>(rect.top);

        size_t mismatch = 0;
        int worst = 0;
        for (int y = 0; y < 4; ++y)
            for (int x = 0; x < 4; ++x) {
                const auto s = src.at({x, y});
                const auto d = scene.standard.getScreenR().at({left + x, top + y});
                const auto expect = [](uint8_t c) {
                    return static_cast<int>(std::lround(static_cast<double>(c) * 128.0 / 255.0));
                };
                worst = std::max({worst,
                                  std::abs(static_cast<int>(d.R) - expect(s.R)),
                                  std::abs(static_cast<int>(d.G) - expect(s.G)),
                                  std::abs(static_cast<int>(d.B) - expect(s.B))});
                if (worst > 1)
                    ++mismatch;
            }

        report("args alpha=128", mismatch == 0 && worst <= 1,
               "worst channel delta=" + std::to_string(worst) + " (opaque source, args alpha 128)");

        // alpha = 0 时一个像素都不应该画
        Item::Renderer::Args zero;
        zero.setAlpha(0);
        scene.standard.getScreenR().clear();
        scene.standard.getObject()->Render(scene.standard, zero);
        const size_t ink = countInk(scene.standard.getScreenR());
        report("args alpha=0", ink == 0, "ink=" + std::to_string(ink));
    }

    // ---------------- 7) isInRange 视口剔除 ----------------
    {
        auto make = [&](const double cx, const double cy) {
            return std::make_shared<Item::Image>(
                makeCoordinateSource(8, 8),
                Item::Shape::Rectangle(scene.at(cx, cy), static_cast<Val>(8 * PX), static_cast<Val>(8 * PX)));
        };

        auto inside = make(0, 0);
        auto outside = make(5000, 0);
        auto straddle = make(RANGE_W / 2.0, 0); // 中心正好在视口右边界

        const bool in_1 = inside->isInRange(scene.standard);
        const bool in_2 = outside->isInRange(scene.standard);
        const bool in_3 = straddle->isInRange(scene.standard);

        scene.standard.getObject()->clear();
        scene.standard.getObject()->addItem(outside);
        scene.standard.getScreenR().clear();
        scene.standard.Shoot();
        const size_t ink_outside = countInk(scene.standard.getScreenR());

        report("isInRange", in_1 && !in_2 && in_3 && ink_outside == 0,
               "inside=" + std::to_string(in_1 ? 1 : 0) +
                   " outside=" + std::to_string(in_2 ? 1 : 0) +
                   " straddle=" + std::to_string(in_3 ? 1 : 0) +
                   " ink(outside)=" + std::to_string(ink_outside));
    }

    // ---------------- 8) Outline 相机只画边框，不画像素 ----------------
    {
        // 用大一点的图，避开 Outline 画的边框/对角线，方便取内部采样点。
        const auto src = makeSource(64, 64);
        auto image = std::make_shared<Item::Image>(
            src, Item::Shape::Rectangle(scene.at(0, 0), static_cast<Val>(64 * PX), static_cast<Val>(64 * PX)));

        scene.outline.getObject()->clear();
        scene.outline.getObject()->addItem(image);
        scene.outline.getScreenR().clear();
        scene.outline.Shoot();

        const auto& out = scene.outline.getScreenR();
        const Item::Image::ScreenRect rect = image->getScreenRect(scene.outline);

        // 内部采样点：x=25%、y=50%，不在四条边、两条对角线、也不在四角上。
        const int64_t sx = static_cast<int64_t>(rect.left + rect.width() * 0.25);
        const int64_t sy = static_cast<int64_t>(rect.top + rect.height() * 0.50);
        const auto p = out.at({sx, sy});

        // 除边框颜色（白）与背景（黑）之外不应有任何像素：
        // 源图里全是彩色，一旦被画出来就会命中这个计数。
        const auto* data = out.getData();
        size_t colored = 0;
        for (int64_t y = 0; y < out.getSize().y; ++y)
            for (int64_t x = 0; x < out.getSize().x; ++x) {
                const auto& c = data[y * out.getSize().x + x];
                const bool white = c.R == 255 && c.G == 255 && c.B == 255;
                const bool black = c.R == 0 && c.G == 0 && c.B == 0;
                if (!white && !black)
                    ++colored;
            }

        const bool ok = countInk(out) > 0 && colored == 0 && p.R == 0 && p.G == 0 && p.B == 0;
        report("outline camera", ok,
               "border ink>0, interior=(" + std::to_string(p.R) + "," + std::to_string(p.G) + "," +
                   std::to_string(p.B) + "), non-border-colored pixels=" + std::to_string(colored));
    }

    // ---------------- 9) setScreen 深拷贝 ----------------
    {
        auto src = makeCoordinateSource(8, 8);
        auto image = std::make_shared<Item::Image>(
            src, Item::Shape::Rectangle(scene.at(0, 0), static_cast<Val>(8 * PX), static_cast<Val>(8 * PX)));

        const auto before = image->getScreenRef().at({0, 0});
        src.at({0, 0}) = Screen::Screen::Pixel{255, 1, 2, 3}; // 改调用方的缓冲
        const auto after_external = image->getScreenRef().at({0, 0});

        auto replacement = makeCoordinateSource(8, 8);
        replacement.at({0, 0}) = Screen::Screen::Pixel{255, 7, 8, 9};
        image->setScreen(replacement);
        const auto after_set = image->getScreenRef().at({0, 0});

        const bool ok = samePixel(before, after_external) &&
                        after_set.R == 7 && after_set.G == 8 && after_set.B == 9;
        report("setScreen deep copy", ok,
               "external edit isolated=" + std::to_string(samePixel(before, after_external) ? 1 : 0) +
                   ", after setScreen rgb=" + std::to_string(after_set.R) + "," +
                   std::to_string(after_set.G) + "," + std::to_string(after_set.B));
    }

    // ---------------- 9b) RGBA8888::operator= 的语义 ----------------
    {
        // operator= 必须是完整深拷贝：先 resize（改变 _size 并换缓冲）再整体拷贝。
        // 这样默认构造/尺寸不同的目标都能正确接收，自赋值也安全。
        Screen::Screen big = makeCoordinateSource(8, 8);
        big.at({7, 7}) = Screen::Screen::Pixel{255, 111, 222, 233};

        Screen::Screen dest;                    // 默认构造：_data 为空
        dest = big;                             // 必须 resize
        const bool resize_ok = dest.getSize().x == 8 && dest.getSize().y == 8 &&
                               samePixel(dest.at({7, 7}), big.at({7, 7}));

        Screen::Screen self = makeCoordinateSource(4, 4);
        self.at({0, 0}) = Screen::Screen::Pixel{255, 44, 55, 66};
        const auto self_before = self.at({0, 0});
        self = self;                            // 自赋值不能把自己清成垃圾
        const bool self_ok = samePixel(self.at({0, 0}), self_before);

        report("operator= deep copy", resize_ok && self_ok,
               "default-construct assign -> " + std::to_string(dest.getSize().x) + "x" +
                   std::to_string(dest.getSize().y) + " pixel(7,7)=" +
                   std::to_string(dest.at({7, 7}).R) + ", self-assign kept=" +
                   std::to_string(self_ok ? 1 : 0));
    }

    // ---------------- 10) 空图 / 零尺寸不崩溃 ----------------
    {
        auto empty = std::make_shared<Item::Image>(
            Screen::Screen({0, 0}),
            Item::Shape::Rectangle(scene.at(0, 0), static_cast<Val>(100), static_cast<Val>(100)));
        auto zero_size = std::make_shared<Item::Image>(
            makeCoordinateSource(8, 8),
            Item::Shape::Rectangle(scene.at(0, 0), static_cast<Val>(0), static_cast<Val>(0)));

        scene.standard.getObject()->clear();
        scene.standard.getObject()->addItem(empty);
        scene.standard.getObject()->addItem(zero_size);
        scene.standard.getScreenR().clear();
        scene.standard.Shoot();

        const size_t ink = countInk(scene.standard.getScreenR());
        report("empty/zero size", ink == 0 && !zero_size->isInRange(scene.standard),
               "ink=" + std::to_string(ink) + ", zero-size isInRange=" +
                   std::to_string(zero_size->isInRange(scene.standard) ? 1 : 0));
    }

    // ---------------- 11) 性能：每帧渲染耗时 ----------------
    {
        using clock = std::chrono::steady_clock;
        // 每帧都 clear()，所以测得值 = 清屏 + 绘制。为了压掉系统抖动，
        // 跑若干批、每批若干帧，取“最快的一批”作为结果。
        const auto bench = [&](const std::shared_ptr<Item::Image>& item, const int iterations) {
            scene.standard.getObject()->clear();
            scene.standard.getObject()->addItem(item);

            for (int i = 0; i < 5; ++i) { // 预热
                scene.standard.getScreenR().clear();
                scene.standard.Shoot();
            }

            double best = 1e9;
            for (int batch = 0; batch < 5; ++batch) {
                const auto start = clock::now();
                for (int i = 0; i < iterations; ++i) {
                    scene.standard.getScreenR().clear();
                    scene.standard.Shoot();
                }
                const auto end = clock::now();
                const double ms = std::chrono::duration<double, std::milli>(end - start).count() / iterations;
                best = std::min(best, ms);
            }
            return best;
        };

        const auto bench_clear = [&]() {
            scene.standard.getObject()->clear();
            for (int i = 0; i < 5; ++i) {
                scene.standard.getScreenR().clear();
                scene.standard.Shoot();
            }
            double best = 1e9;
            for (int batch = 0; batch < 5; ++batch) {
                const auto start = clock::now();
                for (int i = 0; i < 30; ++i) {
                    scene.standard.getScreenR().clear();
                    scene.standard.Shoot();
                }
                const auto end = clock::now();
                best = std::min(best, std::chrono::duration<double, std::milli>(end - start).count() / 30.0);
            }
            return best;
        };

        // 全屏（1280x720）拉伸、半屏、64x64
        const auto full = std::make_shared<Item::Image>(
            makeSource(64, 64),
            Item::Shape::Rectangle(scene.at(0, 0), static_cast<Val>(RANGE_W), static_cast<Val>(RANGE_H)));
        const auto half = std::make_shared<Item::Image>(
            makeSource(64, 64),
            Item::Shape::Rectangle(scene.at(0, 0), static_cast<Val>(RANGE_W / 2), static_cast<Val>(RANGE_H / 2)));
        const auto small = std::make_shared<Item::Image>(
            makeSource(64, 64),
            Item::Shape::Rectangle(scene.at(0, 0), static_cast<Val>(96), static_cast<Val>(96)));

        // 基线：只清屏（不含任何 Item）。bench() 内部每帧都 clear()，
        // 所以实际绘制耗时 = 测得值 - 基线。
        const double ms_clear = bench_clear();
        std::cout << "[bench] clear only      : " << std::fixed << std::setprecision(3) << ms_clear
                  << " ms/frame (baseline)" << std::endl;

        const double ms_full = bench(full, 20);
        const double ms_half = bench(half, 20);
        const double ms_small = bench(small, 40);

        std::cout << "[bench] full 1280x720: " << std::fixed << std::setprecision(3) << ms_full
                  << " ms/frame  -> draw " << (ms_full - ms_clear) << " ms  ("
                  << static_cast<int>(1000.0 / ms_full) << " FPS)\n"
                  << "[bench] half  640x360 : " << ms_half << " ms/frame  -> draw "
                  << (ms_half - ms_clear) << " ms  ("
                  << static_cast<int>(1000.0 / ms_half) << " FPS)\n"
                  << "[bench] small  64x 64 : " << ms_small << " ms/frame  -> draw "
                  << (ms_small - ms_clear) << " ms  ("
                  << static_cast<int>(1000.0 / ms_small) << " FPS)" << std::endl;

        // 全屏绘制不应慢到不可用：给一个宽松上界（Release 下实测 ~0.5 ms）。
        report("perf full-screen", ms_full < 8.0,
               std::to_string(ms_full) + " ms/frame, budget 8 ms");
    }

    std::cout << (checks_ok ? "ALL IMAGE CHECKS PASS" : "SOME IMAGE CHECKS FAILED") << std::endl;

    // ---------------- 可视演示 ----------------
    if (std::getenv("XII_IMAGE_HEADLESS") != nullptr) {
        std::cout << "(headless: skip window)" << std::endl;
        return checks_ok ? 0 : 1;
    }

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return -1;
    }

    SDL_Window* window = SDL_CreateWindow("xII Image Test  [Space: stretch mode]", SCREEN_WIDTH, SCREEN_HEIGHT, 0);
    if (!window) {
        std::cerr << "create window failed: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return -1;
    }

    SDL_Surface* window_surface = SDL_GetWindowSurface(window);
    SDL_Surface* surface = SDL_CreateSurfaceFrom(
        SCREEN_WIDTH, SCREEN_HEIGHT, SDL_PIXELFORMAT_BGRA8888,
        scene.standard.getScreenR().getData(), SCREEN_WIDTH * 4);
    if (!window_surface || !surface) {
        std::cerr << "surface failed: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }
    SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_NONE);

    // 演示用的源图与若干不同拉伸方式的 Item。
    struct Demo {
        std::shared_ptr<Item::Image> image;
        double base_w;
        double base_h;
        const char* name;
    };

    std::vector<Demo> demos;
    {
        const auto src = makeSource(64, 64);
        const struct { double x, y, w, h; const char* name; } variants[] = {
            {-600, 250, 320, 320, "1:1 square"},
            {0, 250, 640, 320, "2x horizontal"},
            {600, 250, 160, 480, "2x vertical"},
            {0, -250, 900, 200, "5.6x wide stretch"},
        };

        for (const auto& v : variants) {
            auto img = std::make_shared<Item::Image>(
                src, Item::Shape::Rectangle(scene.at(v.x, v.y),
                                            static_cast<Val>(v.w * PX), static_cast<Val>(v.h * PX)));
            demos.push_back({img, v.w, v.h, v.name});
            scene.standard.getObject()->addItem(img);
        }
    }

    // 加一个半透明版本（验证 alpha 混合与子项叠加）。
    {
        auto ghost = std::make_shared<Item::Image>(
            makeSource(64, 64),
            Item::Shape::Rectangle(scene.at(-600, -250), static_cast<Val>(320 * PX), static_cast<Val>(320 * PX)));
        scene.standard.getObject()->addItem(ghost);
    }

    std::thread fps_thread(fps_ptf);
    std::cout << "Space: toggle animated stretch / Esc: quit" << std::endl;

    bool animate = true;
    double t = 0.0;
    bool dumped = false;

    while (!is_done.load(std::memory_order_relaxed)) {
        SDL_Event ev{};
        while (SDL_PollEvent(&ev) != 0) {
            if (ev.type == SDL_EVENT_QUIT) {
                is_done.store(true);
            } else if (ev.type == SDL_EVENT_KEY_DOWN && !ev.key.repeat) {
                if (ev.key.key == SDLK_ESCAPE)
                    is_done.store(true);
                else if (ev.key.key == SDLK_SPACE) {
                    animate = !animate;
                    std::cout << "\nmode: " << (animate ? "animated stretch" : "static stretch") << std::endl;
                }
            }
        }

        t += 0.016;
        for (auto& d : demos) {
            const double factor = animate ? (1.0 + 0.5 * std::sin(t)) : 1.0;
            d.image->setShape(Item::Shape::Rectangle(
                d.image->getShapeRef().getCenter(),
                static_cast<Val>(d.base_w * factor * PX),
                static_cast<Val>(d.base_h / factor * PX)));
        }

        scene.standard.getTimePointR()->add();
        scene.standard.getScreenR().clear();
        scene.standard.Shoot();

        SDL_BlitSurface(surface, nullptr, window_surface, nullptr);
        SDL_UpdateWindowSurface(window);
        frame_count.fetch_add(1, std::memory_order_relaxed);

        // 无交互环境下的可视验证：XII_IMAGE_DUMP=/path/frame.bmp
        if (!dumped) {
            if (const char* dump_path = std::getenv("XII_IMAGE_DUMP"); dump_path != nullptr) {
                if (SDL_SaveBMP(surface, dump_path))
                    std::cout << "dumped frame to " << dump_path << std::endl;
                else
                    std::cerr << "SDL_SaveBMP failed: " << SDL_GetError() << std::endl;
                dumped = true;
                if (const char* only = std::getenv("XII_IMAGE_DUMP_ONLY"); only != nullptr && std::string(only) == "1")
                    is_done.store(true);
            }
        }
    }

    is_done.store(true, std::memory_order_relaxed);
    fps_thread.join();

    SDL_DestroySurface(surface);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return checks_ok ? 0 : 1;
}
