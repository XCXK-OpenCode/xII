//
// Created by qpwser from XCXK on 20-26-10-11
//

#include "General.h"
#include "Renderer/Line.h"
#include "Camera/Standard.h"
#include "Camera/Outline.h"
#include "Screen/PointPos.h"

namespace xII::Item {
    void General::Render(Camera::Standard& camera, Item::Renderer::Args args) const {
        auto points = _shape->getPoints(0);
        for (auto& renderer : _renderers)
            renderer->Render(camera, points, args);
        for (const auto& subitem : _subitems)
            if (subitem->isInRange(camera))
                subitem->Render(camera, args);
    }
    
    void General::Render(Camera::Outline& camera) const {
        Screen::PoSize screen_point = {0, 0};
        auto screen_size = camera.getScreen().getSize();
        auto map_point = camera.getPoint();
        auto map_size = camera.getRange();

        auto points = _shape->getPoints(0);
        for (auto ItA = points.begin(); ItA != points.end(); ++ItA)
            for (auto ItB = ItA; ItB != points.end(); ++ItB)
                Renderer::Line::draw_line(camera.getScreenR(),
                                          Screen::Point2PoSize(map_point, map_size, screen_point, screen_size, *ItA),
                                          Screen::Point2PoSize(map_point, map_size, screen_point, screen_size, *ItB),
                                          camera.getColor(),
                                          camera.getWidth());
    }
} // namespace xII::Item
