//
// Created by qpwser on 10/2/26.
//

#pragma once

#include "Base.h"

namespace xII::Camera {
    class Standard : public Base {
    private:

    public:
        Standard(const Standard& v) = default;
        Standard(const Item::Shape::Rectangle& shape, const Screen::PoSize& screen_size, const std::shared_ptr<Item::Base>& object, const std::shared_ptr<Time::OffsetPoint>& time_point)
            : Base(shape, screen_size, object, time_point) {}

        ~Standard() override = default;

        void Shoot() override;
    };
} // namespace xII::Camera
