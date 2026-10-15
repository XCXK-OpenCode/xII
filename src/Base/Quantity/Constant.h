/*****************************************************************************
 * @file        Constant.h
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

#include "Variable.h"

#include <memory>

namespace xII::Base::Quantity::Constant {
    const std::shared_ptr<Variable> Zero = std::make_shared<Variable>(0);
    const std::shared_ptr<Variable> Pi = std::make_shared<Variable>(3.14159265358979323846264338327950288419716939937510582097494459230781640628620899862803482534211706); // 98 numbers after '.'
}