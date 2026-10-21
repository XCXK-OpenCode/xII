//
// Created by qpwser on 10/2/26. XCXK
//

#pragma once

#include "Item/Shape/Rectangle.h"
#include "Screen/Screen.h"
#include "Item/Base.h"
#include "Space/Point.h"
#include "Time/OffsetPoint.h"
#include <memory>

namespace xII::Camera {
    class Base {
    protected:
        Item::Shape::Rectangle _shape;
        std::shared_ptr<Screen::Screen> _screen;
        std::shared_ptr<Item::Base> _object;
        std::shared_ptr<Time::OffsetPoint> _time_point;

    public:
        Base(const Item::Shape::Rectangle& shape, const Screen::PoSize& screen_size, const std::shared_ptr<Item::Base>& object, const std::shared_ptr<Time::OffsetPoint>& time_point)
            : _shape(shape),
              _object(object),
              _time_point(time_point) { _screen = std::make_shared<Screen::Screen>(screen_size); }

        virtual ~Base() = default;

        Item::Shape::Rectangle getShape() const { return _shape; }
        void setShape(const Item::Shape::Rectangle& shape) { _shape = shape; }

        std::shared_ptr<Screen::Screen> getScreen() const { return _screen; }
        void setScreen(const std::shared_ptr<Screen::Screen>& screen) { _screen = screen; }

        std::shared_ptr<Item::Base> getObject() const { return _object; }
        void setObject(const std::shared_ptr<Item::Base>& object) { _object = object; }

        std::shared_ptr<Time::OffsetPoint> getTimePoint() const { return _time_point; }
        void setTimePoint(const std::shared_ptr<Time::OffsetPoint>& time_point) { _time_point = time_point; }

        std::shared_ptr<const Space::Point> getPoint() { return _shape.getP2(); }
        Space::Range getRange() { return _shape.getRange(); }

        virtual void Shoot() = 0;
    };
} // namespace xII::Camera
