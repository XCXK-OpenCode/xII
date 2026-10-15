#include "Standard.h"

namespace xII::Camera {
    void Standard::Shoot() {
        _object->Render(*this);
    }
} // namespace xII::Camera
