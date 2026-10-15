#include "PointPos.h"
#include "Space/Point.h"

namespace xII::Screen {
    PoSize Point2PoSize(const std::shared_ptr<const Space::Point>& map_point, const Space::Range& map_size,
                        const PoSize& screen_pos, const PoSize& screen_size,
                        const std::shared_ptr<const Space::Point>& point) {
        auto map_x = map_point->getX();
        auto map_y = map_point->getY();
        auto map_w = map_size.getX();
        auto map_h = map_size.getY();

        auto screen_x = screen_pos.x;
        auto screen_y = screen_pos.y;
        auto screen_w = screen_size.x;
        auto screen_h = screen_size.y;

        auto px = point->getX();
        auto py = point->getY();

        auto x = (px - map_x) * (screen_w / map_w) + screen_x;
        auto y = (py - map_y) * (screen_h / map_h) + screen_y;

        return {static_cast<int64_t>(x), static_cast<int64_t>(y)};
    }
} // namespace xII::Screen
