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
                              const Screen::RGBA8888::Pixel& color,
                              size_t width);

    public:
        Line() = default;
        // Line() {}

        ~Line() override = default;

        void Render(Camera::Base& camera, std::vector<std::shared_ptr<Space::Point>>& points, Args &arg) const override;
    };
} // namespace xII::Item::Renderer
