// 
// Created by qpwser from XCXK on 2026-10-15
//

#pragma once

#include <memory>

namespace xII::Time {
    class Point : public std::enable_shared_from_this<Point> {
    private:

    public:
        Point() = default;
        virtual ~Point() = default;

        virtual long long getTIme() const { return 0; }
        virtual std::shared_ptr<Point> getRoot() { return shared_from_this(); }

    };
}