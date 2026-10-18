//
// Created by qpwser from XCXK on 2026-10-15
//

#include "Fill.h"
#include "Screen/PoSize.h"
#include "Screen/PointPos.h"

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <algorithm>
#include <functional>

namespace xII::Item::Renderer {

    // 求扫描线 y 与线段 (A,B) 的交点 x；不相交或为水平边返回 false
    static bool edge_intersect_x(const Screen::PoSize& A,
                                 const Screen::PoSize& B,
                                 int64_t y,
                                 int64_t& out_x) {
        if (A.y == B.y)
            return false;

        int64_t ymin = std::min(A.y, B.y);
        int64_t ymax = std::max(A.y, B.y);

        // 半开区间 [ymin, ymax)，避免顶点重复计数
        if (y < ymin || y >= ymax)
            return false;

        double t = static_cast<double>(y - A.y) /
                   static_cast<double>(B.y - A.y);
        out_x = static_cast<int64_t>(std::llround(
            static_cast<double>(A.x) +
            t * (static_cast<double>(B.x) - static_cast<double>(A.x))));
        return true;
    }

    void Fill::draw_fill(Screen::RGBA8888& screen,
                         const std::vector<Screen::PoSize>& pts,
                         const std::function<Screen::RGBA8888::Pixel(const Args::DrawContext&)>& getColor,
                         const Screen::PoSize& item_pos,
                         const Screen::PoSize& item_size) {
        const size_t n = pts.size();
        if (n < 3) return;

        const size_t W = screen.getSize().x;
        const size_t H = screen.getSize().y;
        Screen::RGBA8888::Pixel* data = screen.getData();

        // 1. 求包围盒
        int64_t min_y = pts[0].y;
        int64_t max_y = pts[0].y;
        for (size_t i = 1; i < n; ++i) {
            if (pts[i].y < min_y) min_y = pts[i].y;
            if (pts[i].y > max_y) max_y = pts[i].y;
        }

        // 2. 与屏幕范围求交
        int64_t clip_x0 = 0;
        int64_t clip_y0 = 0;
        int64_t clip_x1 = static_cast<int64_t>(W) - 1;
        int64_t clip_y1 = static_cast<int64_t>(H) - 1;

        // 3. 再与 item 矩形求交（规范化，支持负 size）
        int64_t ixA = item_pos.x;
        int64_t iyA = item_pos.y;
        int64_t ixB = item_pos.x + item_size.x;
        int64_t iyB = item_pos.y + item_size.y;

        int64_t ix0 = std::min(ixA, ixB);
        int64_t iy0 = std::min(iyA, iyB);
        int64_t ix1 = std::max(ixA, ixB);
        int64_t iy1 = std::max(iyA, iyB);

        if (ix0 > clip_x0) clip_x0 = ix0;
        if (iy0 > clip_y0) clip_y0 = iy0;
        if (ix1 < clip_x1) clip_x1 = ix1;
        if (iy1 < clip_y1) clip_y1 = iy1;

        if (clip_x1 < clip_x0 || clip_y1 < clip_y0)
            return; // 裁剪区域为空

        if (min_y < clip_y0) min_y = clip_y0;
        if (max_y > clip_y1) max_y = clip_y1;

        const Screen::PoSize screen_size = screen.getSize();

        std::vector<int64_t> xs;
        xs.reserve(n);

        // 4. 逐行扫描
        for (int64_t y = min_y; y <= max_y; ++y) {
            xs.clear();

            for (size_t i = 0; i < n; ++i) {
                const auto& A = pts[i];
                const auto& B = pts[(i + 1) % n];
                int64_t x;
                if (edge_intersect_x(A, B, y, x))
                    xs.push_back(x);
            }

            if (xs.size() < 2)
                continue;

            std::sort(xs.begin(), xs.end());

            for (size_t k = 0; k + 1 < xs.size(); k += 2) {
                int64_t x_start = xs[k];
                int64_t x_end   = xs[k + 1];

                if (x_start < clip_x0) x_start = clip_x0;
                if (x_end   > clip_x1) x_end   = clip_x1;

                if (x_start > x_end)
                    continue;

                for (int64_t x = x_start; x <= x_end; ++x) {
                    const size_t ux = static_cast<size_t>(x);
                    const size_t uy = static_cast<size_t>(y);

                    const Args::DrawContext ctx{
                        Screen::PoSize{ x, y },
                        screen_size,
                        item_pos,
                        item_size
                    };

                    const Screen::RGBA8888::Pixel color = getColor(ctx);

                    auto& dst = data[uy * W + ux];
                    if (color.A == 255) {
                        dst = color;
                    } else if (color.A != 0) {
                        dst = Screen::RGBA8888::blend_alpha_pixel(color, dst);
                    }
                }
            }
        }
    }

    void Fill::Render(Camera::Base& camera,
                      std::vector<std::shared_ptr<Space::Point>>& points,
                      Args& args,
                      const Screen::PoSize& item_pos,
                      const Screen::PoSize& item_size) const {
        Screen::PoSize screen_point = {0, 0};
        auto screen_size = camera.getScreenR().getSize();
        auto map_point   = camera.getPoint();
        auto map_size    = camera.getRange();

        const size_t n = points.size();
        if (n < 3) return;

        std::vector<Screen::PoSize> screen_pts;
        screen_pts.reserve(n);
        for (size_t i = 0; i < n; ++i) {
            screen_pts.push_back(
                Screen::Point2PoSize(map_point, map_size, screen_point,
                                     screen_size, points[i]));
        }

        // ---- GPU 后端 ----
        if (auto* canvas = camera.getCanvas()) {
            canvas->fill_polygon(screen_pts, args.getColor, item_pos, item_size);
            return;
        }

        // ---- CPU 后端 ----
        draw_fill(camera.getScreenR(), screen_pts, args.getColor, item_pos, item_size);
    }
} // namespace xII::Item::Renderer