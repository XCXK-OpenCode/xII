//
// Created by qpwser on 9/30/26.
//

#pragma once

#include "Base/Quantity/Container.h"

namespace xII::Space {
    template<typename Value_Type>
    class OffsetRoot {
    private:
        Value_Type _x, _y;

    public:
        OffsetRoot(const Value_Type &x, const Value_Type &y) : _x(x), _y(y) {}

        ~OffsetRoot() = default;

        Value_Type getX() const { return _x; }
        Value_Type getY() const { return _y; }

        void setX(const Value_Type &x) const { _x = x; }
        void setY(const Value_Type &y) const { _y = y; }

    };

    using Offset = OffsetRoot<Base::Quantity::Base_Value_Type>;
}