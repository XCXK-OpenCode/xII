//
// OpenGL 3.3 core 函数装载器
//
// 不包含任何系统 GL 头文件，所有入口点都通过 SDL_GL_GetProcAddress 获取，
// 这样同一个二进制既可以在有 GL 的机器上跑，也不会在编译期强依赖 GL 开发包。
//

#pragma once

#include "Backend.h"

#if XII_BACKEND_GL

#include <cstddef>
#include <cstdint>

namespace xII::Screen::GL {
    using GLenum      = unsigned int;
    using GLuint      = unsigned int;
    using GLint       = int;
    using GLsizei     = int;
    using GLfloat     = float;
    using GLchar      = char;
    using GLboolean   = unsigned char;
    using GLbitfield  = unsigned int;
    using GLubyte     = unsigned char;
    using GLsizeiptr  = std::ptrdiff_t;
    using GLintptr    = std::ptrdiff_t;

    // ------------------------------------------------------------------
    // 常量（只列用到的那部分，数值来自 GL 规范）
    // ------------------------------------------------------------------
    inline constexpr GLenum GL_FALSE                      = 0;
    inline constexpr GLenum GL_TRUE                       = 1;
    inline constexpr GLenum GL_NO_ERROR                   = 0;

    inline constexpr GLbitfield GL_COLOR_BUFFER_BIT       = 0x00004000;
    inline constexpr GLbitfield GL_DEPTH_TEST             = 0x0B71;
    inline constexpr GLbitfield GL_BLEND                  = 0x0BE2;
    inline constexpr GLbitfield GL_CULL_FACE              = 0x0B44;

    inline constexpr GLenum GL_SRC_ALPHA                  = 0x0302;
    inline constexpr GLenum GL_ONE_MINUS_SRC_ALPHA        = 0x0303;
    inline constexpr GLenum GL_ONE                        = 1;

    inline constexpr GLenum GL_TRIANGLES                  = 0x0004;
    inline constexpr GLenum GL_TRIANGLE_FAN               = 0x0006;

    inline constexpr GLenum GL_ARRAY_BUFFER               = 0x8892;
    inline constexpr GLenum GL_STREAM_DRAW                = 0x88E0;
    inline constexpr GLenum GL_DYNAMIC_DRAW               = 0x88E8;

    inline constexpr GLenum GL_FLOAT                      = 0x1406;
    inline constexpr GLenum GL_UNSIGNED_BYTE              = 0x1401;

    inline constexpr GLenum GL_VERTEX_SHADER              = 0x8B31;
    inline constexpr GLenum GL_FRAGMENT_SHADER            = 0x8B30;
    inline constexpr GLenum GL_COMPILE_STATUS             = 0x8B81;
    inline constexpr GLenum GL_LINK_STATUS                = 0x8B82;
    inline constexpr GLenum GL_INFO_LOG_LENGTH            = 0x8B84;

    inline constexpr GLenum GL_TEXTURE_2D                 = 0x0DE1;
    inline constexpr GLenum GL_TEXTURE0                   = 0x84C0;
    inline constexpr GLenum GL_TEXTURE_MIN_FILTER         = 0x2801;
    inline constexpr GLenum GL_TEXTURE_MAG_FILTER         = 0x2800;
    inline constexpr GLenum GL_TEXTURE_WRAP_S             = 0x2802;
    inline constexpr GLenum GL_TEXTURE_WRAP_T             = 0x2803;
    inline constexpr GLenum GL_NEAREST                    = 0x2600;
    inline constexpr GLenum GL_CLAMP_TO_EDGE              = 0x812F;

    inline constexpr GLenum GL_RGBA                       = 0x1908;
    inline constexpr GLenum GL_RGBA8                      = 0x8058;

    inline constexpr GLenum GL_UNPACK_ALIGNMENT           = 0x0CF5;
    inline constexpr GLenum GL_PACK_ALIGNMENT             = 0x0D05;

    inline constexpr GLenum GL_FRAMEBUFFER                = 0x8D40;
    inline constexpr GLenum GL_COLOR_ATTACHMENT0          = 0x8CE0;
    inline constexpr GLenum GL_FRAMEBUFFER_COMPLETE       = 0x8CD5;

    // ------------------------------------------------------------------
    // 入口点声明
    // ------------------------------------------------------------------
#define XII_GL_FUNCTION_LIST(X)                                                              \
    X(void,   Enable,                (GLenum))                                               \
    X(void,   Disable,               (GLenum))                                               \
    X(void,   BlendFunc,             (GLenum, GLenum))                                       \
    X(void,   ClearColor,            (GLfloat, GLfloat, GLfloat, GLfloat))                   \
    X(void,   Clear,                 (GLbitfield))                                           \
    X(void,   Viewport,              (GLint, GLint, GLsizei, GLsizei))                        \
    X(void,   GenVertexArrays,       (GLsizei, GLuint*))                                     \
    X(void,   BindVertexArray,       (GLuint))                                               \
    X(void,   DeleteVertexArrays,    (GLsizei, const GLuint*))                               \
    X(void,   GenBuffers,            (GLsizei, GLuint*))                                     \
    X(void,   BindBuffer,            (GLenum, GLuint))                                       \
    X(void,   BufferData,            (GLenum, GLsizeiptr, const void*, GLenum))              \
    X(void,   DeleteBuffers,         (GLsizei, const GLuint*))                               \
    X(void,   EnableVertexAttribArray, (GLuint))                                             \
    X(void,   VertexAttribPointer,   (GLuint, GLint, GLenum, GLboolean, GLsizei, const void*))\
    X(GLuint, CreateShader,          (GLenum))                                               \
    X(void,   ShaderSource,          (GLuint, GLsizei, const GLchar* const*, const GLint*))  \
    X(void,   CompileShader,         (GLuint))                                               \
    X(void,   GetShaderiv,           (GLuint, GLenum, GLint*))                               \
    X(void,   GetShaderInfoLog,      (GLuint, GLsizei, GLsizei*, GLchar*))                   \
    X(void,   DeleteShader,          (GLuint))                                               \
    X(GLuint, CreateProgram,         (void))                                                 \
    X(void,   AttachShader,          (GLuint, GLuint))                                       \
    X(void,   LinkProgram,           (GLuint))                                               \
    X(void,   GetProgramiv,          (GLuint, GLenum, GLint*))                               \
    X(void,   GetProgramInfoLog,     (GLuint, GLsizei, GLsizei*, GLchar*))                   \
    X(void,   DeleteProgram,         (GLuint))                                               \
    X(void,   UseProgram,            (GLuint))                                               \
    X(GLint,  GetUniformLocation,    (GLuint, const GLchar*))                                \
    X(void,   Uniform2f,             (GLint, GLfloat, GLfloat))                              \
    X(void,   Uniform1i,             (GLint, GLint))                                         \
    X(void,   GenTextures,           (GLsizei, GLuint*))                                     \
    X(void,   BindTexture,           (GLenum, GLuint))                                       \
    X(void,   TexImage2D,            (GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const void*)) \
    X(void,   TexParameteri,         (GLenum, GLenum, GLint))                                \
    X(void,   DeleteTextures,        (GLsizei, const GLuint*))                               \
    X(void,   ActiveTexture,         (GLenum))                                               \
    X(void,   GenFramebuffers,       (GLsizei, GLuint*))                                     \
    X(void,   BindFramebuffer,       (GLenum, GLuint))                                       \
    X(void,   FramebufferTexture2D,  (GLenum, GLenum, GLenum, GLuint, GLint))                \
    X(GLenum, CheckFramebufferStatus,(GLenum))                                               \
    X(void,   DeleteFramebuffers,    (GLsizei, const GLuint*))                               \
    X(void,   DrawArrays,            (GLenum, GLint, GLsizei))                               \
    X(void,   ReadPixels,            (GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, void*))\
    X(void,   PixelStorei,           (GLenum, GLint))                                        \
    X(void,   Finish,                (void))                                                 \
    X(GLenum, GetError,              (void))                                                 \
    X(const GLubyte*, GetString,     (GLenum))

#define XII_GL_DECLARE(ret, name, args)                \
    using PFN_##name = ret (*) args;                   \
    extern PFN_##name name;

    XII_GL_FUNCTION_LIST(XII_GL_DECLARE)

#undef XII_GL_DECLARE

    // 通过 SDL_GL_GetProcAddress 装载全部入口点。
    // 失败时返回 false，并把缺失的函数名写入 out_missing（可为 nullptr）。
    bool load(const char** out_missing = nullptr);

    void unload();

} // namespace xII::Screen::GL

#endif // XII_BACKEND_GL
