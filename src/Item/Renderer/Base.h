//
// Created by qpwser from XCXK on 7/10/26
//

#pragma once

#include "Camera/Base.h"
#include "Space/Point.h"

namespace xII::Item::Renderer {
    class Base {
    private:

    public:
        Base() = default;

        virtual ~Base() = default;

        virtual void Render(Camera::Base& camera, std::vector<std::shared_ptr<Space::Point>>& points) const = 0;
    };
} // namespace xII::Item::Renderer
