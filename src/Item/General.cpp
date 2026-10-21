//
// Created by qpwser from XCXK on 20-26-10-11
//

#include "General.h"
#include "Renderer/Line.h"
#include "Camera/Standard.h"
#include "Camera/Outline.h"
#include "Screen/PointPos.h"

#include <algorithm>

namespace xII::Item {
    bool General::isInRange(const Camera::Base& camera) const {
        // 没有形状信息时不能确定位置，交给上层（与 Item::Base 一致：不剔除）。
        if (_shape == nullptr)
            return true;

        // 形状包围盒：getPR() 返回一对对角点（Rectangle 是 P1/P3）。
        const auto pr = _shape->getPR();
        if (pr.first == nullptr || pr.second == nullptr)
            return true;

        const double ax = static_cast<double>(pr.first->getX());
        const double ay = static_cast<double>(pr.first->getY());
        const double bx = static_cast<double>(pr.second->getX());
        const double by = static_cast<double>(pr.second->getY());

        const double item_x0 = std::min(ax, bx);
        const double item_x1 = std::max(ax, bx);
        const double item_y0 = std::min(ay, by);
        const double item_y1 = std::max(ay, by);

        // 相机视口就是它自己的 Shape::Rectangle（center + 宽高），也是场景坐标。
        const auto view = camera.getShape();
        const auto view_center = view.getCenter();
        const double view_w = static_cast<double>(view.getWidth());
        const double view_h = static_cast<double>(view.getHeight());
        if (view_center == nullptr || view_w <= 0.0 || view_h <= 0.0)
            return true; // 视口无效时不做剔除

        const double view_x = static_cast<double>(view_center->getX());
        const double view_y = static_cast<double>(view_center->getY());

        // 描边宽度是屏幕像素，而这里比较的是场景坐标，所以要按相机缩放折算。
        //   Standard：Fill 之类的墨迹不会超出包围盒，只留 1px 抵消坐标取整误差；
        //   Outline ：会用 getWidth() 画线，线条向外扩出半个线宽。
        double stroke_half_px = 1.0;
        if (const auto* outline = dynamic_cast<const Camera::Outline*>(&camera))
            stroke_half_px += static_cast<double>(outline->getWidth()) / 2.0;

        const auto screen_size = camera.getScreenR().getSize();
        const double scene_per_px_x = (screen_size.x > 0) ? view_w / static_cast<double>(screen_size.x) : 0.0;
        const double scene_per_px_y = (screen_size.y > 0) ? view_h / static_cast<double>(screen_size.y) : 0.0;

        const double margin_x = stroke_half_px * scene_per_px_x;
        const double margin_y = stroke_half_px * scene_per_px_y;

        // 轴对齐包围盒相交：只有“完全在视口某一侧之外”才算离开。
        const double view_x0 = view_x - view_w / 2.0 - margin_x;
        const double view_x1 = view_x + view_w / 2.0 + margin_x;
        const double view_y0 = view_y - view_h / 2.0 - margin_y;
        const double view_y1 = view_y + view_h / 2.0 + margin_y;

        return !(item_x1 < view_x0 || item_x0 > view_x1 ||
                 item_y1 < view_y0 || item_y0 > view_y1);
    }

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
        for (const auto& subitem : _subitems)
            if (subitem->isInRange(camera))
                subitem->Render(camera);                             
    }
} // namespace xII::Item
