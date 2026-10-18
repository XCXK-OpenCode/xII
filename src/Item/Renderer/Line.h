//
// Created by qpwser in 2026-10-11
//

#pragma once

#include "Base.h"
#include "Screen/Screen.h"
#include <cstddef>
#include <cstdint>
#include <vector>

namespace xII::Item::Renderer {
    class Line : public Base {
    private:

    public:
        // coverage 非空时用来记录本次绘制已经混合过的像素：
        // 同一像素只合成一次，避免线宽/线段重叠导致 alpha 累加。
        static void draw_line(Screen::RGBA8888& screen,
                              const Screen::PoSize& A,
                              const Screen::PoSize& B,
                              const std::function<Screen::RGBA8888::Pixel(const Args::DrawContext&)>& getColor,
                              size_t width,
                              const Screen::PoSize& item_pos,
                              const Screen::PoSize& item_size,
                              std::vector<uint8_t>* coverage = nullptr);

    public:
        Line() = default;
        // Line() {}

        ~Line() override = default;

        void Render(Camera::Base& camera, std::vector<std::shared_ptr<Space::Point>>& points, Args &arg, const Screen::PoSize& item_pos, const Screen::PoSize& item_size) const override;
    };
} // namespace xII::Item::Renderer
