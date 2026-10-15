//
// Created by qpwwser on 2026-10-11
//

#include "PoSize.h"
#include "Space/Point.h"
#include "Space/Range.h"

namespace xII::Screen {
    PoSize Point2PoSize(const std::shared_ptr<const Space::Point>& map_point, const Space::Range& map_size,
                        const PoSize& screen_pos, const PoSize& screen_size,
                        const std::shared_ptr<const Space::Point>& point);
}
