//
// 绘制上下文：从 Item::Renderer::Args 中抽出来，
// 让 Screen 层（Canvas）不必反向依赖 Item 层。
//

#pragma once

#include "PoSize.h"

namespace xII::Screen {
    struct DrawContext {
        PoSize screen_pos;   // 当前像素/顶点在屏幕上的位置
        PoSize screen_size;  // 屏幕尺寸
        PoSize item_pos;     // 当前 item 在屏幕上的左上角
        PoSize item_size;    // 当前 item 在屏幕上的尺寸（可能为负）
    };
} // namespace xII::Screen
