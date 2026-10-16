//
// Created by qpwser in 2026-10-11
//

#include "Base.h"
#include "Screen/Screen.h"
#include <cstddef>

namespace xII::Item::Renderer {
    class Line : public Base {
    private:

    public:
        static void draw_line(Screen::RGBA8888& screen,
                              const Screen::PoSize& A,
                              const Screen::PoSize& B,
                              const std::function<Screen::RGBA8888::Pixel(const Args::DrawContext&)>& getColor,
                              size_t width,
                              const Screen::PoSize& item_pos,
                              const Screen::PoSize& item_size);

    public:
        Line() = default;
        // Line() {}

        ~Line() override = default;

        void Render(Camera::Base& camera, std::vector<std::shared_ptr<Space::Point>>& points, Args &arg, const Screen::PoSize& item_pos, const Screen::PoSize& item_size) const override;
    };
} // namespace xII::Item::Renderer
