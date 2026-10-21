// 
// Created by qpwser from XCXK on 2026-10-21
//

#pragma once

#include "Base.h"
#include "Screen/Screen.h"
#include "Shape/Rectangle.h"

#include <cstdint>
#include <utility>

namespace xII::Item {
    class Image : public Base {
    private:
        Screen::Screen _screen;
        Shape::Rectangle _shape;

    public:
        Image(const Image&) = default;
        Image(Image&&) = default;

        Image(Screen::Screen screen, const Shape::Rectangle& shape)
            : _screen(std::move(screen)),
              _shape(shape) {}

        ~Image() override = default;

        Screen::Screen getScreen() const { return _screen; }
        Screen::Screen& getScreenRef() { return _screen; }
        const Screen::Screen& getScreenRef() const { return _screen; }

        // 深拷贝源图：RGBA8888::operator= 现在会先 resize 再整体拷贝。
        void setScreen(const Screen::Screen& screen) { _screen = screen; }

        Shape::Rectangle getShape() const { return _shape; }
        Shape::Rectangle& getShapeRef() { return _shape; }
        const Shape::Rectangle& getShapeRef() const { return _shape; }
        void setShape(const Shape::Rectangle& shape) { _shape = shape; }

        // 目标矩形映射到屏幕后的像素矩形，半开区间 [left, right) x [top, bottom)。
        // 宽高 <= 0 时 valid = false。
        struct ScreenRect {
            bool valid = false;
            double left = 0.0;
            double top = 0.0;
            double right = 0.0;
            double bottom = 0.0;

            [[nodiscard]] double width() const { return right - left; }
            [[nodiscard]] double height() const { return bottom - top; }
        };

        [[nodiscard]] ScreenRect getScreenRect(Camera::Base& camera) const;

        void Render(Camera::Standard& camera, Item::Renderer::Args args) const override;
        void Render(Camera::Outline& camera) const override;

        // 目标矩形与相机视口是否相交；完全离开时父级会跳过 Render。
        [[nodiscard]] bool isInRange(const Camera::Base& camera) const override;
    };
}

