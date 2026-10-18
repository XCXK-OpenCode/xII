#include "GLFunctions.h"

#if XII_BACKEND_GL

#include <SDL3/SDL.h>

namespace xII::Screen::GL {

#define XII_GL_DEFINE(ret, name, args) PFN_##name name = nullptr;
    XII_GL_FUNCTION_LIST(XII_GL_DEFINE)
#undef XII_GL_DEFINE

    bool load(const char** out_missing) {
        bool ok = true;
        const char* missing = nullptr;

        auto fetch = [&](const char* name) -> SDL_FunctionPointer {
            return SDL_GL_GetProcAddress(name);
        };

#define XII_GL_LOAD(ret, name, args)                                       \
        do {                                                               \
            name = reinterpret_cast<PFN_##name>(fetch("gl" #name));        \
            if (name == nullptr && ok) {                                   \
                ok = false;                                                \
                missing = "gl" #name;                                      \
            }                                                              \
        } while (false);

        XII_GL_FUNCTION_LIST(XII_GL_LOAD)

#undef XII_GL_LOAD

        if (out_missing != nullptr)
            *out_missing = missing;

        return ok;
    }

    void unload() {
#define XII_GL_RESET(ret, name, args) name = nullptr;
        XII_GL_FUNCTION_LIST(XII_GL_RESET)
#undef XII_GL_RESET
    }

} // namespace xII::Screen::GL

#endif // XII_BACKEND_GL
