#include "GLCanvas.h"

#if XII_BACKEND_GL

#include "GLFunctions.h"

#include <SDL3/SDL.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>

namespace xII::Screen {

using namespace GL;

namespace {

struct GLVertex {
    float x, y;
    unsigned char r, g, b, a;
};
static_assert(sizeof(GLVertex) == 12, "GLVertex must be tightly packed");

// ------------------------------------------------------------------
// 着色器
// ------------------------------------------------------------------
const char* k_solid_vs = R"(#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec4 aColor;
uniform vec2 uViewport;
out vec4 vColor;
void main() {
    // 像素坐标（y 向下）-> NDC（y 向上）
    vec2 ndc = vec2(aPos.x / uViewport.x * 2.0 - 1.0,
                    1.0 - aPos.y / uViewport.y * 2.0);
    gl_Position = vec4(ndc, 0.0, 1.0);
    vColor = aColor;
}
)";

const char* k_solid_fs = R"(#version 330 core
in vec4 vColor;
out vec4 FragColor;
void main() {
    FragColor = vColor;
}
)";

const char* k_tex_vs = R"(#version 330 core
layout(location = 0) in vec2 aPos;   // 直接是 NDC
out vec2 vUV;
void main() {
    vUV = aPos * 0.5 + 0.5;
    gl_Position = vec4(aPos, 0.0, 1.0);
}
)";

const char* k_tex_fs = R"(#version 330 core
in vec2 vUV;
uniform sampler2D uTex;
out vec4 FragColor;
void main() {
    FragColor = texture(uTex, vUV);
}
)";

GLuint compile_shader(GLenum type, const char* src) {
    const GLuint shader = CreateShader(type);
    if (shader == 0) {
        std::fprintf(stderr, "[xII GL] glCreateShader failed\n");
        return 0;
    }

    ShaderSource(shader, 1, &src, nullptr);
    CompileShader(shader);

    GLint ok = 0;
    GetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (ok == 0) {
        char log[2048] = {};
        GLsizei len = 0;
        GetShaderInfoLog(shader, static_cast<GLsizei>(sizeof(log) - 1), &len, log);
        std::fprintf(stderr, "[xII GL] shader compile failed:\n%s\n", log);
        DeleteShader(shader);
        return 0;
    }

    return shader;
}

GLuint link_program(const char* vs_src, const char* fs_src) {
    const GLuint vs = compile_shader(GL_VERTEX_SHADER, vs_src);
    if (vs == 0) return 0;

    const GLuint fs = compile_shader(GL_FRAGMENT_SHADER, fs_src);
    if (fs == 0) {
        DeleteShader(vs);
        return 0;
    }

    const GLuint prog = CreateProgram();
    AttachShader(prog, vs);
    AttachShader(prog, fs);
    LinkProgram(prog);

    DeleteShader(vs);
    DeleteShader(fs);

    GLint ok = 0;
    GetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (ok == 0) {
        char log[2048] = {};
        GLsizei len = 0;
        GetProgramInfoLog(prog, static_cast<GLsizei>(sizeof(log) - 1), &len, log);
        std::fprintf(stderr, "[xII GL] program link failed:\n%s\n", log);
        DeleteProgram(prog);
        return 0;
    }

    return prog;
}

// CPU 端 Line::draw_line 为了让偶数线宽对称，会把整条线沿“主轴”平移
// (width-1)/2 - floor(width/2) 个像素（例如 width=2 时是 -0.5）。
// GPU 必须使用同样的偏移，否则奇偶线宽会差半个像素。
float stroke_bias(uint32_t width) {
    return (static_cast<float>(width) - 1.0f) * 0.5f
         - std::floor(static_cast<float>(width) * 0.5f);
}

} // namespace

// ----------------------------------------------------------------------
// Impl
// ----------------------------------------------------------------------
struct GLCanvas::Impl {
    SDL_Window* window = nullptr;
    int width = 0;
    int height = 0;

    GLuint vao = 0;
    GLuint vbo = 0;

    GLuint prog_solid = 0;
    GLuint prog_tex = 0;
    GLint u_viewport = -1;
    GLint u_tex = -1;

    GLuint scratch_tex = 0;
    GLuint scratch_fbo = 0;
    int scratch_w = 0;
    int scratch_h = 0;

    uint64_t draw_calls = 0;
    uint64_t vertex_count = 0;

    std::vector<GLVertex> verts;

    void bind_state() {
        BindVertexArray(vao);
        BindBuffer(GL_ARRAY_BUFFER, vbo);
    }

    void upload(const std::vector<GLVertex>& v) {
        BufferData(GL_ARRAY_BUFFER,
                   static_cast<GLsizeiptr>(v.size() * sizeof(GLVertex)),
                   v.data(), GL_STREAM_DRAW);
    }

    void draw_solid(const std::vector<GLVertex>& v, bool blend) {
        if (v.empty()) return;

        UseProgram(prog_solid);
        Uniform2f(u_viewport, static_cast<float>(width), static_cast<float>(height));
        bind_state();
        upload(v);

        if (blend) {
            Enable(GL_BLEND);
            BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        } else {
            Disable(GL_BLEND);
        }

        DrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(v.size()));
        ++draw_calls;
        vertex_count += v.size();
    }

    bool ensure_scratch() {
        if (scratch_fbo != 0 && scratch_w == width && scratch_h == height)
            return true;

        if (scratch_tex != 0) { DeleteTextures(1, &scratch_tex); scratch_tex = 0; }
        if (scratch_fbo != 0) { DeleteFramebuffers(1, &scratch_fbo); scratch_fbo = 0; }

        GenTextures(1, &scratch_tex);
        BindTexture(GL_TEXTURE_2D, scratch_tex);
        TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0,
                   GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        GenFramebuffers(1, &scratch_fbo);
        BindFramebuffer(GL_FRAMEBUFFER, scratch_fbo);
        FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                             GL_TEXTURE_2D, scratch_tex, 0);

        const GLenum status = CheckFramebufferStatus(GL_FRAMEBUFFER);
        BindFramebuffer(GL_FRAMEBUFFER, 0);

        if (status != GL_FRAMEBUFFER_COMPLETE) {
            std::fprintf(stderr, "[xII GL] scratch framebuffer incomplete: 0x%x\n",
                         status);
            return false;
        }

        scratch_w = width;
        scratch_h = height;
        return true;
    }

    // 把 scratch 以 SRC_ALPHA / ONE_MINUS_SRC_ALPHA 合成回默认帧缓冲，
    // 得到与 CPU 相同的“每像素只混合一次”的语义。
    void composite_scratch() {
        BindFramebuffer(GL_FRAMEBUFFER, 0);
        Viewport(0, 0, width, height);

        Enable(GL_BLEND);
        BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        UseProgram(prog_tex);
        Uniform1i(u_tex, 0);
        ActiveTexture(GL_TEXTURE0);
        BindTexture(GL_TEXTURE_2D, scratch_tex);

        static const GLVertex quad[6] = {
            {-1.0f, -1.0f, 255, 255, 255, 255},
            { 1.0f, -1.0f, 255, 255, 255, 255},
            { 1.0f,  1.0f, 255, 255, 255, 255},
            {-1.0f, -1.0f, 255, 255, 255, 255},
            { 1.0f,  1.0f, 255, 255, 255, 255},
            {-1.0f,  1.0f, 255, 255, 255, 255},
        };

        bind_state();
        upload(std::vector<GLVertex>(quad, quad + 6));
        DrawArrays(GL_TRIANGLES, 0, 6);

        ++draw_calls;
        vertex_count += 6;
    }
};

// ----------------------------------------------------------------------
// GLCanvas
// ----------------------------------------------------------------------
GLCanvas::~GLCanvas() {
    if (!_impl) return;

    if (_impl->scratch_tex != 0) DeleteTextures(1, &_impl->scratch_tex);
    if (_impl->scratch_fbo != 0) DeleteFramebuffers(1, &_impl->scratch_fbo);
    if (_impl->prog_solid != 0) DeleteProgram(_impl->prog_solid);
    if (_impl->prog_tex != 0) DeleteProgram(_impl->prog_tex);
    if (_impl->vbo != 0) DeleteBuffers(1, &_impl->vbo);
    if (_impl->vao != 0) DeleteVertexArrays(1, &_impl->vao);
}

std::unique_ptr<GLCanvas> GLCanvas::create(SDL_Window* window, const PoSize& size) {
    const char* missing = nullptr;
    if (!GL::load(&missing)) {
        s_last_error = missing != nullptr ? missing : "SDL_GL_GetProcAddress failed";
        return nullptr;
    }

    auto canvas = std::unique_ptr<GLCanvas>(new GLCanvas());
    canvas->_impl = std::make_unique<Impl>();
    Impl& impl = *canvas->_impl;

    impl.window = window;
    impl.width = static_cast<int>(size.x);
    impl.height = static_cast<int>(size.y);

    if (impl.width <= 0 || impl.height <= 0) {
        s_last_error = "invalid canvas size";
        return nullptr;
    }

    impl.prog_solid = link_program(k_solid_vs, k_solid_fs);
    if (impl.prog_solid == 0) {
        s_last_error = "solid program link failed";
        return nullptr;
    }

    impl.prog_tex = link_program(k_tex_vs, k_tex_fs);
    if (impl.prog_tex == 0) {
        s_last_error = "textured program link failed";
        return nullptr;
    }

    impl.u_viewport = GetUniformLocation(impl.prog_solid, "uViewport");
    impl.u_tex = GetUniformLocation(impl.prog_tex, "uTex");

    GenVertexArrays(1, &impl.vao);
    GenBuffers(1, &impl.vbo);
    impl.bind_state();

    EnableVertexAttribArray(0);
    VertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE,
                        static_cast<GLsizei>(sizeof(GLVertex)),
                        reinterpret_cast<const void*>(0));
    EnableVertexAttribArray(1);
    VertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE,
                        static_cast<GLsizei>(sizeof(GLVertex)),
                        reinterpret_cast<const void*>(sizeof(float) * 2));

    PixelStorei(GL_UNPACK_ALIGNMENT, 1);
    Disable(GL_DEPTH_TEST);
    Disable(GL_CULL_FACE);

    if (!impl.ensure_scratch()) {
        s_last_error = "scratch framebuffer incomplete";
        return nullptr;
    }

    return canvas;
}

void GLCanvas::begin_frame(const RGBA8888::Pixel& clear_color) {
    Impl& impl = *_impl;

    BindFramebuffer(GL_FRAMEBUFFER, 0);
    Viewport(0, 0, impl.width, impl.height);

    ClearColor(clear_color.R / 255.0f, clear_color.G / 255.0f,
               clear_color.B / 255.0f, clear_color.A / 255.0f);
    Clear(GL_COLOR_BUFFER_BIT);

    Disable(GL_BLEND);
    Disable(GL_DEPTH_TEST);
    Disable(GL_CULL_FACE);
}

void GLCanvas::end_frame() {
    // 当前后端是立即提交模式，无需延迟刷新。
}

void GLCanvas::fill_polygon(const std::vector<PoSize>& pts,
                            const ColorFunction& get_color,
                            const PoSize& item_pos,
                            const PoSize& item_size) {
    if (pts.size() < 3) return;

    Impl& impl = *_impl;

    int64_t max_x = pts[0].x;
    for (const auto& p : pts)
        max_x = std::max(max_x, p.x);

    // CPU 端 Fill::draw_fill 的扫描线约定：
    //   y：边与扫描线求交用半开区间 [ymin, ymax)，所以 max_y 那一行不会被填充；
    //   x：[x_start, x_end] 闭区间，所以 max_x 那一列会被填充。
    // GPU 必须镜像这个约定，否则每个多边形会多出一行像素。
    // （这个 1 像素的不对称是 CPU 侧既有行为，此处只做对齐，不擅自改变。）
    std::vector<GLVertex> fan;
    fan.reserve(pts.size());

    bool translucent = false;
    const PoSize screen_size{impl.width, impl.height};

    for (const auto& p : pts) {
        float x = static_cast<float>(p.x);
        float y = static_cast<float>(p.y);
        if (p.x == max_x) x += 1.0f;   // 闭区间：覆盖到 max_x 这一列

        const RGBA8888::Pixel c = get_color(DrawContext{p, screen_size, item_pos, item_size});
        if (c.A < 255) translucent = true;

        fan.push_back(GLVertex{x, y, c.R, c.G, c.B, c.A});
    }

    // 凸多边形：以 0 号顶点为中心做扇形三角化（Rectangle 等形状都是凸的）
    impl.verts.clear();
    impl.verts.reserve((fan.size() - 2) * 3);
    for (size_t i = 1; i + 1 < fan.size(); ++i) {
        impl.verts.push_back(fan[0]);
        impl.verts.push_back(fan[i]);
        impl.verts.push_back(fan[i + 1]);
    }

    impl.draw_solid(impl.verts, translucent);
}

void GLCanvas::stroke_polyline(const std::vector<PoSize>& pts,
                               bool closed,
                               const ColorFunction& get_color,
                               uint32_t width,
                               const PoSize& item_pos,
                               const PoSize& item_size) {
    if (width == 0 || pts.size() < (closed ? 3u : 2u)) return;

    Impl& impl = *_impl;

    const float half_width = static_cast<float>(width) * 0.5f - 0.001f;
    const float bias = stroke_bias(width);
    const PoSize screen_size{impl.width, impl.height};

    impl.verts.clear();
    bool translucent = false;

    const size_t n = pts.size();
    const size_t seg_count = closed ? n : n - 1;

    for (size_t i = 0; i < seg_count; ++i) {
        const PoSize& a = pts[i];
        const PoSize& b = pts[(i + 1) % n];

        // 端点用“像素中心”坐标（下标 + 0.5）
        float ax = static_cast<float>(a.x) + 0.5f;
        float ay = static_cast<float>(a.y) + 0.5f;
        float bx = static_cast<float>(b.x) + 0.5f;
        float by = static_cast<float>(b.y) + 0.5f;

        const bool horizontal = std::abs(static_cast<int64_t>(b.x) - a.x)
                             >= std::abs(static_cast<int64_t>(b.y) - a.y);
        if (horizontal) { ay += bias; by += bias; }
        else            { ax += bias; bx += bias; }

        float dx = bx - ax;
        float dy = by - ay;
        float len = std::sqrt(dx * dx + dy * dy);

        if (len < 1e-6f) {
            // 退化线段：CPU 会画一条宽 width 的竖条，这里保持一致
            if (horizontal) { dx = 1.0f; dy = 0.0f; }
            else            { dx = 0.0f; dy = 1.0f; }
            len = 1.0f;
        }

        const float ux = dx / len;
        const float uy = dy / len;

        // 两端各外扩半个像素，使端点像素也被覆盖（与 Bresenham 的闭合端点一致）
        const float x0 = ax - ux * 0.5f;
        const float y0 = ay - uy * 0.5f;
        const float x1 = bx + ux * 0.5f;
        const float y1 = by + uy * 0.5f;

        // 法线
        const float nx = -uy * half_width;
        const float ny = ux * half_width;

        const RGBA8888::Pixel c0 = get_color(DrawContext{a, screen_size, item_pos, item_size});
        const RGBA8888::Pixel c1 = get_color(DrawContext{b, screen_size, item_pos, item_size});
        if (c0.A < 255 || c1.A < 255) translucent = true;

        const GLVertex v0{x0 + nx, y0 + ny, c0.R, c0.G, c0.B, c0.A};
        const GLVertex v1{x0 - nx, y0 - ny, c0.R, c0.G, c0.B, c0.A};
        const GLVertex v2{x1 + nx, y1 + ny, c1.R, c1.G, c1.B, c1.A};
        const GLVertex v3{x1 - nx, y1 - ny, c1.R, c1.G, c1.B, c1.A};

        impl.verts.push_back(v0);
        impl.verts.push_back(v1);
        impl.verts.push_back(v2);

        impl.verts.push_back(v2);
        impl.verts.push_back(v1);
        impl.verts.push_back(v3);
    }

    if (impl.verts.empty()) return;

    if (!translucent) {
        // 不透明：直接覆盖，等价于 CPU 的 dst = color
        impl.draw_solid(impl.verts, false);
        return;
    }

    // 半透明：先画到 scratch（关闭混合，每个像素只被写一次），
    // 再整体合成，避免拐角/线宽重叠处 alpha 被叠加多次。
    if (!impl.ensure_scratch()) return;

    BindFramebuffer(GL_FRAMEBUFFER, impl.scratch_fbo);
    Viewport(0, 0, impl.width, impl.height);
    ClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    Clear(GL_COLOR_BUFFER_BIT);

    impl.draw_solid(impl.verts, false);
    impl.composite_scratch();
}

void GLCanvas::read_pixels(RGBA8888& out) {
    Impl& impl = *_impl;

    BindFramebuffer(GL_FRAMEBUFFER, 0);
    PixelStorei(GL_PACK_ALIGNMENT, 1);

    std::vector<unsigned char> buffer(
        static_cast<size_t>(impl.width) * static_cast<size_t>(impl.height) * 4u);

    ReadPixels(0, 0, impl.width, impl.height, GL_RGBA, GL_UNSIGNED_BYTE, buffer.data());

    if (out.getSize().x != impl.width || out.getSize().y != impl.height)
        out.resize(PoSize{impl.width, impl.height});

    // GL 的行序是自下而上，需要翻转；字节序是 R,G,B,A
    for (int y = 0; y < impl.height; ++y) {
        const unsigned char* row =
            buffer.data() + static_cast<size_t>(impl.height - 1 - y) * impl.width * 4u;
        for (int x = 0; x < impl.width; ++x) {
            const unsigned char* s = row + static_cast<size_t>(x) * 4u;
            RGBA8888::Pixel& d = out.at(PoSize{x, y});
            d.R = s[0];
            d.G = s[1];
            d.B = s[2];
            d.A = s[3];
        }
    }
}

uint64_t GLCanvas::getDrawCalls() const { return _impl ? _impl->draw_calls : 0; }
uint64_t GLCanvas::getVertexCount() const { return _impl ? _impl->vertex_count : 0; }

} // namespace xII::Screen

#endif // XII_BACKEND_GL
