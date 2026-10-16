// 
// Created by qpwser from XCXK on 2026-10-15
//

#pragma once

#include "Base.h"
#include "Screen/PoSize.h"
#include "Screen/PointPos.h"

#include <algorithm>
#include <cmath>

namespace xII::Item::Renderer {
    class Fill : public Base {
    private:

    public:
        static void draw_fill(Screen::RGBA8888& screen,
                              const std::vector<Screen::PoSize>& pts,
                              const std::function<Screen::RGBA8888::Pixel(const Args::DrawContext&)>& getColor,
                              const Screen::PoSize& item_pos,
                              const Screen::PoSize& item_size);

    public:
        Fill() = default;
        ~Fill() override = default;

        void Render(Camera::Base& camera,
                    std::vector<std::shared_ptr<Space::Point>>& points,
                    Args& args,
                    const Screen::PoSize& item_pos,
                    const Screen::PoSize& item_size) const override;
    };
} // namespace xII::Item::Renderer