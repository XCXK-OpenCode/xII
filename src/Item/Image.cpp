//
// Created by qpwser from XCXK on 2026-10-21
//

#include "Image.h"

#include "Camera/Outline.h"
#include "Camera/Standard.h"
#include "Renderer/Args.h"
#include "Renderer/Line.h"
#include "Screen/PointPos.h"

#include <algorithm>
#include <cmath>

namespace xII::Item {
    Image::ScreenRect Image::getScreenRect(Camera::Base& camera) const {
        ScreenRect rect;

        const auto pr = _shape.getPR();
        if (pr.first == nullptr || pr.second == nullptr)
            return rect;

        const auto screen_size = camera.getScreen()->getSize();
        if (screen_size.x <= 0 || screen_size.y <= 0)
            return rect;

        const auto map_point = camera.getPoint();
        const auto map_size = camera.getRange();

        // 目标矩形的两个对角点（场景坐标）-> 屏幕像素坐标。
        const auto a = Screen::Point2PoSize(map_point, map_size, {0, 0}, screen_size, pr.first);
        const auto b = Screen::Point2PoSize(map_point, map_size, {0, 0}, screen_size, pr.second);

        rect.left = static_cast<double>(std::min(a.x, b.x));
        rect.right = static_cast<double>(std::max(a.x, b.x));
        rect.top = static_cast<double>(std::min(a.y, b.y));
        rect.bottom = static_cast<double>(std::max(a.y, b.y));
        rect.valid = rect.width() > 0.0 && rect.height() > 0.0;
        return rect;
    }

    bool Image::isInRange(const Camera::Base& camera) const {
        // 目标矩形就是 _shape，所以直接用它自己的包围盒，
        // 判定方式与 General::isInRange 保持一致（场景坐标下的 AABB 相交）。
        if (_shape.getWidth() <= 0.0f || _shape.getHeight() <= 0.0f)
            return false; // 尺寸为 0：没有像素可画

        const auto pr = _shape.getPR();
        if (pr.first == nullptr || pr.second == nullptr)
            return true;

        const double ax = static_cast<double>(pr.first->getX());
        const double ay = static_cast<double>(pr.first->getY());
        const double bx = static_cast<double>(pr.second->getX());
        const double by = static_cast<double>(pr.second->getY());

        const double item_x0 = std::min(ax, bx);
        const double item_x1 = std::max(ax, bx);
        const double item_y0 = std::min(ay, by);
        const double item_y1 = std::max(ay, by);

        const auto view = camera.getShape();
        const auto view_center = view.getCenter();
        const double view_w = static_cast<double>(view.getWidth());
        const double view_h = static_cast<double>(view.getHeight());
        if (view_center == nullptr || view_w <= 0.0 || view_h <= 0.0)
            return true; // 视口无效时不做剔除

        const double view_x = static_cast<double>(view_center->getX());
        const double view_y = static_cast<double>(view_center->getY());

        // 像素图不会超出目标矩形，只留 1px 折算成场景单位抵消取整误差。
        const auto screen_size = camera.getScreen()->getSize();
        const double scene_per_px_x = (screen_size.x > 0) ? view_w / static_cast<double>(screen_size.x) : 0.0;
        const double scene_per_px_y = (screen_size.y > 0) ? view_h / static_cast<double>(screen_size.y) : 0.0;

        const double view_x0 = view_x - view_w / 2.0 - scene_per_px_x;
        const double view_x1 = view_x + view_w / 2.0 + scene_per_px_x;
        const double view_y0 = view_y - view_h / 2.0 - scene_per_px_y;
        const double view_y1 = view_y + view_h / 2.0 + scene_per_px_y;

        return !(item_x1 < view_x0 || item_x0 > view_x1 ||
                 item_y1 < view_y0 || item_y0 > view_y1);
    }

    void Image::Render(Camera::Standard& camera, Item::Renderer::Args args) const {
        auto dest_screen = camera.getScreen();
        const auto screen_size = dest_screen->getSize();

        const auto src_size = _screen->getSize();
        const ScreenRect rect = getScreenRect(camera);

        // 源图或目标矩形无效时什么都不画（空图不是错误）。
        if (rect.valid && src_size.x > 0 && src_size.y > 0 &&
            screen_size.x > 0 && screen_size.y > 0) {
            const auto* src_data = _screen->getData();
            auto* dest_data = dest_screen->getData();

            // 与屏幕求交：只遍历真正可见的像素。
            const int64_t x0 = std::max<int64_t>(0, static_cast<int64_t>(std::ceil(rect.left)));
            const int64_t x1 = std::min<int64_t>(screen_size.x, static_cast<int64_t>(std::ceil(rect.right)));
            const int64_t y0 = std::max<int64_t>(0, static_cast<int64_t>(std::ceil(rect.top)));
            const int64_t y1 = std::min<int64_t>(screen_size.y, static_cast<int64_t>(std::ceil(rect.bottom)));

            const double dest_w = rect.width();
            const double dest_h = rect.height();

            // Renderer::Args 的 alpha（默认 255）与源像素自身的 A 相乘。
            const uint32_t args_alpha = args.getAlpha();

            if (args_alpha != 0 && x1 > x0 && y1 > y0) {
                // 采样下标用 16.16 定点增量推进，避免每像素两次浮点除法：
                //   sx = floor((x + 0.5 - left) * src_w / dest_w)
                // 累加误差 < 0.02 源像素（1280 步 × 0.5/65536），对整数下标无影响。
                const double scale_x = static_cast<double>(src_size.x) / dest_w;
                const double scale_y = static_cast<double>(src_size.y) / dest_h;

                int64_t step_x = static_cast<int64_t>(std::llround(scale_x * 65536.0));
                int64_t step_y = static_cast<int64_t>(std::llround(scale_y * 65536.0));
                if (step_x <= 0) step_x = 1; // 极端缩小：保证至少前进 1 个 1/65536
                if (step_y <= 0) step_y = 1;

                const double start_u = (static_cast<double>(x0) + 0.5 - rect.left) * scale_x;
                const double start_v = (static_cast<double>(y0) + 0.5 - rect.top) * scale_y;

                int64_t acc_y = static_cast<int64_t>(std::llround(start_v * 65536.0));

                const int64_t src_w = src_size.x;
                const int64_t src_h = src_size.y;

                // 不透明源 + 不透明 args 时直接写入，跳过混合。
                const bool opaque = (args_alpha == 255);

                for (int64_t y = y0; y < y1; ++y, acc_y += step_y) {
                    int64_t sy = acc_y >> 16;
                    sy = std::clamp<int64_t>(sy, 0, src_h - 1);

                    const Screen::Screen::Pixel* src_row =
                        src_data + static_cast<size_t>(sy) * static_cast<size_t>(src_w);
                    auto* dst_row = dest_data + static_cast<size_t>(y) * static_cast<size_t>(screen_size.x);

                    int64_t acc_x = static_cast<int64_t>(std::llround(start_u * 65536.0));

                    if (opaque) {
                        for (int64_t x = x0; x < x1; ++x, acc_x += step_x) {
                            int64_t sx = acc_x >> 16;
                            sx = std::clamp<int64_t>(sx, 0, src_w - 1);

                            const auto& src = src_row[static_cast<size_t>(sx)];
                            if (src.A == 255) {
                                dst_row[static_cast<size_t>(x)] = src;
                            } else if (src.A != 0) {
                                dst_row[static_cast<size_t>(x)] =
                                    Screen::Screen::blend_alpha_pixel(src, dst_row[static_cast<size_t>(x)]);
                            }
                        }
                    } else {
                        for (int64_t x = x0; x < x1; ++x, acc_x += step_x) {
                            int64_t sx = acc_x >> 16;
                            sx = std::clamp<int64_t>(sx, 0, src_w - 1);

                            const auto& src = src_row[static_cast<size_t>(sx)];
                            const uint32_t alpha = static_cast<uint32_t>(
                                (static_cast<uint32_t>(src.A) * args_alpha + 127u) / 255u);
                            if (alpha == 0)
                                continue;

                            const Screen::Screen::Pixel color{
                                static_cast<uint8_t>(alpha), src.R, src.G, src.B};

                            if (alpha == 255) {
                                dst_row[static_cast<size_t>(x)] = color;
                            } else {
                                dst_row[static_cast<size_t>(x)] =
                                    Screen::Screen::blend_alpha_pixel(color, dst_row[static_cast<size_t>(x)]);
                            }
                        }
                    }
                }
            }
        }

        // 子 Item 照常递归（与 General 的行为一致）。
        for (const auto& subitem : _subitems)
            if (subitem->isInRange(camera))
                subitem->Render(camera, args);
    }

    void Image::Render(Camera::Outline& camera) const {
        Screen::PoSize screen_point = {0, 0};
        auto screen_size = camera.getScreen()->getSize();
        auto map_point = camera.getPoint();
        auto map_size = camera.getRange();

        // Outline 相机不画像素，只画目标矩形的边框（与 General 一致）。
        auto points = _shape.getPoints(0);
        for (auto ItA = points.begin(); ItA != points.end(); ++ItA)
            for (auto ItB = ItA; ItB != points.end(); ++ItB)
                Renderer::Line::draw_line(*camera.getScreen(),
                                          Screen::Point2PoSize(map_point, map_size, screen_point, screen_size, *ItA),
                                          Screen::Point2PoSize(map_point, map_size, screen_point, screen_size, *ItB),
                                          camera.getColor,
                                          camera.getWidth(),
                                          {0, 0},
                                          camera.getScreen()->getSize());

        for (const auto& subitem : _subitems)
            if (subitem->isInRange(camera))
                subitem->Render(camera);
    }
} // namespace xII::Item
