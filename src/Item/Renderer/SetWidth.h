// 
// Created by qpwser from XCXK on 2026-10-15
//

#pragma once

#include "Base.h"

namespace xII::Item::Renderer {
    class SetWidth : public Base {
        private:
            uint32_t _width;

        public:
            SetWidth(uint32_t width) : _width(width) {}
            ~SetWidth() override = default;

            void setWidth(uint32_t width) { _width = width; }
            uint32_t getWidth() const { return _width; }

            void Render(Camera::Base& camera, std::vector<std::shared_ptr<Space::Point>>& points, Args &args, const Screen::PoSize& item_pos, const Screen::PoSize& item_size) const override 
            { args.getWidth = [this]() -> uint32_t { return _width; }; };
    };
}