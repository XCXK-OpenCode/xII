//
// 画布抽象：渲染器（Line / Fill）把“几何 + 取色函数”交给画布，
// 由具体后端决定怎么画。
//
//   - 不设置 Canvas 时：沿用原来的 CPU 光栅化路径（直接写 RGBA8888）。
//   - 设置 GLCanvas 时：走 GPU 三角形路径。
//
// 坐标系与 CPU 后端保持一致：
//   一个“屏幕像素 (x, y)”对应 GL 里 [x, x+1] × [y, y+1] 的单位方格，
//   顶点坐标用连续的像素单位表示（像素中心为 x+0.5）。
//

#pragma once

#include "DrawContext.h"
#include "PoSize.h"
#include "RGBA8888.h"

#include <cstdint>
#include <functional>
#include <vector>

namespace xII::Screen {
    using ColorFunction = std::function<RGBA8888::Pixel(const DrawContext&)>;

    class Canvas {
    public:
        virtual ~Canvas() = default;

        [[nodiscard]] virtual const char* name() const = 0;

        // 每帧开始：清屏（与 RGBA8888::clear() 的语义一致）
        virtual void begin_frame(const RGBA8888::Pixel& clear_color) = 0;

        // 每帧结束：提交本帧累积的所有几何
        virtual void end_frame() = 0;

        // 凸多边形填充，pts 为屏幕坐标（可以是不规则的，按凸多边形处理）
        virtual void fill_polygon(const std::vector<PoSize>& pts,
                                  const ColorFunction& get_color,
                                  const PoSize& item_pos,
                                  const PoSize& item_size) = 0;

        // 有宽度的折线。closed=true 时首尾相连。
        // 语义与 CPU 的 Line::draw_line + coverage 掩码一致：
        // 每个像素最多合成一次，不会因为线段/线宽重叠而叠加变亮。
        virtual void stroke_polyline(const std::vector<PoSize>& pts,
                                     bool closed,
                                     const ColorFunction& get_color,
                                     uint32_t width,
                                     const PoSize& item_pos,
                                     const PoSize& item_size) = 0;

        // 把画布内容读回 CPU 缓冲（测试 / 截图 / 混用 CPU 后处理时使用）。
        // GPU 后端会触发一次同步读回。
        virtual void read_pixels(RGBA8888& out) = 0;
    };
} // namespace xII::Screen
