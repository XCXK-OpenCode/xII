//
// Created by qpwser from XCXK on 10/7/26
//

#pragma once

#include "Base.h"
#include "Screen/Screen.h"
#include <cstddef>

namespace xII::Camera {
    class Outline : public Base {
    private:
        Screen::Screen::Pixel _color;
        size_t _width;

    public:
        Outline(const Outline& v) = default;
        Outline(const Item::Shape::Rectangle& shape,
                const Screen::PoSize& screen_size,
                const std::shared_ptr<Item::Base>& object,
                const std::shared_ptr<Time::OffsetPoint>& time_point,
                const Screen::Screen::Pixel& color = {191, 0, 255, 127},
                size_t width = 2)
            : Base(shape, screen_size, object, time_point),
              _color(color),
              _width(width) {}

        ~Outline() override = default;

        Screen::Screen::Pixel getColor() const { return _color; }
        void setColor(const Screen::Screen::Pixel& color) { _color = color; }

        size_t getWidth() const { return _width; }
        void setWidth(size_t width) { _width = width; }

        void Shoot() override;
    };
} // namespace xII::Camera
