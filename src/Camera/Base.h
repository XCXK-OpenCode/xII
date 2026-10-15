//
// Created by qpwser on 10/2/26. XCXK
//

#pragma once

#include "Item/Shape/Rectangle.h"
#include "Screen/Screen.h"
#include "Item/Base.h"
#include "Space/Point.h"
#include <memory>

namespace xII::Camera {
    class Base {
    protected:
        Item::Shape::Rectangle _shape;
        Screen::Screen _screen;
        std::shared_ptr<Item::Base> _object;

    public:
        Base(const Item::Shape::Rectangle& shape, const Screen::PoSize& screen_size, const std::shared_ptr<Item::Base>& object)
            : _shape(shape),
              _screen(screen_size),
              _object(object) {}
        virtual ~Base() = default;

        Item::Shape::Rectangle getShape() const { return _shape; }
        void setShape(const Item::Shape::Rectangle& shape) { _shape = shape; }

        Screen::Screen getScreen() const { return _screen; }
        Screen::Screen& getScreenR() { return _screen; }
        const Screen::Screen& getScreenR() const { return _screen; }
        void setScreen(const Screen::Screen& screen) { _screen = screen; }

        std::shared_ptr<Item::Base> getObject() const { return _object; }
        void setObject(const std::shared_ptr<Item::Base>& object) { _object = object; }

        std::shared_ptr<const Space::Point> getPoint() { return _shape.getP2(); }
        Space::Range getRange() { return _shape.getRange(); }

        virtual void Shoot() = 0;
    };
} // namespace xII::Camera
