#include "Outline.h"
#include "Item/Renderer/Line.h"
#include "Item/Renderer/Args.h"

using namespace xII::Item::Renderer;

namespace xII::Camera {
    void Outline::Shoot() {
        const Screen::PoSize pairs[][2] = {{{0, 0}, {0, getScreenR().getSize().y}},
                                           {{0, 0}, {getScreenR().getSize().x, 0}},
                                           {{0, getScreenR().getSize().y}, getScreenR().getSize()},
                                           {{getScreenR().getSize().x, 0}, getScreenR().getSize()},
                                           {{0, 0}, getScreenR().getSize()},
                                           {{0, getScreenR().getSize().y}, {getScreenR().getSize().x, 0}}};
        for (auto& pair : pairs)
            Item::Renderer::Line::draw_line(getScreenR(), pair[0], pair[1], getColor, getWidth());

        _object->Render(*this);
    }
} // namespace xII::Camera
