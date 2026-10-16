//
// Created by qpwser from XCXK on 2026-10-15
//

#pragma once

#include "Point.h"
#include "Offset.h"

namespace xII::Time {
    template <typename Offset_Type>
    class OffsetPointRoot : public Point {
    protected:
        const std::shared_ptr<Point> _origin;
        Offset_Type _offset;

    public:
        OffsetPointRoot(const std::shared_ptr<Point>& origin, const Offset_Type& offset)
            : _origin(origin),
              _offset(offset) {}

        ~OffsetPointRoot() override = default;

        long long getTIme() const override { return _origin->getTIme() + _offset.getTime(); }
        std::shared_ptr<Point> getRoot() override { return _origin->getRoot(); }

        std::shared_ptr<Point> getOrigin() const { return _origin; }
        Offset_Type getOffset() const { return _offset; }

        bool operator==(const OffsetPointRoot& v) const { return (_origin == v._origin) && (_offset == v._offset); }
        bool operator!=(const OffsetPointRoot& v) const { return (_origin != v._origin) || (_offset != v._offset); }
    };

    using OffsetPoint = OffsetPointRoot<Offset>;
}