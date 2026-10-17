//
// Created by qpwser from XCXK on 2026-10-15
//

#pragma once

#include "Base.h"
#include "Time/Point.h"
#include "Base/Quantity/Container.h"

namespace xII::Item::Renderer {
    template<xII::Base::Quantity::Base_Value_Type (*func)(xII::Base::Quantity::Base_Value_Type)>
    class Fadein : public Base {
    private:
        std::shared_ptr<Time::Point> _start_time;
        Time::Offset _length;

        public:
        Fadein(const std::shared_ptr<Time::Point>& start_time, const Time::Offset& length)
            : _start_time(start_time),
                _length(length) {}
        Fadein(const Fadein& other) = default;
        ~Fadein() override = default;

        std::shared_ptr<Time::Point> getStartTime() const { return _start_time; }
        void setStartTime(const std::shared_ptr<Time::Point>& start_time) { _start_time = start_time; }

        Time::Offset getLength() const { return _length; }
        void setLength(const Time::Offset& length) { _length = length; }

        void Render(Camera::Base& camera, std::vector<std::shared_ptr<Space::Point>>& points, Args &args, const Screen::PoSize& item_pos, const Screen::PoSize& item_size) const override {
            auto current_time = camera.getTimePoint();
            auto start_time = _start_time->getTime();
            auto length = _length.getTime();
            auto end_time = start_time + length;

            if (current_time->getTime() < start_time) {
                return;
            } else if (current_time->getTime() >= end_time) {
                return;
            }

            xII::Base::Quantity::Base_Value_Type progress = static_cast<xII::Base::Quantity::Base_Value_Type>(current_time->getTime() - start_time) / static_cast<xII::Base::Quantity::Base_Value_Type>(length);
            xII::Base::Quantity::Base_Value_Type value = func(progress);

            args.multiplyAlpha(static_cast<uint8_t>(value * 255));

            std::cout << "Fadein: progress = " << progress << ", value = " << value << ", alpha = " << static_cast<int>(args.getAlpha()) << std::endl;
        }
    };
} // namespace xII::Item::Renderer