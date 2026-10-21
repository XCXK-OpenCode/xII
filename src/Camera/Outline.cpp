#include "Outline.h"
#include "Item/Renderer/Line.h"
#include "Item/Renderer/Args.h"

using namespace xII::Item::Renderer;

namespace xII::Camera {
    void Outline::Shoot() {
        const Screen::PoSize pairs[][2] = {{{0, 0}, {0, getScreen()->getSize().y}},
                                           {{0, 0}, {getScreen()->getSize().x, 0}},
                                           {{0, getScreen()->getSize().y}, getScreen()->getSize()},
                                           {{getScreen()->getSize().x, 0}, getScreen()->getSize()},
                                           {{0, 0}, getScreen()->getSize()},
                                           {{0, getScreen()->getSize().y}, {getScreen()->getSize().x, 0}}};
        for (auto& pair : pairs)
            Item::Renderer::Line::draw_line(*getScreen(), pair[0], pair[1], getColor, getWidth(), {0, 0}, getScreen()->getSize());

        _object->Render(*this);
    }
} // namespace xII::Camera
