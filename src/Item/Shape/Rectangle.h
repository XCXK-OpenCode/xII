//
// Created by qpwser on 10/2/26.
//

#pragma once

#include "Base.h"
#include "Space/Point.h"
#include "Space/Range.h"
#include <memory>

namespace xII::Item::Shape {
    class Rectangle : public Base {
    private:
        std::shared_ptr<Space::Point> _center;
        xII::Base::Quantity::Base_Value_Type _width;
        xII::Base::Quantity::Base_Value_Type _height;

    public:
        //Rectangle() = default;
        Rectangle(const Rectangle& v) = default;
        Rectangle(const std::shared_ptr<Space::Point>& center, const xII::Base::Quantity::Base_Value_Type& width, const xII::Base::Quantity::Base_Value_Type& height)
            : _center(center),
              _width(width),
              _height(height) {}
        ~Rectangle() override = default;

        std::shared_ptr<Space::Point> getCenter() const { return _center; }
        xII::Base::Quantity::Base_Value_Type getWidth() const { return _width; }
        xII::Base::Quantity::Base_Value_Type getHeight() const { return _height; }

        void setCenter(const std::shared_ptr<Space::Point>& center) { _center = center; }
        void setWidth(const xII::Base::Quantity::Base_Value_Type& width) { _width = width; }
        void setHeight(const xII::Base::Quantity::Base_Value_Type& height) { _height = height; }

        std::shared_ptr<const Space::Point> getP0() const;
        std::shared_ptr<const Space::Point> getP1() const;
        std::shared_ptr<const Space::Point> getP2() const;
        std::shared_ptr<const Space::Point> getP3() const;

        Space::Range getRange() { return {_width, _height}; }

        std::vector<std::shared_ptr<Space::Point>> getPoints(const xII::Base::Quantity::Base_Value_Type PPU) override;
    };
} // namespace xII::Item::Shape
