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

        auto item_pr = _shape->getPR();
        auto item_pos = Screen::Point2PoSize(camera.getPoint(), camera.getRange(), {0, 0}, camera.getScreenR().getSize(), item_pr.first);
        auto item_epos = Screen::Point2PoSize(camera.getPoint(), camera.getRange(), {0, 0}, camera.getScreenR().getSize(), item_pr.second);
        auto item_size = Screen::PoSize{item_epos.x - item_pos.x, item_epos.y - item_pos.y};

        for (auto& renderer : _renderers)
            renderer->Render(camera, points, args, item_pos, item_size);
        for (const auto& subitem : _subitems)
            if (subitem->isInRange(camera))
                subitem->Render(camera, args);
    }
    
    void General::Render(Camera::Outline& camera) const {
        Screen::PoSize screen_point = {0, 0};
        auto screen_size = camera.getScreenR().getSize();
        auto map_point = camera.getPoint();
        auto map_size = camera.getRange();

        auto points = _shape->getPoints(0);
        for (auto ItA = points.begin(); ItA != points.end(); ++ItA)
            for (auto ItB = ItA; ItB != points.end(); ++ItB)
                Renderer::Line::draw_line(camera.getScreenR(),
                                          Screen::Point2PoSize(map_point, map_size, screen_point, screen_size, *ItA),
                                          Screen::Point2PoSize(map_point, map_size, screen_point, screen_size, *ItB),
                                          camera.getColor,
                                          camera.getWidth(), 
                                          {0, 0},
                                          camera.getScreenR().getSize());
    }
} // namespace xII::Item
