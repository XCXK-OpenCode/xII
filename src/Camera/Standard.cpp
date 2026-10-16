#include "Standard.h"
#include "Item/Renderer/Args.h"

namespace xII::Camera {
    void Standard::Shoot() {
        _object->Render(*this, Item::Renderer::Args());
    }
} // namespace xII::Camera
