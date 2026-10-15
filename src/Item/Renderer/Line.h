//
// Created by qpwser in 2026-10-11
//

#include "Base.h"
#include "Screen/Screen.h"
#include <cstddef>

namespace xII::Item::Renderer {
    class Line : public Base {
    private:
        Screen::Screen::Pixel _color;
        size_t _width;

    public:
        static void draw_line(Screen::RGBA8888& screen,
                              const Screen::PoSize& A,
                              const Screen::PoSize& B,
                              const Screen::RGBA8888::Pixel& color,
                              size_t width);

    public:
        Line() = default;
        Line(const Screen::Screen::Pixel& color, size_t width)
            : _color(color),
              _width(width) {}

        ~Line() override = default;

        Screen::Screen::Pixel getColor() const { return _color; }
        void setColor(const Screen::Screen::Pixel& color) { _color = color; }

        size_t getWidth() const { return _width; }
        void setWidth(size_t width) { _width = width; }

        void Render(Camera::Base& camera, std::vector<std::shared_ptr<Space::Point>>& points) const override;
    };
} // namespace xII::Item::Renderer
