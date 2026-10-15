#include "Base.h"

#include "Camera/Base.h"
#include "Camera/Standard.h"
#include "Camera/Outline.h"
#include "Renderer/Line.h"

namespace xII::Item {
    bool Base::isInRange(const Camera::Base& camera) const {
        return true;
    }

    void Base::Render(Camera::Standard& camera, Item::Renderer::Args args) const {
        for (const auto& subitem : _subitems)
            if (subitem->isInRange(camera))
                subitem->Render(camera, args);
    }
    void Base::Render(Camera::Outline& camera) const {
        for (const auto& subitem : _subitems)
            if (subitem->isInRange(camera))
                subitem->Render(camera);
    }
} // namespace xII::Item
