#include "Line.h"
#include "Screen/PoSize.h"
#include "Screen/PointPos.h"

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <algorithm>
#include <functional>

namespace xII::Item::Renderer {
        void Line::draw_line(Screen::RGBA8888& screen,
                const Screen::PoSize& A,
                const Screen::PoSize& B,
                const std::function<Screen::RGBA8888::Pixel(const Args::DrawContext&)>& getColor,
                size_t width,
                const Screen::PoSize& item_pos,
                const Screen::PoSize& item_size,
                std::vector<uint8_t>* coverage)
    {
        if (width == 0) return;

        const size_t W = screen.getSize().x;
        const size_t H = screen.getSize().y;
        Screen::RGBA8888::Pixel* data = screen.getData();

        int64_t x0 = A.x, y0 = A.y;
        int64_t x1 = B.x, y1 = B.y;
        int64_t dx = std::llabs(x1 - x0), dy = std::llabs(y1 - y0);
        int64_t sx = (x0 < x1) ? 1 : -1;
        int64_t sy = (y0 < y1) ? 1 : -1;

        const int64_t half = static_cast<int64_t>(width) / 2;
        const int64_t lo = -half;
        const int64_t hi = static_cast<int64_t>(width) - 1 - half;

        const bool horizontal = dx >= dy;

        const Screen::PoSize screen_size = screen.getSize();

        auto plot = [&](int64_t px, int64_t py) {
            if (px < 0 || py < 0) return;
            const auto ux = static_cast<size_t>(px);
            const auto uy = static_cast<size_t>(py);
            if (ux >= W || uy >= H) return;

            const size_t index = uy * W + ux;
            if (coverage != nullptr) {
                if ((*coverage)[index] != 0) return;
                (*coverage)[index] = 1;
            }

            const Args::DrawContext ctx{
                Screen::PoSize{ px, py },
                screen_size,
                item_pos,
                item_size
            };

            const Screen::RGBA8888::Pixel color = getColor(ctx);

            auto& dst = data[index];
            if (color.A == 255) {
                dst = color;
            } else if (color.A != 0) {
                dst = Screen::RGBA8888::blend_alpha_pixel(color, dst);
            }
        };

        for (int64_t off = lo; off <= hi; ++off) {
            int64_t cx0 = x0, cy0 = y0;
            int64_t cx1 = x1, cy1 = y1;
            if (horizontal) { cy0 += off; cy1 += off; }
            else            { cx0 += off; cx1 += off; }

            int64_t err = dx - dy;
            int64_t cx = cx0, cy = cy0;
            const int64_t ex = cx1, ey = cy1;

            while (true) {
                plot(cx, cy);
                if (cx == ex && cy == ey) break;
                int64_t e2 = 2 * err;
                if (e2 > -dy) { err -= dy; cx += sx; }
                if (e2 < dx)  { err += dx; cy += sy; }
            }
        }
    }


    void Line::Render(Camera::Base& camera, std::vector<std::shared_ptr<Space::Point>>& points, Args &args, const Screen::PoSize& item_pos, const Screen::PoSize& item_size) const {
        Screen::PoSize screen_point = {0, 0};
        auto screen_size = camera.getScreen()->getSize();
        auto map_point = camera.getPoint();
        auto map_size = camera.getRange();

        // 半透明绘制时，同一条折线的线段之间、以及线宽的多个偏移之间会覆盖同一像素。
        // 如果没有掩码，每覆盖一次就会再做一次 alpha 混合，颜色会越叠越亮。
        std::vector<uint8_t> coverage;
        std::vector<uint8_t>* coverage_ptr = nullptr;
        if (args.getAlpha() < 255) {
            coverage.assign(static_cast<size_t>(screen_size.x) * static_cast<size_t>(screen_size.y), 0);
            coverage_ptr = &coverage;
        }

        const size_t n = points.size();
        for (size_t i = 0; i < n; i++) {
            auto a = Screen::Point2PoSize(map_point, map_size, screen_point, screen_size, points[i]);
            auto b = Screen::Point2PoSize(map_point, map_size, screen_point, screen_size, points[(i + 1) % n]);
            draw_line(*camera.getScreen(), a, b, args.getColor, args.getWidth(), item_pos, item_size, coverage_ptr);
        }
    }
} // namespace xII::Item::Renderer
