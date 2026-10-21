//
// Created by qpwser on 10/2/26.
//


#pragma once

#include "RGBA8888.h"

#include <memory>

namespace xII::Screen {
    using Screen = RGBA8888;

    // 屏幕的共享句柄：Camera::Base / Item::Image 都靠它持有屏幕，
    // 因此 `image.getScreen() << camera.getScreen()` 这种写法可以直接用。
    using ScreenRef = std::shared_ptr<Screen>;

    // 把右侧整块画面深拷贝进左侧缓冲区（尺寸不同时左侧先 resize）。
    //   auto pip = std::make_shared<Item::Image>(PoSize{480, 270}, shape);
    //   pip->getScreen() << mini_camera.getScreen();
    // 空句柄安全（任一侧为空则什么都不做），返回左侧句柄以支持链式 a << b << c。
    // 依赖 ADL：shared_ptr<RGBA8888> 的关联命名空间包含 xII::Screen。
    inline ScreenRef operator<<(const ScreenRef& dest, const ScreenRef& src) {
        if (dest && src)
            *dest << *src;
        return dest;
    }
}