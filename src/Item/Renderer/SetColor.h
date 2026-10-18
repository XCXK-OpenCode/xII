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
        {
            // 注意要按引用捕获 args：SetColor 之后还可能有 Fadein/Fadeout/SetAlpha
            // 修改 alpha，按值捕获会把当时的 alpha 固定下来，导致淡入淡出对颜色无效。
            // 颜色自带的 A 与 args 的 alpha 相乘：
            //   - 只用 SetColor(pixel) 时，alpha 就是 pixel.A；
            //   - 再叠加 SetAlpha/Fadein/Fadeout 时会按比例淡出，而不是被 args 的默认 255 覆盖。
            args.getColor = [this, &args](const Args::DrawContext& ctx) -> Screen::Screen::Pixel {
                const uint32_t alpha = static_cast<uint32_t>(_color.A) * args.getAlpha() / 255u;
                return {.A = static_cast<uint8_t>(alpha), .R = _color.R, .G = _color.G, .B = _color.B};
            };
        };
    };
} // namespace xII::Item::Renderer