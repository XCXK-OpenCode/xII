#include "Line.h"
#include "Screen/PoSize.h"
#include "Screen/PointPos.h"

#include <cmath>

namespace xII::Item::Renderer {
    void Line::draw_line(Screen::RGBA8888& screen,
                         const Screen::PoSize& A,
                         const Screen::PoSize& B,
                         const Screen::RGBA8888::Pixel& color,
                         size_t width) {
        if (width == 0)
            return;

        const size_t W = screen.getSize().x;
        const size_t H = screen.getSize().y;
        Screen::RGBA8888::Pixel* data = screen.getData();

        int x0 = static_cast<int>(A.x), y0 = static_cast<int>(A.y);
        int x1 = static_cast<int>(B.x), y1 = static_cast<int>(B.y);
        int dx = std::abs(x1 - x0), dy = std::abs(y1 - y0);
        int sx = (x0 < x1) ? 1 : -1;
        int sy = (y0 < y1) ? 1 : -1;

        // 计算沿法线方向的偏移量集合
        const int half = static_cast<int>(width) / 2;
        const int lo = -(half);
        const int hi = (static_cast<int>(width) - 1) - half;

        // 若线段更"水平"，沿 y 方向偏移；否则沿 x 方向偏移
        const bool horizontal = dx >= dy;

        auto plot = [&](int px, int py) {
            if (px >= 0 && py >= 0 &&
                static_cast<size_t>(px) < W && static_cast<size_t>(py) < H) {
                auto& dst = data[static_cast<size_t>(py) * W + static_cast<size_t>(px)];
                dst = xII::Screen::RGBA8888::blend_alpha_pixel(color, dst);
            }
        };

        for (int off = lo; off <= hi; ++off) {
            int cx0 = x0, cy0 = y0;
            int cx1 = x1, cy1 = y1;
            if (horizontal) {
                cy0 += off;
                cy1 += off;
            } else {
                cx0 += off;
                cx1 += off;
            }

            int err = dx - dy;
            int cx = cx0, cy = cy0;
            const int ex = cx1, ey = cy1;

            while (true) {
                plot(cx, cy);
                if (cx == ex && cy == ey)
                    break;
                int e2 = 2 * err;
                if (e2 > -dy) {
                    err -= dy;
                    cx += sx;
                }
                if (e2 < dx) {
                    err += dx;
                    cy += sy;
                }
            }
        }
    }

    void Line::Render(Camera::Base& camera, std::vector<std::shared_ptr<Space::Point>>& points, Args &arg) const {
        Screen::PoSize screen_point = {0, 0};
        auto screen_size = camera.getScreen().getSize();
        auto map_point = camera.getPoint();
        auto map_size = camera.getRange();
        const size_t n = points.size();
        for (size_t i = 0; i < n; i++) {
            auto a = Screen::Point2PoSize(map_point, map_size, screen_point, screen_size, points[i]);
            auto b = Screen::Point2PoSize(map_point, map_size, screen_point, screen_size, points[(i + 1) % n]);
            draw_line(camera.getScreenR(), a, b, arg.getColor(), 1);
        }
    }
} // namespace xII::Item::Renderer
