//
// Created by qpwser from XCXK on 2026-10-15
//

#pragma once

#include "Base.h"

namespace xII::Item::Renderer {
    class Rainbow1 : public Base {
    private:

    public:
        Rainbow1() = default;
        ~Rainbow1() override = default;

        void Render(Camera::Base& camera, std::vector<std::shared_ptr<Space::Point>>& points, Args &args, const Screen::PoSize& item_pos, const Screen::PoSize& item_size) const override;
    };
}