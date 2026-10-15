/*****************************************************************************
 * @file        Variable.h
 * @brief       ${BRIEF_DESCRIPTION}
 *
 * @project     xII (XCXK (OC) II)
 * @organization XCXK-OC (XCXK OpenCode)
 *              (A subsidiary organization under XCXK)
 *
 * @copyright   Copyright (C) XCXK-OC. All Rights Reserved.
 *
 * @license     Use and distribution of this source code, binary forms, or any
 *              derivative works are permitted ONLY under the terms and
 *              conditions explicitly stipulated by XCXK-OC.
 *
 * @legal       DISCLAIMER & INTERPRETATION RIGHTS:
 *              XCXK-OC reserves the right of final interpretation for this
 *              software, its source code, documentation, and related materials.
 *              As the parent organization, XCXK retains the overriding right
 *              of final interpretation over all projects under XCXK-OC,
 *              including this one (xII / XCXK (OC) II).
 *
 *              Unauthorized distribution, modification, or use that violates
 *              the stipulated regulations is strictly prohibited.
 *****************************************************************************/
#pragma once

#include "Container.h"

#include <utility>

namespace xII::Base::Quantity {
    class Variable : public Container {
    private:
        Base_Value_Type _value;
    public:
        Variable(const Base_Value_Type &value) : _value(value) {}
        Variable(const Variable &v) : _value(v._value) {}
        ~Variable() override = default;

        [[nodiscard]] Base_Value_Type getValue() const override { return _value; }
        void setValue(const Base_Value_Type &value) { _value = value; }

        Variable &operator=(const Variable &v) { _value = v._value; return *this; }
        Variable &operator=(Variable &&v) noexcept { _value = v._value; return *this; }

        Variable &operator++() { _value++; return *this; }
        Variable operator++(int) { return _value++; }
        Variable &operator--() { _value--; return *this; }
        Variable operator--(int) { return _value--; }

        Variable operator-() const { return -_value; }
        Variable operator+() const { return +_value; }

        Variable operator-(const Base_Value_Type v) const { return _value - v; }
        Variable operator+(const Base_Value_Type v) const { return _value + v; }

        Variable &operator+=(const Base_Value_Type v) { _value += v; return *this; }
        Variable &operator-=(const Base_Value_Type v) { _value -= v; return *this; }
    };
}
