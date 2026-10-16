//
// Created by qpwser on 10/2/26.
//

#pragma once

#include "Space/Point.h"
#include "Base/Quantity/Container.h"
#include "Space/OffsetPoint.h"

#include <vector>
#include <memory>

namespace xII::Item::Shape {
    class Base {
    private:

    public:
        Base() = default;
        virtual ~Base() = default;

        virtual std::vector<std::shared_ptr<Space::Point>> getPoints(const xII::Base::Quantity::Base_Value_Type PPU) = 0;

        virtual std::pair<std::shared_ptr<const Space::Point>, std::shared_ptr<const Space::Point>> getPR() = 0;
    };
}
