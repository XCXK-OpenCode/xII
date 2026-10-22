//
// Created by qpwser on 9/30/26.
//

#pragma once

#include "Base/Quantity/Container.h"
#include "Point.h"
#include "Offset.h"

namespace xII::Space {
    template <typename Offset_Type>
    class OffsetPointRoot : public Point {
    protected:
        std::shared_ptr<Point> _origin;
        Offset_Type _offset;

    public:
        // OffsetPointRoot() = default;
        OffsetPointRoot(const std::shared_ptr<Point>& origin, const Offset_Type& offset)
            : _origin(origin),
              _offset(offset) {}

        ~OffsetPointRoot() override = default;

        Base::Quantity::Base_Value_Type getX() const override { return _origin->getX() + _offset.getX(); }
        Base::Quantity::Base_Value_Type getY() const override { return _origin->getY() + _offset.getY(); }
        std::shared_ptr<Point> getRoot() override { return _origin->getRoot(); }

        std::shared_ptr<Point> getOrigin() const { return _origin; }
        void setOrigin(const std::shared_ptr<Point>& origin) { _origin = origin; }

        Offset_Type getOffset() const { return _offset; }
        Offset_Type &getOffsetRef() { return _offset; }
        const Offset_Type &getOffsetRef() const { return _offset; }

        // 点本身是“活的”引用：持有它的 Item / Camera 每帧都会重新调用 getX()/getY()，
        // 所以修改 offset 后不需要重建对象，也不需要重新 set 给使用者。
        void setOffset(const Offset_Type& offset) { _offset = offset; }
        bool operator==(const OffsetPointRoot& v) const { return (_origin == v._origin) && (_offset == v._offset); }
        bool operator!=(const OffsetPointRoot& v) const { return (_origin != v._origin) || (_offset != v._offset); }
    };

    using OffsetPoint = OffsetPointRoot<Offset>;
} // namespace xII::Space
