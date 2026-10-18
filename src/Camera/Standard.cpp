#include "Standard.h"
#include "Item/Renderer/Args.h"
#include "Screen/Canvas.h"

namespace xII::Camera {
    void Standard::Shoot() {
        // GPU 后端：先清屏，再提交几何，最后 flush。
        // CPU 后端：清屏仍由调用方的 getScreenR().clear() 负责，行为保持不变。
        if (_canvas != nullptr) {
            const Screen::RGBA8888::Pixel clear_color{.A = 255, .R = 0, .G = 0, .B = 0};
            _canvas->begin_frame(clear_color);
        }

        _object->Render(*this, Item::Renderer::Args());

        if (_canvas != nullptr)
            _canvas->end_frame();
    }
} // namespace xII::Camera
