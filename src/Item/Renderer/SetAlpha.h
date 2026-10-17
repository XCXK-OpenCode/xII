//
// Created by qpwser from XCXK on 2026-10-17
//

#pragma once

#include "Base.h"

namespace xII::Item::Renderer {
    class SetAlpha : public Base {
    private:
        uint8_t _alpha;

    public:
        SetAlpha(uint8_t alpha) : _alpha(alpha) {}
        ~SetAlpha() override = default;

        uint8_t getAlpha() const { return _alpha; }
        void setAlpha(uint8_t alpha) { _alpha = alpha; }

        void Render(Camera::Base& camera, std::vector<std::shared_ptr<Space::Point>>& points, Args &args, const Screen::PoSize& item_pos, const Screen::PoSize& item_size) const override {
            args.setAlpha(_alpha);
        }
    };
} // namespace xII::Item::Renderer