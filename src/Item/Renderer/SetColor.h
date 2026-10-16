// 
// Created by qpwser from XCXK on 2026-10-15
//

#include "Base.h"

namespace xII::Item::Renderer {
    class SetColor : public Base {
    private:
        Screen::RGBA8888::Pixel _color;

    public:
        SetColor(const Screen::RGBA8888::Pixel& color) : _color(color) {}
        ~SetColor() override = default;

        void setColor(const Screen::RGBA8888::Pixel& color) { _color = color; }
        Screen::RGBA8888::Pixel getColor() const { return _color; }

        void Render(Camera::Base& camera, std::vector<std::shared_ptr<Space::Point>>& points, Args &args) const override 
        { args.getColor = [this, args](const Args::DrawContext& ctx) -> Screen::Screen::Pixel { return {.A = args.getAlpha(), .R = _color.R, .G = _color.G, .B = _color.B}; }; };
    };
} // namespace xII::Item::Renderer