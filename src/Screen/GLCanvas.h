//
// OpenGL 3.3 core 画布后端
//
// 只在 XII_BACKEND_GL 为 1 时参与编译（见 Backend.h / CMakeLists.txt）。
//

#pragma once

#include "Backend.h"

#if XII_BACKEND_GL

#include "Canvas.h"

#include <memory>

struct SDL_Window;

namespace xII::Screen {
    class GLCanvas final : public Canvas {
    public:
        // 需要先创建好 OpenGL 上下文（SDL_GL_CreateContext）并使之成为当前上下文。
        // 返回 nullptr 时 getLastError() 给出原因。
        static std::unique_ptr<GLCanvas> create(SDL_Window* window,
                                                const PoSize& size);

        ~GLCanvas() override;

        [[nodiscard]] const char* name() const override { return "opengl"; }

        void begin_frame(const RGBA8888::Pixel& clear_color) override;
        void end_frame() override;

        void fill_polygon(const std::vector<PoSize>& pts,
                          const ColorFunction& get_color,
                          const PoSize& item_pos,
                          const PoSize& item_size) override;

        void stroke_polyline(const std::vector<PoSize>& pts,
                             bool closed,
                             const ColorFunction& get_color,
                             uint32_t width,
                             const PoSize& item_pos,
                             const PoSize& item_size) override;

        void read_pixels(RGBA8888& out) override;

        // 诊断信息
        [[nodiscard]] uint64_t getDrawCalls() const;
        [[nodiscard]] uint64_t getVertexCount() const;
        [[nodiscard]] static const char* getLastError() { return s_last_error; }

    private:
        GLCanvas() = default;
        GLCanvas(const GLCanvas&) = delete;
        GLCanvas& operator=(const GLCanvas&) = delete;

        struct Impl;
        std::unique_ptr<Impl> _impl;

        static inline const char* s_last_error = nullptr;
    };
} // namespace xII::Screen

#endif // XII_BACKEND_GL
