//
// 编译期渲染后端选择
//
// 用法（CMake）：
//   cmake -DXII_ENABLE_OPENGL=OFF ..   # 只有 CPU 后端（默认）
//   cmake -DXII_ENABLE_OPENGL=ON  ..   # 同时编译 OpenGL 后端
//
// 宏语义：
//   XII_BACKEND_CPU  恒为 1。CPU 光栅化后端永远可用，它同时也是
//                    GPU 后端的像素级校验基准，以及无 GPU 环境时的兜底。
//   XII_BACKEND_GL   由 XII_ENABLE_OPENGL 决定。为 1 时才会编译
//                    GLCanvas / GLFunctions，并提供 GLCanvas::create()。
//
// 这两个宏只决定“能不能用”，运行时用不用由 Camera::setCanvas() 决定，
// 因此同一份业务代码可以 --cpu / --gl 切换而不需要重新编译。
//

#pragma once

#ifndef XII_ENABLE_OPENGL
#   define XII_ENABLE_OPENGL 0
#endif

#define XII_BACKEND_CPU 1
#define XII_BACKEND_GL  XII_ENABLE_OPENGL

namespace xII::Screen::Backend {
    inline constexpr bool cpu = XII_BACKEND_CPU != 0;
    inline constexpr bool gl  = XII_BACKEND_GL != 0;
} // namespace xII::Screen::Backend
