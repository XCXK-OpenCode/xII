//
// Created by qpwser on 10/1/26.
//

#pragma once

#include "Renderer/Args.h"

#include <memory>
#include <vector>

namespace xII::Camera {
    class Base;
    class Standard;
    class Outline;
} // namespace xII::Camera

namespace xII::Item {
    class Base {
    protected:
        // std::vector<std::unique_ptr<Base>> _subitems;
        std::vector<std::shared_ptr<Base>> _subitems;

    public:
        Base() = default;
        virtual ~Base() = default;
        /*
        void addItem(std::unique_ptr<Base>&& item) {
            _subitems.push_back(std::move(item));
        }
        */

        void addItem(std::shared_ptr<Base>&& item) {
            _subitems.push_back(item);
        }

        void erase(const size_t index) {
            if (index < _subitems.size())
                _subitems.erase(_subitems.begin() + static_cast<std::ptrdiff_t>(index));
        }
        void clear() { _subitems.clear(); }

        // std::unique_ptr<Base>& getSubitem(const size_t index) { return _subitems.at(index); }
        std::shared_ptr<Base>& getSubitem(const size_t index) { return _subitems.at(index); }
        // std::vector<std::unique_ptr<Base>>& getSubitemsR() { return _subitems; }
        std::vector<std::shared_ptr<Base>>& getSubitemsR() { return _subitems; }

        virtual bool isInRange(const Camera::Base& camera) const;
        virtual void Render(Camera::Standard& camera, Item::Renderer::Args args) const;
        virtual void Render(Camera::Outline& camera) const;
    };
} // namespace xII::Item
