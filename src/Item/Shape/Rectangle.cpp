//
// Created by qpwser on 10/2/26.
//

#include "Rectangle.h"

#include "AxisOrientation.h"
#include "Space/Offset.h"
#include "Space/OffsetPoint.h"
#include "Space/Point.h"
#include <memory>

namespace xII::Item::Shape {
    std::shared_ptr<const Space::Point> Rectangle::getP0() const {
        return std::make_shared<Space::OffsetPoint>(_center, Space::Offset(_width / 2 * static_cast<xII::Base::Quantity::Base_Value_Type>(AxisOrientation::Horizontal),
                                                                           _height / 2 * static_cast<xII::Base::Quantity::Base_Value_Type>(AxisOrientation::Vertical)));
    }
    std::shared_ptr<const Space::Point> Rectangle::getP1() const {
        return std::make_shared<Space::OffsetPoint>(_center, Space::Offset(_width / -2 * static_cast<xII::Base::Quantity::Base_Value_Type>(AxisOrientation::Horizontal),
                                                                           _height / 2 * static_cast<xII::Base::Quantity::Base_Value_Type>(AxisOrientation::Vertical)));
    }
    std::shared_ptr<const Space::Point> Rectangle::getP2() const {
        return std::make_shared<Space::OffsetPoint>(_center, Space::Offset(_width / -2 * static_cast<xII::Base::Quantity::Base_Value_Type>(AxisOrientation::Horizontal),
                                                                           _height / -2 * static_cast<xII::Base::Quantity::Base_Value_Type>(AxisOrientation::Vertical)));
    }
    std::shared_ptr<const Space::Point> Rectangle::getP3() const {
        return std::make_shared<Space::OffsetPoint>(_center, Space::Offset(_width / 2 * static_cast<xII::Base::Quantity::Base_Value_Type>(AxisOrientation::Horizontal),
                                                                           _height / -2 * static_cast<xII::Base::Quantity::Base_Value_Type>(AxisOrientation::Vertical)));
    }
    std::vector<std::shared_ptr<Space::Point>> Rectangle::getPoints(const xII::Base::Quantity::Base_Value_Type PPU) {
        auto data = new Space::OffsetPoint[]{

            Space::OffsetPoint(_center, Space::Offset(_width / 2 * static_cast<xII::Base::Quantity::Base_Value_Type>(AxisOrientation::Horizontal),
                                                      _height / 2 * static_cast<xII::Base::Quantity::Base_Value_Type>(AxisOrientation::Vertical))),
            Space::OffsetPoint(_center, Space::Offset(_width / -2 * static_cast<xII::Base::Quantity::Base_Value_Type>(AxisOrientation::Horizontal),
                                                      _height / 2 * static_cast<xII::Base::Quantity::Base_Value_Type>(AxisOrientation::Vertical))),
            Space::OffsetPoint(_center, Space::Offset(_width / -2 * static_cast<xII::Base::Quantity::Base_Value_Type>(AxisOrientation::Horizontal),
                                                      _height / -2 * static_cast<xII::Base::Quantity::Base_Value_Type>(AxisOrientation::Vertical))),
            Space::OffsetPoint(_center, Space::Offset(_width / 2 * static_cast<xII::Base::Quantity::Base_Value_Type>(AxisOrientation::Horizontal),
                                                      _height / -2 * static_cast<xII::Base::Quantity::Base_Value_Type>(AxisOrientation::Vertical)))};

        std::shared_ptr<Space::OffsetPoint[4]> owner(data);
        std::vector<std::shared_ptr<Space::Point>> vec;
        vec.reserve(4);

        vec.emplace_back(owner, data);
        vec.emplace_back(owner, data + 1);
        vec.emplace_back(owner, data + 2);
        vec.emplace_back(owner, data + 3);

        return vec;
    }
} // namespace xII::Item::Shape
