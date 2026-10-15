/*****************************************************************************
 * @file        Container.h
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

namespace xII::Base::Quantity {
    using Base_Value_Type = float;

    class Container {
    public:
        virtual ~Container() = default;
        [[nodiscard]] virtual Base_Value_Type getValue() const = 0;

        operator Base_Value_Type() const { return getValue(); }
    };

    inline bool operator==(const Container &l, const Container &r) { return l.getValue() == r.getValue(); }
    inline bool operator!=(const Container &l, const Container &r) { return l.getValue() != r.getValue(); }
    inline bool operator<(const Container &l, const Container &r) { return l.getValue() < r.getValue(); }
    inline bool operator>(const Container &l, const Container &r) { return l.getValue() > r.getValue(); }
    inline bool operator<=(const Container &l, const Container &r) { return l.getValue() <= r.getValue(); }
    inline bool operator>=(const Container &l, const Container &r) { return l.getValue() >= r.getValue(); }
}