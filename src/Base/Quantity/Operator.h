/*****************************************************************************
 * @file        Operator.h
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
#include <memory>

namespace xII::Base::Quantity {
    class OperatorCC : public Container {
    protected:
        std::shared_ptr<const Container> _l;
        std::shared_ptr<const Container> _r;
    public:
        OperatorCC(const std::shared_ptr<const Container> &l, const std::shared_ptr<const Container> &r) : _l(l), _r(r) {}
        ~OperatorCC() override = default;

        std::shared_ptr<const Container> getLeft() const { return _l; }
        std::shared_ptr<const Container> getRight() const { return _r; }
        void setValue(const std::shared_ptr<const Container> &l, const std::shared_ptr<const Container> &r) { _l = l; _r = r; }

        class Add;
        class Div;
        class Mul;
        class Sub;
    };

    class OperatorCC::Add : public OperatorCC {
    public:
        Add(const std::shared_ptr<const Container> &l, const std::shared_ptr<const Container> &r) : OperatorCC(l, r) {}
        Add(const Add &v) : OperatorCC(v._l, v._r) {}
        ~Add() override = default;

        Base_Value_Type getValue() const override { return _l->getValue() + _r->getValue(); }

        Add &operator=(const Add &v) {
            this->_l = v._l;
            this->_r = v._r;
            return *this;
        }

        Add &operator=(Add &&v) noexcept {
            this->_l = std::move(v._l);
            this->_r = std::move(v._r);
            return *this;
        }
    };
    // inline OperatorCC::Add operator+(const std::shared_ptr<const Container> &l, const std::shared_ptr<const Container> &r) { return OperatorCC::Add(l, r); }
    inline std::shared_ptr<const OperatorCC::Add> operator+(const std::shared_ptr<const Container> &l, const std::shared_ptr<const Container> &r) { return std::make_shared<const OperatorCC::Add>(l, r); }

    class OperatorCC::Div : public OperatorCC {
    public:
        Div(const std::shared_ptr<const Container> &l, const std::shared_ptr<const Container> &r) : OperatorCC(l, r) {}
        Div(const Div &v) : OperatorCC(v._l, v._r) {}
        ~Div() override = default;

        Base_Value_Type getValue() const override { return _l->getValue() / _r->getValue(); }

        Div &operator=(const Div &v) {
            this->_l = v._l;
            this->_r = v._r;
            return *this;
        }

        Div &operator=(Div &&v) noexcept {
            this->_l = std::move(v._l);
            this->_r = std::move(v._r);
            return *this;
        }
    };
    // inline OperatorCC::Div operator/(const std::shared_ptr<const Container> &l, const std::shared_ptr<const Container> &r) { return OperatorCC::Div(l, r); }
    inline std::shared_ptr<const OperatorCC::Div> operator/(const std::shared_ptr<const Container> &l, const std::shared_ptr<const Container> &r) { return std::make_shared<const OperatorCC::Div>(l, r); }

    class OperatorCC::Mul : public OperatorCC {
    public:
        Mul(const std::shared_ptr<const Container> &l, const std::shared_ptr<const Container> &r) : OperatorCC(l, r) {}
        Mul(const Mul &v) : OperatorCC(v._l, v._r) {}
        ~Mul() override = default;

        Base_Value_Type getValue() const override { return _l->getValue() * _r->getValue(); }

        Mul &operator=(const Mul &v) {
            this->_l = v._l;
            this->_r = v._r;
            return *this;
        }

        Mul &operator=(Mul &&v) noexcept {
            this->_l = std::move(v._l);
            this->_r = std::move(v._r);
            return *this;
        }
    };
    // inline OperatorCC::Mul operator*(const std::shared_ptr<const Container> &l, const std::shared_ptr<const Container> &r) { return OperatorCC::Mul(l, r); }
    inline std::shared_ptr<const OperatorCC::Mul> operator*(const std::shared_ptr<const Container> &l, const std::shared_ptr<const Container> &r) { return std::make_shared<const OperatorCC::Mul>(l, r); }

    class OperatorCC::Sub : public OperatorCC {
    public:
        Sub(const std::shared_ptr<const Container> &l, const std::shared_ptr<const Container> &r) : OperatorCC(l, r) {}
        Sub(const Sub &v) : OperatorCC(v._l, v._r) {}
        ~Sub() override = default;

        Base_Value_Type getValue() const override { return _l->getValue() - _r->getValue(); }

        Sub &operator=(const Sub &v) {
            this->_l = v._l;
            this->_r = v._r;
            return *this;
        }

        Sub &operator=(Sub &&v) noexcept {
            this->_l = std::move(v._l);
            this->_r = std::move(v._r);
            return *this;
        }
    };
    //inline OperatorCC::Sub operator-(const std::shared_ptr<const Container> &l, const std::shared_ptr<const Container> &r) { return OperatorCC::Sub(l, r); }
    inline std::shared_ptr<const OperatorCC::Sub> operator-(const std::shared_ptr<const Container> &l, const std::shared_ptr<const Container> &r) { return std::make_shared<const OperatorCC::Sub>(l, r); }

    class OperatorCB : public Container {
    protected:
        std::shared_ptr<const Container> _l;
        Base_Value_Type _r;
    public:
        OperatorCB(const std::shared_ptr<const Container> &l, const Base_Value_Type r) : _l(l), _r(r) {}
        ~OperatorCB() override = default;

        std::shared_ptr<const Container> getLeft() const { return _l; }
        Base_Value_Type getRight() const { return _r; }
        void setValue(const std::shared_ptr<const Container> &l, const Base_Value_Type r) { _l = l; _r = r; }

        class Add;
        class Div;
        class Mul;
        class Sub;
    };

    class OperatorCB::Add : public OperatorCB {
    public:
        Add(const std::shared_ptr<const Container> &l, const Base_Value_Type r) : OperatorCB(l, r) {}
        Add(const Add &v) : OperatorCB(v._l, v._r) {}
        ~Add() override = default;

        Base_Value_Type getValue() const override { return _l->getValue() + _r; }

        Add &operator=(const Add &v) {
            this->_l = v._l;
            this->_r = v._r;
            return *this;
        }

        Add &operator=(Add &&v) noexcept {
            this->_l = std::move(v._l);
            this->_r = std::move(v._r);
            return *this;
        }
    };
    // inline OperatorCB::Add operator+(const std::shared_ptr<const Container> &l, const Base_Value_Type &r) { return OperatorCB::Add(l, r); }
    inline std::shared_ptr<const OperatorCB::Add> operator+(const std::shared_ptr<const Container> &l, const Base_Value_Type &r) { return std::make_shared<const OperatorCB::Add>(l, r); }

    class OperatorCB::Div : public OperatorCB {
    public:
        Div(const std::shared_ptr<const Container> &l, const Base_Value_Type r) : OperatorCB(l, r) {}
        Div(const Div &v) : OperatorCB(v._l, v._r) {}
        ~Div() override = default;

        Base_Value_Type getValue() const override { return _l->getValue() / _r; }

        Div &operator=(const Div &v) {
            this->_l = v._l;
            this->_r = v._r;
            return *this;
        }

        Div &operator=(Div &&v) noexcept {
            this->_l = std::move(v._l);
            this->_r = std::move(v._r);
            return *this;
        }
    };
    // inline OperatorCB::Div operator/(const std::shared_ptr<const Container> &l, const Base_Value_Type &r) { return OperatorCB::Div(l, r); }
    inline std::shared_ptr<const OperatorCB::Div> operator/(const std::shared_ptr<const Container> &l, const Base_Value_Type &r) { return std::make_shared<const OperatorCB::Div>(l, r); }

    class OperatorCB::Mul : public OperatorCB {
    public:
        Mul(const std::shared_ptr<const Container> &l, const Base_Value_Type r) : OperatorCB(l, r) {}
        Mul(const Mul &v) : OperatorCB(v._l, v._r) {}
        ~Mul() override = default;

        Base_Value_Type getValue() const override { return _l->getValue() * _r; }

        Mul &operator=(const Mul &v) {
            this->_l = v._l;
            this->_r = v._r;
            return *this;
        }

        Mul &operator=(Mul &&v) noexcept {
            this->_l = std::move(v._l);
            this->_r = std::move(v._r);
            return *this;
        }
    };
    // inline OperatorCB::Mul operator*(const std::shared_ptr<const Container> &l, const Base_Value_Type &r) { return OperatorCB::Mul(l, r); }
    inline std::shared_ptr<const OperatorCB::Mul> operator*(const std::shared_ptr<const Container> &l, const Base_Value_Type &r) { return std::make_shared<const OperatorCB::Mul>(l, r); }

    class OperatorCB::Sub : public OperatorCB {
    public:
        Sub(const std::shared_ptr<const Container> &l, const Base_Value_Type r) : OperatorCB(l, r) {}
        Sub(const Sub &v) : OperatorCB(v._l, v._r) {}
        ~Sub() override = default;

        Base_Value_Type getValue() const override { return _l->getValue() - _r; }

        Sub &operator=(const Sub &v) {
            this->_l = v._l;
            this->_r = v._r;
            return *this;
        }

        Sub &operator=(Sub &&v) noexcept {
            this->_l = std::move(v._l);
            this->_r = std::move(v._r);
            return *this;
        }
    };
    // inline OperatorCB::Sub operator-(const std::shared_ptr<const Container> &l, const Base_Value_Type &r) { return OperatorCB::Sub(l, r); }
    inline std::shared_ptr<const OperatorCB::Sub> operator-(const std::shared_ptr<const Container> &l, const Base_Value_Type &r) { return std::make_shared<const OperatorCB::Sub>(l, r); }

    class OperatorBC : public Container {
    protected:
        Base_Value_Type _l;
        std::shared_ptr<const Container> _r;
    public:
        OperatorBC(const Base_Value_Type l, const std::shared_ptr<const Container> &r) : _l(l), _r(r) {}
        ~OperatorBC() override = default;

        Base_Value_Type getLeft() const { return _l; }
        std::shared_ptr<const Container> getRight() const { return _r; }
        void setValue(const Base_Value_Type l, const std::shared_ptr<const Container> &r) { _l = l; _r = r; }

        class Add;
        class Div;
        class Mul;
        class Sub;
    };

    class OperatorBC::Add : public OperatorBC {
    public:
        Add(const Base_Value_Type l, const std::shared_ptr<const Container> &r) : OperatorBC(l, r) {}
        Add(const Add &v) : OperatorBC(v._l, v._r) {}
        ~Add() override = default;

        Base_Value_Type getValue() const override { return _l + _r->getValue(); }

        Add &operator=(const Add &v) {
            this->_l = v._l;
            this->_r = v._r;
            return *this;
        }

        Add &operator=(Add &&v) noexcept {
            this->_l = std::move(v._l);
            this->_r = std::move(v._r);
            return *this;
        }
    };
    // inline OperatorBC::Add operator+(const Base_Value_Type &l, const std::shared_ptr<const Container> &r) { return OperatorBC::Add(l, r); }
    inline std::shared_ptr<const OperatorBC::Add> operator+(const Base_Value_Type &l, const std::shared_ptr<const Container> &r) { return std::make_shared<const OperatorBC::Add>(l , r); }

    class OperatorBC::Div : public OperatorBC {
    public:
        Div(const Base_Value_Type l, const std::shared_ptr<const Container> &r) : OperatorBC(l, r) {}
        Div(const Div &v) : OperatorBC(v._l, v._r) {}
        ~Div() override = default;

        Base_Value_Type getValue() const override { return _l / _r->getValue(); }

        Div &operator=(const Div &v) {
            this->_l = v._l;
            this->_r = v._r;
            return *this;
        }

        Div &operator=(Div &&v) noexcept {
            this->_l = std::move(v._l);
            this->_r = std::move(v._r);
            return *this;
        }
    };
    // inline OperatorBC::Div operator/(const Base_Value_Type &l, const std::shared_ptr<const Container> &r) { return OperatorBC::Div(l, r); }
    inline std::shared_ptr<const OperatorBC::Div> operator/(const Base_Value_Type &l, const std::shared_ptr<const Container> &r) { return std::make_shared<const OperatorBC::Div>(l , r); }

    class OperatorBC::Mul : public OperatorBC {
    public:
        Mul(const Base_Value_Type l, const std::shared_ptr<const Container> &r) : OperatorBC(l, r) {}
        Mul(const Mul &v) : OperatorBC(v._l, v._r) {}
        ~Mul() override = default;

        Base_Value_Type getValue() const override { return _l * _r->getValue(); }

        Mul &operator=(const Mul &v) {
            this->_l = v._l;
            this->_r = v._r;
            return *this;
        }

        Mul &operator=(Mul &&v) noexcept {
            this->_l = std::move(v._l);
            this->_r = std::move(v._r);
            return *this;
        }
    };
    //inline OperatorBC::Mul operator*(const Base_Value_Type &l, const std::shared_ptr<const Container> &r) { return OperatorBC::Mul(l, r); }
    inline std::shared_ptr<const OperatorBC::Mul> operator*(const Base_Value_Type &l, const std::shared_ptr<const Container> &r) { return std::make_shared<const OperatorBC::Mul>(l , r); }

    class OperatorBC::Sub : public OperatorBC {
    public:
        Sub(const Base_Value_Type l, const std::shared_ptr<const Container> &r) : OperatorBC(l, r) {}
        Sub(const Sub &v) : OperatorBC(v._l, v._r) {}
        ~Sub() override = default;

        Base_Value_Type getValue() const override { return _l - _r->getValue(); }

        Sub &operator=(const Sub &v) {
            this->_l = v._l;
            this->_r = v._r;
            return *this;
        }

        Sub &operator=(Sub &&v) noexcept {
            this->_l = std::move(v._l);
            this->_r = std::move(v._r);
            return *this;
        }
    };
    // inline OperatorBC::Sub operator-(const Base_Value_Type &l, const std::shared_ptr<const Container> &r) { return OperatorBC::Sub(l, r); }
    inline std::shared_ptr<const OperatorBC::Sub> operator-(const Base_Value_Type &l, const std::shared_ptr<const Container> &r) { return std::make_shared<const OperatorBC::Sub>(l , r); }

    inline std::shared_ptr<const Container> operator+(const std::shared_ptr<const Container> &v) { return std::make_shared<const OperatorCB::Mul>(v, +1); }
    inline std::shared_ptr<const Container> operator-(const std::shared_ptr<const Container> &v) { return std::make_shared<const OperatorCB::Mul>(v, -1); }
}