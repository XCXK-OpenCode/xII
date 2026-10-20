// 
// Created by qpwser from XCXK on 2026-10-20
// Powered by DeepSeek V4.1 Flash
//
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "Camera/Standard.h"
#include "Camera/Outline.h"
#include "Item/Base.h"
#include "Item/Text.h"
#include "Item/Shape/Rectangle.h"
#include "SDL3/SDL.h"
#include "Screen/Screen.h"
#include "Space/Offset.h"
#include "Space/OffsetPoint.h"
#include "Space/Point.h"
#include "Time/Point.h"
#include "Time/OffsetPoint.h"

const int SCREEN_WIDTH = 1280;
const int SCREEN_HEIGHT = 720;

using namespace xII;

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
            size_t current = frame_count.load(std::memory_order_relaxed);
            double fps = static_cast<double>(current - last_count) * 1000.0 / static_cast<double>(elapsed);

            std::cout << "\rFPS: " << std::fixed << std::setprecision(1) << fps
                      << "   frames: " << current << "        " << std::flush;

            last_count = current;
            last_time = now;
        }
    }
    std::cout << std::endl;
}

std::string findFont(const std::vector<std::string>& candidates) {
    for (const auto& path : candidates)
        if (std::filesystem::exists(path))
            return path;
    return {};
}

size_t countNonBlack(const Screen::Screen& screen) {
    const auto* data = screen.getData();
    const auto size = screen.getSize();
    const size_t total = static_cast<size_t>(size.x) * static_cast<size_t>(size.y);

    size_t count = 0;
    for (size_t i = 0; i < total; ++i)
        if (data[i].R != 0 || data[i].G != 0 || data[i].B != 0)
            ++count;
    return count;
}

struct BBox {
    int64_t x0 = 0;
    int64_t y0 = 0;
    int64_t x1 = -1;
    int64_t y1 = -1;
    size_t count = 0;
};

BBox nonBlackBBox(const Screen::Screen& screen, const int64_t rx0, const int64_t ry0,
                  const int64_t rx1, const int64_t ry1) {
    BBox box;
    const auto* data = screen.getData();
    const auto size = screen.getSize();

    for (int64_t y = std::max<int64_t>(0, ry0); y < std::min<int64_t>(size.y, ry1); ++y) {
        for (int64_t x = std::max<int64_t>(0, rx0); x < std::min<int64_t>(size.x, rx1); ++x) {
            const auto& pixel = data[static_cast<size_t>(y) * static_cast<size_t>(size.x) + static_cast<size_t>(x)];
            if (pixel.R == 0 && pixel.G == 0 && pixel.B == 0)
                continue;

            if (box.count == 0) {
                box.x0 = box.x1 = x;
                box.y0 = box.y1 = y;
            } else {
                box.x0 = std::min(box.x0, x);
                box.x1 = std::max(box.x1, x);
                box.y0 = std::min(box.y0, y);
                box.y1 = std::max(box.y1, y);
            }
            ++box.count;
        }
    }
    return box;
}

// 用离屏相机渲染单个 Text，返回整帧拷贝（用于逐像素对比）。
Screen::Screen renderFrame(const std::shared_ptr<Item::Text>& item, const double range_w,
                           const double range_h, const bool outline_mode) {
    auto root = std::make_shared<Space::Point>();
    auto shape = Item::Shape::Rectangle(
        std::make_shared<Space::OffsetPoint>(root, Space::Offset(0, 0)),
        static_cast<xII::Base::Quantity::Base_Value_Type>(range_w),
        static_cast<xII::Base::Quantity::Base_Value_Type>(range_h));
    auto time_point = std::make_shared<Time::OffsetPoint>(std::make_shared<Time::Point>(), Time::Offset(0));

    if (outline_mode) {
        // width = 1：保留 Outline 相机自己的边框线（测试中用空文本帧相减排除），
        // 同时字号轮廓也能正常绘制。
        auto camera = Camera::Outline(shape, {SCREEN_WIDTH, SCREEN_HEIGHT},
                                      std::make_shared<Item::Base>(), time_point,
                                      Screen::Screen::Pixel{255, 255, 255, 255}, 1);
        camera.getObject()->addItem(item);
        camera.getScreenR().clear();
        camera.Shoot();
        return camera.getScreenR();
    }

    auto camera = Camera::Standard(shape, {SCREEN_WIDTH, SCREEN_HEIGHT},
                                   std::make_shared<Item::Base>(), time_point);
    camera.getObject()->addItem(item);
    camera.getScreenR().clear();
    camera.Shoot();
    return camera.getScreenR();
}

bool isInk(const Screen::Screen::Pixel& pixel) {
    return pixel.R != 0 || pixel.G != 0 || pixel.B != 0;
}

size_t countInkInRegion(const Screen::Screen& screen, int64_t x0, int64_t y0, int64_t x1, int64_t y1) {
    const auto* data = screen.getData();
    const auto size = screen.getSize();
    size_t count = 0;

    for (int64_t y = std::max<int64_t>(0, y0); y < std::min<int64_t>(size.y, y1); ++y)
        for (int64_t x = std::max<int64_t>(0, x0); x < std::min<int64_t>(size.x, x1); ++x)
            if (isInk(data[static_cast<size_t>(y) * static_cast<size_t>(size.x) + static_cast<size_t>(x)]))
                ++count;
    return count;
}

bool samePixelsInRegion(const Screen::Screen& a, const Screen::Screen& b,
                        int64_t x0, int64_t y0, int64_t x1, int64_t y1);

// 返回 (差异像素数量, 第一个差异的像素坐标)；无差异时坐标为 (-1,-1)。
std::pair<size_t, Screen::PoSize> diffPixelsInRegion(const Screen::Screen& a, const Screen::Screen& b,
                                                    int64_t x0, int64_t y0, int64_t x1, int64_t y1) {
    const auto* da = a.getData();
    const auto* db = b.getData();
    const auto size = a.getSize();

    size_t diff = 0;
    Screen::PoSize first{-1, -1};

    for (int64_t y = std::max<int64_t>(0, y0); y < std::min<int64_t>(size.y, y1); ++y) {
        for (int64_t x = std::max<int64_t>(0, x0); x < std::min<int64_t>(size.x, x1); ++x) {
            const size_t index = static_cast<size_t>(y) * static_cast<size_t>(size.x) + static_cast<size_t>(x);
            if (da[index].R != db[index].R || da[index].G != db[index].G ||
                da[index].B != db[index].B || da[index].A != db[index].A) {
                if (diff == 0)
                    first = {x, y};
                ++diff;
            }
        }
    }
    return {diff, first};
}

bool samePixelsInRegion(const Screen::Screen& a, const Screen::Screen& b,
                        int64_t x0, int64_t y0, int64_t x1, int64_t y1) {
    return diffPixelsInRegion(a, b, x0, y0, x1, y1).first == 0;
}

int main() {
    std::cout << "Test 20261020-200709: Item::Text (FreeType)" << std::endl;

    const std::string latin_font = findFont({
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/TTF/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
    });
    const std::string cjk_font = findFont({
        "/usr/share/fonts/noto-cjk/NotoSansCJK-Regular.ttc",
        "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc",
        "/usr/share/fonts/noto/NotoSansCJK-Regular.ttc",
    });

    std::cout << "latin font: " << (latin_font.empty() ? "<fallback>" : latin_font) << '\n'
              << "cjk font  : " << (cjk_font.empty() ? "<fallback>" : cjk_font) << std::endl;

    std::thread fps_thread(fps_ptf);

    auto root_space_point = std::make_shared<Space::Point>();
    auto root_time_point = std::make_shared<Time::Point>();

    auto camera_space_point = std::make_shared<Space::OffsetPoint>(root_space_point, Space::Offset(0, 0));

    auto camera = Camera::Standard(
        Item::Shape::Rectangle(camera_space_point, 1920, 1080),
        {SCREEN_WIDTH, SCREEN_HEIGHT},
        std::make_shared<Item::Base>(),
        std::make_shared<Time::OffsetPoint>(root_time_point, Time::Offset(0))
    );

    auto outline_camera = Camera::Outline(
        Item::Shape::Rectangle(camera_space_point, 1920, 1080),
        {SCREEN_WIDTH, SCREEN_HEIGHT},
        std::make_shared<Item::Base>(),
        std::make_shared<Time::OffsetPoint>(root_time_point, Time::Offset(0)),
        Screen::Screen::Pixel{255, 255, 220, 0},
        2
    );

    // 相机的 getPoint() 是 Rectangle 的 P2（左下角 -960,-540），所以
    // 屏幕 (x,y) 对应的场景坐标是 ((x-640)*1.5, (y-360)*1.5)（缩放 2/3）。
    // Rectangle.center 是文本块中心，下面的坐标都是“块中心”在场景中的位置。
    constexpr double SCENE_SCALE = static_cast<double>(SCREEN_HEIGHT) / 1080.0;

    auto makeText = [&](const double scene_x, const double scene_y, std::string content,
                        const std::string& font, const double height,
                        const Screen::Screen::Pixel& color) {
        return std::make_shared<Item::Text>(
            std::make_shared<Space::OffsetPoint>(
                root_space_point,
                Space::Offset(static_cast<xII::Base::Quantity::Base_Value_Type>(scene_x),
                              static_cast<xII::Base::Quantity::Base_Value_Type>(scene_y))),
            std::move(content), font, height, color);
    };

    auto title = makeText(-540, -420, "xII::Item::Text", latin_font, 96,
                          Screen::Screen::Pixel{255, 255, 255, 255});

    auto subtitle = makeText(-360, -315, "height / spacing / width are SCENE units", latin_font, 42,
                             Screen::Screen::Pixel{255, 150, 150, 150});

    auto cjk = makeText(-480, -195, "你好，世界！文字渲染", cjk_font, 78,
                        Screen::Screen::Pixel{255, 80, 230, 230});

    auto float_size = makeText(-600, -90, "Float height 55.5", latin_font, 55.5,
                               Screen::Screen::Pixel{255, 255, 200, 60});

    auto centered = makeText(465, -420, "Center aligned / 居中对齐", cjk_font, 60,
                             Screen::Screen::Pixel{255, 255, 200, 60});
    centered->setAlign(Item::Text::Align::Center);

    auto translucent = makeText(480, -315, "Alpha = 96 (semi-transparent)", latin_font, 60,
                                Screen::Screen::Pixel{96, 80, 220, 255});

    auto multiline = makeText(-585, 60, "Line 1: 第一行\nLine 2: second line\nLine 3: 第三行", cjk_font, 51,
                              Screen::Screen::Pixel{255, 255, 255, 255});

    auto wrapped = makeText(480, -90, "行宽 300 场景单位自动换行 / wrapped at 300 scene units",
                            cjk_font, 40, Screen::Screen::Pixel{255, 120, 220, 120});
    wrapped->setLineWidth(300);

    auto spaced = makeText(510, 165, "行间距 20\nline spacing 20", cjk_font, 40,
                           Screen::Screen::Pixel{255, 220, 220, 120});
    spaced->setLineSpacing(20);

    // 裁剪演示：Rectangle 的 width/height 是文本块总宽/总高（center 是文本块中心），
    // 框外的部分不渲染，跨在边界上的字形只显示框内可见部分。
    auto clipped_demo = makeText(480, 345, "裁剪 / clipped by Rectangle", cjk_font, 44,
                                 Screen::Screen::Pixel{255, 255, 140, 140});
    {
        const Item::Text::Measure m = clipped_demo->measure();
        clipped_demo->setShape(Item::Shape::Rectangle(
            clipped_demo->getPosition(),
            static_cast<float>(m.width * 0.60),
            static_cast<float>(m.height * 0.62)));
    }

    const std::vector<std::shared_ptr<Item::Text>> texts{
        title, subtitle, cjk, float_size, centered, translucent, multiline, wrapped, spaced, clipped_demo};

    for (const auto& text : texts) {
        camera.getObject()->addItem(text);
        outline_camera.getObject()->addItem(text);
    }

    // 输出每个 Text 的实际测量结果，作为“文字确实被加载并可排版”的证据。
    bool metrics_ok = true;
    std::cout << "---- Text metrics (scene units, camera scale = " << SCENE_SCALE << ") ----" << std::endl;
    for (size_t i = 0; i < texts.size(); ++i) {
        const auto& text = texts[i];
        const Item::Text::Measure measure = text->measure();
        const bool ok = text->isValid() && measure.width > 0.0 && measure.height > 0.0;
        metrics_ok = metrics_ok && ok;

        std::cout << "[" << i << "] valid=" << (text->isValid() ? 1 : 0)
                  << " height=" << text->getHeight()
                  << " scene=" << measure.width << "x" << measure.height
                  << " screen=" << measure.width * SCENE_SCALE << "x" << measure.height * SCENE_SCALE
                  << " text=\"" << text->getText() << "\"";
        if (!text->getError().empty())
            std::cout << " error=\"" << text->getError() << "\"";
        std::cout << (ok ? "  PASS" : "  FAIL") << std::endl;
    }
    std::cout << (metrics_ok ? "ALL TEXT METRICS PASS" : "SOME TEXT METRICS FAILED") << std::endl;

    // ---- 功能实测：浮点字号 / 行间距 / 行宽 / 场景单位 -> 屏幕像素 ----
    bool features_ok = true;
    const auto report = [&](const char* name, const bool ok, const std::string& detail) {
        std::cout << "[feature] " << name << ": " << detail << "  " << (ok ? "PASS" : "FAIL") << std::endl;
        features_ok = features_ok && ok;
    };

    {
        // 浮点字号：同内容 40 与 40.5，宽度比应等于字号比。
        auto a = makeText(0, 0, "Available Width", latin_font, 40.0, Screen::Screen::Pixel{255, 255, 255, 255});
        auto b = makeText(0, 0, "Available Width", latin_font, 40.5, Screen::Screen::Pixel{255, 255, 255, 255});
        const double wa = a->measure().width;
        const double wb = b->measure().width;
        const double ratio = (wa > 0.0) ? wb / wa : 0.0;
        const bool ok = std::abs(ratio - 40.5 / 40.0) < 0.005;
        report("float height", ok,
               "width(40)=" + std::to_string(wa) + " width(40.5)=" + std::to_string(wb) +
                   " ratio=" + std::to_string(ratio) + " expected=" + std::to_string(40.5 / 40.0));
    }

    {
        // 行间距：两行文本，额外 20 场景单位 → 总高度增加 2*20。
        auto a = makeText(0, 0, "spacing\nspacing", latin_font, 40, Screen::Screen::Pixel{255, 255, 255, 255});
        auto b = makeText(0, 0, "spacing\nspacing", latin_font, 40, Screen::Screen::Pixel{255, 255, 255, 255});
        b->setLineSpacing(20);
        const double ha = a->measure().height;
        const double hb = b->measure().height;
        const bool ok = std::abs((hb - ha) - 40.0) < 0.01;
        report("line spacing", ok,
               "height(0)=" + std::to_string(ha) + " height(20)=" + std::to_string(hb) +
                   " delta=" + std::to_string(hb - ha) + " expected=40");
    }

    {
        // 行宽：限定 400 场景单位后必须换行，且行宽不超过 400。
        const std::string long_text = "行宽自动换行测试 with a fairly long English sentence to wrap";
        auto a = makeText(0, 0, long_text, cjk_font, 40, Screen::Screen::Pixel{255, 255, 255, 255});
        auto b = makeText(0, 0, long_text, cjk_font, 40, Screen::Screen::Pixel{255, 255, 255, 255});
        b->setLineWidth(400);
        const Item::Text::Measure ma = a->measure();
        const Item::Text::Measure mb = b->measure();
        const bool ok = mb.height > ma.height * 1.5 && mb.width <= 400.5 && mb.width > 100.0;
        report("line width", ok,
               "unwrapped=" + std::to_string(ma.width) + "x" + std::to_string(ma.height) +
                   " wrapped=" + std::to_string(mb.width) + "x" + std::to_string(mb.height) +
                   " limit=400");
    }

    {
        // 场景单位 -> 屏幕像素：同一相机 1920x1080 下，字号 48 与 96 的屏幕尺寸应成 2 倍。
        auto a = makeText(0, 0, "Scene Scale", latin_font, 48, Screen::Screen::Pixel{255, 255, 255, 255});
        auto b = makeText(0, 0, "Scene Scale", latin_font, 96, Screen::Screen::Pixel{255, 255, 255, 255});

        const BBox ba = nonBlackBBox(renderFrame(a, 1920, 1080, false), 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
        const BBox bb = nonBlackBBox(renderFrame(b, 1920, 1080, false), 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

        const double wa = static_cast<double>(ba.x1 - ba.x0 + 1);
        const double wb = static_cast<double>(bb.x1 - bb.x0 + 1);
        const double ha = static_cast<double>(ba.y1 - ba.y0 + 1);
        const double hb = static_cast<double>(bb.y1 - bb.y0 + 1);

        const bool ok = wa > 0.0 && std::abs(wb / wa - 2.0) < 0.06 && std::abs(hb / ha - 2.0) < 0.15;
        report("scene units scale", ok,
               "ink(48)=" + std::to_string(wa) + "x" + std::to_string(ha) +
                   " ink(96)=" + std::to_string(wb) + "x" + std::to_string(hb) +
                   " ratio=" + std::to_string(wb / wa) + "/" + std::to_string(hb / ha) + " expected≈2");
    }

    {
        // 场景单位绝对值：height=96、range 1920 → 屏幕 EM = 96 * 2/3 = 64 像素；
        // 大写 "H" 的墨迹高度约等于 cap height（≈ 0.72 EM）。
        auto a = makeText(0, 0, "H", latin_font, 96, Screen::Screen::Pixel{255, 255, 255, 255});
        const Item::Text::Measure m = a->measure();
        const BBox box = nonBlackBBox(renderFrame(a, 1920, 1080, false), 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
        const double ink_h = static_cast<double>(box.y1 - box.y0 + 1);
        const double em_px = 96.0 * SCENE_SCALE;
        const bool ok = ink_h > em_px * 0.60 && ink_h < em_px * 0.80;
        report("scene -> screen", ok,
               "measure_scene=" + std::to_string(m.height) + " em_px=" + std::to_string(em_px) +
                   " ink_px=" + std::to_string(ink_h) + " (cap≈0.72 EM)");
    }

    {
        // ---- Item::Shape::Rectangle：center = 文本块中心，width/height = 总宽/总高（场景单位）----
        constexpr double RANGE_W = 1920.0;
        constexpr double RANGE_H = 1080.0;
        constexpr double SCALE = static_cast<double>(SCREEN_HEIGHT) / RANGE_H;
        // Camera::Base::getPoint() 是 Rectangle 的 P2（左下角），所以场景->屏幕要减掉它。
        constexpr double MAP_X = -RANGE_W / 2.0;
        constexpr double MAP_Y = -RANGE_H / 2.0;
        // 测试用文本块中心（场景坐标）与字体高度。
        constexpr double TEXT_X = 200.0;
        constexpr double TEXT_Y = 100.0;
        constexpr double TEXT_HEIGHT = 80.0;

        auto baseline_item = makeText(TEXT_X, TEXT_Y, "Clipping Test gpq 裁剪", latin_font, TEXT_HEIGHT,
                                      Screen::Screen::Pixel{255, 255, 255, 255});
        const Item::Text::Measure content = baseline_item->measure();
        const double center_px_x = (TEXT_X - MAP_X) * SCALE;
        const double center_px_y = (TEXT_Y - MAP_Y) * SCALE;

        {
            // 传入的 Rectangle 应当原样保留（center / width / height）。
            const auto probe_center = std::make_shared<Space::OffsetPoint>(root_space_point, Space::Offset(12, 34));
            auto probe = makeText(TEXT_X, TEXT_Y, "probe", latin_font, 40,
                                  Screen::Screen::Pixel{255, 255, 255, 255});            probe->setShape(Item::Shape::Rectangle(probe_center, 500.0f, 400.0f));
            const Item::Shape::Rectangle& kept = probe->getShape();
            const bool ok = kept.getCenter() == probe_center &&
                            std::abs(static_cast<double>(kept.getWidth()) - 500.0) < 0.01 &&
                            std::abs(static_cast<double>(kept.getHeight()) - 400.0) < 0.01;
            report("shape roundtrip", ok,
                   "center=(" + std::to_string(kept.getCenter()->getX()) + "," +
                       std::to_string(kept.getCenter()->getY()) + ") size=" +
                       std::to_string(kept.getWidth()) + "x" + std::to_string(kept.getHeight()));
        }

        {
            // 自适应（width=height=0）：墨迹中心应落在 Rectangle.center 附近。
            const BBox box = nonBlackBBox(renderFrame(baseline_item, RANGE_W, RANGE_H, false),
                                          0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
            const double ink_cx = (static_cast<double>(box.x0) + static_cast<double>(box.x1)) / 2.0;
            const double ink_cy = (static_cast<double>(box.y0) + static_cast<double>(box.y1)) / 2.0;
            const bool ok = box.count > 0 && std::abs(ink_cx - center_px_x) < 4.0 &&
                            std::abs(ink_cy - center_px_y) < 12.0;
            report("shape center", ok,
                   "center_px=(" + std::to_string(center_px_x) + "," + std::to_string(center_px_y) +
                       ") ink_center=(" + std::to_string(ink_cx) + "," + std::to_string(ink_cy) + ")");
        }

        {
            // 框尺寸 = 内容尺寸时，文本块位置与裁剪都与自适应完全一致 → 逐像素相同。
            auto exact_item = makeText(TEXT_X, TEXT_Y, "Clipping Test gpq 裁剪", latin_font, TEXT_HEIGHT,
                                       Screen::Screen::Pixel{255, 255, 255, 255});
            exact_item->setShape(Item::Shape::Rectangle(
                exact_item->getPosition(),
                static_cast<float>(content.width),
                static_cast<float>(content.height)));

            const Screen::Screen base = renderFrame(baseline_item, RANGE_W, RANGE_H, false);
            const Screen::Screen exact = renderFrame(exact_item, RANGE_W, RANGE_H, false);
            const auto diff = diffPixelsInRegion(base, exact, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
            report("shape == content", diff.first == 0,
                   "diff_pixels=" + std::to_string(diff.first) + " (期望 0)");
        }

        // ---- 宽度裁剪：框宽 = 内容宽度的一半，右半部分不渲染 ----
        {
            auto clipped_item = makeText(TEXT_X, TEXT_Y, "Clipping Test gpq 裁剪", latin_font, TEXT_HEIGHT,
                                         Screen::Screen::Pixel{255, 255, 255, 255});
            const double box_w = content.width * 0.5;
            clipped_item->setShape(Item::Shape::Rectangle(
                clipped_item->getPosition(), static_cast<float>(box_w), 0.0f));

            const Screen::Screen clipped = renderFrame(clipped_item, RANGE_W, RANGE_H, false);

            const double left_px = center_px_x - box_w * SCALE / 2.0;
            const double right_px = center_px_x + box_w * SCALE / 2.0;
            const int64_t left = static_cast<int64_t>(std::ceil(left_px));
            const int64_t right = static_cast<int64_t>(std::floor(right_px));

            const size_t outside_right = countInkInRegion(clipped, right + 1, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
            const size_t outside_left = countInkInRegion(clipped, 0, 0, left, SCREEN_HEIGHT);
            const size_t inside_ink = countInkInRegion(clipped, left, 0, right + 1, SCREEN_HEIGHT);
            const size_t near_left_ink = countInkInRegion(clipped, left, 0, left + 4, SCREEN_HEIGHT);

            const bool ok = outside_right == 0 && outside_left == 0 && inside_ink > 0 && near_left_ink > 0;
            report("shape clip width", ok,
                   "rect_x=[" + std::to_string(left) + ", " + std::to_string(right) + "]" +
                       " ink_outside_left=" + std::to_string(outside_left) +
                       " ink_outside_right=" + std::to_string(outside_right) +
                       " inside_ink=" + std::to_string(inside_ink) +
                       " near_left_ink=" + std::to_string(near_left_ink));
        }

        // ---- 高度裁剪：框高 = 内容高度的一半，下半部分不渲染 ----
        {
            auto clipped_h = makeText(TEXT_X, TEXT_Y, "Clipping Test gpq 裁剪", latin_font, TEXT_HEIGHT,
                                      Screen::Screen::Pixel{255, 255, 255, 255});
            const double box_h = content.height * 0.5;
            clipped_h->setShape(Item::Shape::Rectangle(
                clipped_h->getPosition(), 0.0f, static_cast<float>(box_h)));

            const Screen::Screen clipped = renderFrame(clipped_h, RANGE_W, RANGE_H, false);

            const double top_px = center_px_y - box_h * SCALE / 2.0;
            const double bottom_px = center_px_y + box_h * SCALE / 2.0;
            const int64_t top = static_cast<int64_t>(std::ceil(top_px));
            const int64_t bottom = static_cast<int64_t>(std::floor(bottom_px));

            const size_t outside_above = countInkInRegion(clipped, 0, 0, SCREEN_WIDTH, top);
            const size_t outside_below = countInkInRegion(clipped, 0, bottom + 1, SCREEN_WIDTH, SCREEN_HEIGHT);
            const size_t inside_ink = countInkInRegion(clipped, 0, top, SCREEN_WIDTH, bottom + 1);

            const bool ok = outside_above == 0 && outside_below == 0 && inside_ink > 0;
            report("shape clip height", ok,
                   "rect_y=[" + std::to_string(top) + ", " + std::to_string(bottom) + "]" +
                       " ink_outside_above=" + std::to_string(outside_above) +
                       " ink_outside_below=" + std::to_string(outside_below) +
                       " inside_ink=" + std::to_string(inside_ink));
        }

        // ---- Outline 相机同样遵守裁剪（用空文本得到纯边框线帧作为对照）----
        {
            auto border_only = makeText(TEXT_X, TEXT_Y, "", latin_font, TEXT_HEIGHT,
                                        Screen::Screen::Pixel{255, 255, 255, 255});
            auto clipped_o = makeText(TEXT_X, TEXT_Y, "Clipping Test gpq 裁剪", latin_font, TEXT_HEIGHT,
                                      Screen::Screen::Pixel{255, 255, 255, 255});

            const Screen::Screen border = renderFrame(border_only, RANGE_W, RANGE_H, true);
            const Screen::Screen base = renderFrame(baseline_item, RANGE_W, RANGE_H, true);

            // 正文墨迹列范围 = base 与 border 的差异列（排除 Outline 相机自己画的边框线）
            int64_t glyph_x0 = -1;
            int64_t glyph_x1 = -1;
            for (int64_t x = 0; x < SCREEN_WIDTH; ++x) {
                const size_t a = countInkInRegion(base, x, 0, x + 1, SCREEN_HEIGHT);
                const size_t b = countInkInRegion(border, x, 0, x + 1, SCREEN_HEIGHT);
                if (a != b) {
                    if (glyph_x0 < 0)
                        glyph_x0 = x;
                    glyph_x1 = x;
                }
            }

            int64_t cut = -1;
            size_t base_outside = 0;
            size_t clipped_outside = 0;
            size_t clipped_inside = 0;

            if (glyph_x0 >= 0 && glyph_x1 > glyph_x0) {
                cut = glyph_x0 + (glyph_x1 - glyph_x0) / 2;

                // 构造一个包围 "墨迹左端 .. cut" 的矩形（场景单位）：
                // 这样右侧超出 cut 的轮廓会被裁掉，左侧保持完整。
                const double left = static_cast<double>(glyph_x0) - 4.0;
                const double right = static_cast<double>(cut);
                const double width_px = right - left;
                const double center_px = (left + right) / 2.0;

                const double scale_x = static_cast<double>(SCREEN_WIDTH) / RANGE_W;
                const double scene_cx = center_px / scale_x + MAP_X;
                const double scene_cy = center_px_y / SCALE + MAP_Y;

                auto clip_center = std::make_shared<Space::OffsetPoint>(
                    root_space_point,
                    Space::Offset(static_cast<xII::Base::Quantity::Base_Value_Type>(scene_cx),
                                  static_cast<xII::Base::Quantity::Base_Value_Type>(scene_cy)));

                clipped_o->setShape(Item::Shape::Rectangle(
                    clip_center, static_cast<float>(width_px / scale_x), 0.0f));

                const Screen::Screen clipped = renderFrame(clipped_o, RANGE_W, RANGE_H, true);

                const size_t cut_safe = static_cast<size_t>(cut + 2);
                base_outside = countInkInRegion(base, static_cast<int64_t>(cut_safe), 0, SCREEN_WIDTH, SCREEN_HEIGHT) -
                               countInkInRegion(border, static_cast<int64_t>(cut_safe), 0, SCREEN_WIDTH, SCREEN_HEIGHT);
                clipped_outside = countInkInRegion(clipped, static_cast<int64_t>(cut_safe), 0, SCREEN_WIDTH, SCREEN_HEIGHT) -
                                  countInkInRegion(border, static_cast<int64_t>(cut_safe), 0, SCREEN_WIDTH, SCREEN_HEIGHT);
                clipped_inside = countInkInRegion(clipped, 0, 0, cut - 2, SCREEN_HEIGHT);
            }

            const bool ok = cut > 0 && clipped_outside == 0 && base_outside > 0 && clipped_inside > 0;
            report("shape clip outline", ok,
                   "glyph_cols=[" + std::to_string(glyph_x0) + ", " + std::to_string(glyph_x1) + "]" +
                       " cut_x=" + std::to_string(cut) +
                       " base_glyph_ink_outside=" + std::to_string(base_outside) +
                       " clipped_glyph_ink_outside=" + std::to_string(clipped_outside) +
                       " inside_ink=" + std::to_string(clipped_inside));
        }
    }

    {
        // ---- isInRange：视口剔除（Item::Base/General 只在 true 时调用子 Item 的 Render）----
        constexpr double RANGE_W = 1920.0;
        constexpr double RANGE_H = 1080.0;
        // 视口 = [-960, 960] x [-540, 540]（相机 Rectangle 以 center 为中心）
        constexpr double VIEW_HALF_W = RANGE_W / 2.0;

        auto camera_root = std::make_shared<Space::Point>();
        auto probe_camera = Camera::Standard(
            Item::Shape::Rectangle(std::make_shared<Space::OffsetPoint>(camera_root, Space::Offset(0, 0)),
                                   static_cast<xII::Base::Quantity::Base_Value_Type>(RANGE_W),
                                   static_cast<xII::Base::Quantity::Base_Value_Type>(RANGE_H)),
            {SCREEN_WIDTH, SCREEN_HEIGHT},
            std::make_shared<Item::Base>(),
            std::make_shared<Time::OffsetPoint>(std::make_shared<Time::Point>(), Time::Offset(0)));

        auto at = [&](const double x, const double y, std::string content, const double h) {
            return std::make_shared<Item::Text>(
                std::make_shared<Space::OffsetPoint>(
                    camera_root,
                    Space::Offset(static_cast<xII::Base::Quantity::Base_Value_Type>(x),
                                  static_cast<xII::Base::Quantity::Base_Value_Type>(y))),
                std::move(content), latin_font, h, Screen::Screen::Pixel{255, 255, 255, 255});
        };

        constexpr double H = 80.0;

        // 1) 完全在视口内
        auto inside = at(0, 0, "In Range", H);
        // 2) 完全离开视口（右侧很远）
        auto outside = at(5000, 0, "Outside", H);
        // 3) 块中心在视口外，但左半边还在视口内（跨边界，必须继续渲染）
        auto straddle = at(0, 0, "Straddle Edge", H);
        const double block_w = straddle->measure().width;
        straddle->setPosition(std::make_shared<Space::OffsetPoint>(
            camera_root,
            Space::Offset(
                static_cast<xII::Base::Quantity::Base_Value_Type>(VIEW_HALF_W + block_w / 2.0 - 10.0),
                static_cast<xII::Base::Quantity::Base_Value_Type>(0))));

        // 4) 刚离开视口（超过 1 EM 余量 + 余裕），应被剔除
        auto just_outside = at(0, 0, "Just Outside", H);
        const double jw = just_outside->measure().width;
        just_outside->setPosition(std::make_shared<Space::OffsetPoint>(
            camera_root,
            Space::Offset(
                static_cast<xII::Base::Quantity::Base_Value_Type>(VIEW_HALF_W + jw / 2.0 + H + 50.0),
                static_cast<xII::Base::Quantity::Base_Value_Type>(0))));

        // 5) 空文本：没有可画的东西
        auto empty = at(0, 0, "", H);

        const bool in_1 = inside->isInRange(probe_camera);
        const bool in_2 = outside->isInRange(probe_camera);
        const bool in_3 = straddle->isInRange(probe_camera);
        const bool in_4 = just_outside->isInRange(probe_camera);
        const bool in_5 = empty->isInRange(probe_camera);

        // 端到端：被剔除的 Item 放进相机后应为 0 墨迹；跨边界的应 > 0。
        const BBox box_outside = nonBlackBBox(renderFrame(outside, RANGE_W, RANGE_H, false),
                                              0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
        const BBox box_straddle = nonBlackBBox(renderFrame(straddle, RANGE_W, RANGE_H, false),
                                               0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

        const bool ok = in_1 && !in_2 && in_3 && !in_4 && !in_5 &&
                        box_outside.count == 0 && box_straddle.count > 0;
        report("isInRange", ok,
               "inside=" + std::to_string(in_1 ? 1 : 0) +
                   " outside=" + std::to_string(in_2 ? 1 : 0) +
                   " straddle=" + std::to_string(in_3 ? 1 : 0) +
                   " just_outside=" + std::to_string(in_4 ? 1 : 0) +
                   " empty=" + std::to_string(in_5 ? 1 : 0) +
                   " ink(outside)=" + std::to_string(box_outside.count) +
                   " ink(straddle)=" + std::to_string(box_straddle.count));
    }

    std::cout << ((metrics_ok && features_ok) ? "ALL CHECKS PASS" : "SOME CHECKS FAILED") << std::endl;

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        is_done.store(true);
        fps_thread.join();
        return -1;
    }

    SDL_Window* window = SDL_CreateWindow("xII Text Test [Tab: outline]", SCREEN_WIDTH, SCREEN_HEIGHT, 0);
    if (!window) {
        std::cerr << "can not create window: " << SDL_GetError() << std::endl;
        is_done.store(true);
        fps_thread.join();
        SDL_Quit();
        return -1;
    }

    SDL_Surface* window_surface = SDL_GetWindowSurface(window);
    if (!window_surface) {
        is_done.store(true);
        fps_thread.join();
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    auto makeSurface = [](Screen::Screen& screen, const char* label) -> SDL_Surface* {
        SDL_Surface* surface = SDL_CreateSurfaceFrom(
            static_cast<int>(screen.getSize().x),
            static_cast<int>(screen.getSize().y),
            SDL_PIXELFORMAT_BGRA8888,
            screen.getData(),
            static_cast<int>(screen.getSize().x * 4));
        if (!surface) {
            std::cerr << label << ": CreateSurfaceFrom failed: " << SDL_GetError() << std::endl;
            return nullptr;
        }
        if (!SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_NONE)) {
            std::cerr << label << ": SetSurfaceBlendMode failed: " << SDL_GetError() << std::endl;
        }
        return surface;
    };

    SDL_Surface* surface_cpu = makeSurface(camera.getScreenR(), "cpu");
    SDL_Surface* surface_outline = makeSurface(outline_camera.getScreenR(), "outline");
    if (!surface_cpu || !surface_outline) {
        is_done.store(true);
        fps_thread.join();
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    bool outline_mode = false;
    bool first_frame_checked = false;
    bool dumped = false;

    if (const char* env = std::getenv("XII_TEXT_OUTLINE"); env != nullptr && std::string(env) == "1") {
        outline_mode = true;
        SDL_SetWindowTitle(window, "xII Text Test [OUTLINE - Tab: standard]");
    }

    while (!is_done.load(std::memory_order_relaxed)) {
        SDL_Event event{};
        while (SDL_PollEvent(&event) != 0) {
            if (event.type == SDL_EVENT_QUIT) {
                is_done.store(true);
            } else if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat) {
                if (event.key.key == SDLK_ESCAPE) {
                    is_done.store(true);
                } else if (event.key.key == SDLK_TAB) {
                    outline_mode = !outline_mode;
                    SDL_SetWindowTitle(window, outline_mode
                        ? "xII Text Test [OUTLINE - Tab: standard]"
                        : "xII Text Test [Tab: outline]");
                    std::cout << "\nmode: " << (outline_mode ? "outline" : "standard") << std::endl;
                }
            }
        }

        Camera::Base* active = outline_mode
            ? static_cast<Camera::Base*>(&outline_camera)
            : static_cast<Camera::Base*>(&camera);

        active->getTimePointR()->add();
        active->getScreenR().clear();
        active->Shoot();

        if (!first_frame_checked) {
            const size_t non_black = countNonBlack(active->getScreenR());
            std::cout << "first " << (outline_mode ? "outline" : "standard")
                      << " frame: non-black pixels = " << non_black
                      << " (expected > 0)  " << (non_black > 0 ? "PASS" : "FAIL") << std::endl;

            if (!outline_mode) {
                // 多行文本：离屏渲染后，墨迹高度应接近“行数 * 行距”减去首行空白。
                // 下界取 0.7 倍（首行 ascent 区域与末行 descender 不计墨迹），
                // 上界取 1.0 倍（不可能超过排版高度）。
                const Item::Text::Measure multiline_measure = multiline->measure();
                const double expected_h = multiline_measure.height * SCENE_SCALE;
                const BBox multiline_box = nonBlackBBox(renderFrame(multiline, 1920, 1080, false),
                                                       0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
                const double ink_h = static_cast<double>(multiline_box.y1 - multiline_box.y0 + 1);
                const bool height_ok = multiline_box.count > 0 &&
                                       ink_h > expected_h * 0.70 && ink_h <= expected_h;
                std::cout << "multiline ink height: actual=" << ink_h
                          << " expected≈" << expected_h << " (" << multiline_measure.height
                          << " scene * " << SCENE_SCALE << ")  "
                          << (height_ok ? "PASS" : "FAIL") << std::endl;
            }
            first_frame_checked = true;
        }

        // 无交互环境下的可视验证：XII_TEXT_DUMP=/path/frame.bmp
        // XII_TEXT_DUMP_ONLY=1 时保存后立即退出。
        if (!dumped) {
            if (const char* dump_path = std::getenv("XII_TEXT_DUMP"); dump_path != nullptr) {
                SDL_Surface* dump_surface = outline_mode ? surface_outline : surface_cpu;
                if (SDL_SaveBMP(dump_surface, dump_path)) {
                    std::cout << "dumped " << (outline_mode ? "outline" : "standard")
                              << " frame to " << dump_path << std::endl;
                } else {
                    std::cerr << "SDL_SaveBMP failed: " << SDL_GetError() << std::endl;
                }
                dumped = true;
                if (const char* only = std::getenv("XII_TEXT_DUMP_ONLY"); only != nullptr && std::string(only) == "1")
                    is_done.store(true);
            }
        }

        SDL_BlitSurface(outline_mode ? surface_outline : surface_cpu, nullptr, window_surface, nullptr);
        SDL_UpdateWindowSurface(window);
        frame_count.fetch_add(1, std::memory_order_relaxed);
    }

    is_done.store(true, std::memory_order_relaxed);
    fps_thread.join();

    SDL_DestroySurface(surface_cpu);
    SDL_DestroySurface(surface_outline);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return metrics_ok ? 0 : 1;
}
