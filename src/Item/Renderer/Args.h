//
// Created by qpwser from XCXK on 2026-10-14
//

#pragma once

#include "Screen/Screen.h"

#include <functional>

namespace xII::Item::Renderer {
    class Args {
    private:
        uint8_t _alpha = 255;

    public:
        Args() = default;
        Args(const Args& args) = default;
        
        ~Args() = default;

        struct DrawContext {
            Screen::PoSize screen_pos;
            Screen::PoSize screen_size;
            Screen::PoSize item_pos;
            Screen::PoSize item_size;
        };      

        std::function<Screen::Screen::Pixel(const DrawContext&)> getColor = [this](const DrawContext& ctx) -> Screen::Screen::Pixel { return Screen::Screen::Pixel{0, 0, 0, _alpha}; };
        
        std::function<uint32_t()> getWidth = [this]() -> uint32_t { return 1; };

        uint8_t getAlpha() const { return _alpha; }
        void setAlpha(uint8_t alpha) { _alpha = alpha; }
        void multiplyAlpha(uint8_t alpha) { _alpha = static_cast<uint8_t>(_alpha * (alpha / 255.0f)); }
    };
} // namespace xII::Item::Renderer
