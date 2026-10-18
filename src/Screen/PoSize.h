//
// Created by qpwser on 10/1/26.
//

#pragma once
// #include <cstddef>
#include <cstdint>

namespace xII::Screen {
    struct PoSize {
        // size_t x, y;
        int64_t x, y;
    };

    inline PoSize operator+(const PoSize &l, const PoSize &r) { return {l.x + r.x, l.y + r.y}; }

    inline PoSize operator-(const PoSize &l, const PoSize &r) { return {l.x - r.x, l.y - r.y}; }


} // namespace xII::Screen
