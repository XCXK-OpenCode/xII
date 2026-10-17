// 
// Created by qpwser from XCXK on 2026-10-15
// 

#pragma once

#include "Point.h"

namespace xII::Time {
    template<typename Value_Type>
    class OffsetRoot {
        private:
            Value_Type _time;

        public:
            OffsetRoot(const Value_Type &time) : _time(time) {}
            virtual ~OffsetRoot() = default;

            Value_Type getTime() const { return _time; }

            void setTime(const Value_Type &time) { _time = time; }
    };

    using Offset = OffsetRoot<long long>;
}