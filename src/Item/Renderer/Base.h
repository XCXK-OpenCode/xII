//
// Created by qpwser from XCXK on 7/10/26
//

#pragma once

#include "Camera/Base.h"
#include "Space/Point.h"
#include "Args.h"

#include <vector>
#include <memory>

namespace xII::Item::Renderer {
    class Base {
    private:

    public:
        Base() = default;

        virtual ~Base() = default;

        virtual void Render(Camera::Base& camera, std::vector<std::shared_ptr<Space::Point>>& points, Args &args, const Screen::PoSize& item_pos, const Screen::PoSize& item_size) const = 0;
    };
} // namespace xII::Item::Renderer
