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

        // 注意：这两个 setter 之前被误写成 const 方法，const 对象里不能赋值，
        // 一旦被调用就编译失败（模板没实例化所以一直没暴露）。
        void setX(const Value_Type &x) { _x = x; }
        void setY(const Value_Type &y) { _y = y; }

    };

    using Offset = OffsetRoot<Base::Quantity::Base_Value_Type>;
}