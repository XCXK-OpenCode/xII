#include "Base.h"

#include "Camera/Base.h"
#include "Camera/Standard.h"
#include "Camera/Outline.h"
#include "Renderer/Line.h"

namespace xII::Item {
    bool Base::isInRange(const Camera::Base& camera) const {
        return true;
    }

    void Base::Render(Camera::Standard& camera) const {
        for (const auto& subitem : _subitems)
            if (subitem->isInRange(camera))
                subitem->Render(camera);
    }
    void Base::Render(Camera::Outline& camera) const {
        for (const auto& subitem : _subitems)
            if (subitem->isInRange(camera))
                subitem->Render(camera);
    }
} // namespace xII::Item
