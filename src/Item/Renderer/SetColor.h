// 
// Created by qpwser from XCXK on 2026-10-15
//

#pragma once

#include "Base.h"

namespace xII::Item::Renderer {
    class SetColor : public Base {
    private:
        Screen::Screen::Pixel _color;

    public:
        SetColor(const Screen::Screen::Pixel& color) : _color(color) {}
        ~SetColor() override = default;

        void setColor(const Screen::Screen::Pixel& color) { _color = color; }
        Screen::Screen::Pixel getColor() const { return _color; }

        void Render(Camera::Base& camera, std::vector<std::shared_ptr<Space::Point>>& points, Args &args, const Screen::PoSize& item_pos, const Screen::PoSize& item_size) const override 
        { args.getColor = [this, args](const Args::DrawContext& ctx) -> Screen::Screen::Pixel { return {.A = args.getAlpha(), .R = _color.R, .G = _color.G, .B = _color.B}; }; };
    };
} // namespace xII::Item::Renderer