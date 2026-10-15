//
// Created by qpwser on 9/30/26.
//

#pragma once

#include "Base/Quantity/Container.h"
#include <memory>

namespace xII::Space {
    class Point : public std::enable_shared_from_this<Point> {
    private:

    public:
        Point() = default;

        virtual ~Point() = default;

        virtual Base::Quantity::Base_Value_Type getX() const { return 0; }
        virtual Base::Quantity::Base_Value_Type getY() const { return 0; }
        virtual std::shared_ptr<Point> getRoot() { return shared_from_this(); }
    };
} // namespace xII::Space
