//
// GPU 后端示例：同一份场景可以在 CPU 光栅化与 OpenGL 之间切换
//
//   ./example_gl_renderer          # 默认 CPU
//   ./example_gl_renderer --gl     # OpenGL（需要 -DXII_ENABLE_OPENGL=ON 编译）
//
// 编译期开关见 src/Screen/Backend.h 与顶层 CMakeLists.txt：
//   cmake -DXII_ENABLE_OPENGL=ON ..
//

#include <atomic>
#include <chrono>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <memory>
#include <thread>
#include <vector>

#include <Camera/Standard.h>
#include <Item/General.h>
#include <Item/Renderer/Fill.h>
#include <Item/Renderer/Line.h>
#include <Item/Renderer/Rainbow.h>
#include <Item/Renderer/SetAlpha.h>
#include <Item/Renderer/SetColor.h>
#include <Item/Renderer/SetWidth.h>
#include <Item/Shape/Rectangle.h>
#include <Screen/Backend.h>
#include <Screen/Screen.h>
#if XII_BACKEND_GL
#   include <Screen/GLCanvas.h>
#endif
#include <Space/Offset.h>
#include <Space/OffsetPoint.h>
#include <Space/Point.h>
#include <Time/OffsetPoint.h>
#include <Time/Point.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_video.h>

using namespace std;
using namespace xII;

const int SCREEN_WIDTH = 1280;
const int SCREEN_HEIGHT = 720;

std::atomic<size_t> frame_count{0};
atomic<bool> isDone(false);

void fps_ptf() {
    using namespace chrono;

    auto   last_time  = steady_clock::now();
    size_t last_count = 0;

    while (!isDone.load(std::memory_order_relaxed)) {
        this_thread::sleep_for(milliseconds(100));

        auto now     = steady_clock::now();
        auto elapsed = duration_cast<milliseconds>(now - last_time).count();

        if (elapsed >= 1000) {
            size_t cur = frame_count.load(std::memory_order_relaxed);
            double fps = (cur - last_count) * 1000.0 / elapsed;

            cout << "\rFPS: " << fixed << setprecision(1) << fps
                 << "   frames: " << cur << "        " << flush;

            last_count = cur;
            last_time  = now;
        }
    }
    cout << endl;
}

int main(int argc, char** argv) {
    bool want_gpu = false;
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--gl") == 0)  want_gpu = true;
        if (strcmp(argv[i], "--cpu") == 0) want_gpu = false;
    }

    cout << "GL Renderer Example  (cpu=" << Screen::Backend::cpu
         << " gl=" << Screen::Backend::gl << ")" << endl;

#if !XII_BACKEND_GL
    if (want_gpu) {
        cerr << "This binary was built without the OpenGL backend. "
                "Reconfigure with -DXII_ENABLE_OPENGL=ON." << endl;
        return 1;
    }
#endif

    thread fps_pt(fps_ptf);

    auto root_space_point = std::make_shared<Space::Point>();
    auto root_time_point = std::make_shared<Time::Point>();

    auto camera_time_point = std::make_shared<Time::OffsetPoint>(root_time_point, Time::Offset(0));
    auto camera_center = std::make_shared<Space::OffsetPoint>(root_space_point, Space::Offset(0, 0));

    auto camera = Camera::Standard(
        Item::Shape::Rectangle(camera_center, 1920, 1080),
        {SCREEN_WIDTH, SCREEN_HEIGHT},
        std::make_shared<Item::Base>(),
        camera_time_point
    );

    // 三个半透明矩形（Fill）
    auto add_fill = [&](float x, float y, Screen::Screen::Pixel color) {
        camera.getObject()->addItem(std::make_shared<Item::General>(
            std::make_shared<Item::Shape::Rectangle>(
                std::make_shared<Space::OffsetPoint>(root_space_point, Space::Offset(x, y)),
                500, 500),
            std::vector<std::shared_ptr<Item::Renderer::Base>>{
                std::make_shared<Item::Renderer::SetColor>(color),
                std::make_shared<Item::Renderer::Fill>()
            }));
    };

    add_fill(-200, -200, Screen::Screen::Pixel{255, 255, 63, 63});
    add_fill(0, 0,     Screen::Screen::Pixel{255, 63, 255, 63});
    add_fill(200, 200, Screen::Screen::Pixel{255, 63, 63, 255});

    // 三个描边（Line）：彩虹 + 半透明
    auto add_outline = [&](float cx, float cy, float w, float h,
                           uint8_t alpha, Screen::Screen::Pixel color,
                           uint32_t width, bool rainbow) {
        auto item = std::make_shared<Item::General>(
            std::make_shared<Item::Shape::Rectangle>(
                std::make_shared<Space::OffsetPoint>(root_space_point, Space::Offset(cx, cy)),
                w, h));
        item->getRenderersR().push_back(std::make_shared<Item::Renderer::SetAlpha>(alpha));
        if (rainbow)
            item->getRenderersR().push_back(std::make_shared<Item::Renderer::Rainbow1>());
        else
            item->getRenderersR().push_back(std::make_shared<Item::Renderer::SetColor>(color));
        item->getRenderersR().push_back(std::make_shared<Item::Renderer::SetWidth>(width));
        item->getRenderersR().push_back(std::make_shared<Item::Renderer::Line>());
        camera.getObject()->addItem(item);
    };

    add_outline(0, 0, 300, 300, 255, Screen::Screen::Pixel{255, 255, 255, 0}, 2, true);
    add_outline(-400, 200, 400, 300, 96, Screen::Screen::Pixel{255, 0, 255, 255}, 3, false);
    add_outline(400, -300, 500, 200, 64, Screen::Screen::Pixel{255, 255, 0, 255}, 6, false);

    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return -1;
    }

    if (want_gpu) {
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    }

    SDL_Window* gWindow = SDL_CreateWindow("GL Renderer Example",
                                           SCREEN_WIDTH, SCREEN_HEIGHT,
                                           want_gpu ? SDL_WINDOW_OPENGL : 0);
    if (!gWindow) {
        SDL_Log("can not create window:%s", SDL_GetError());
        return -1;
    }

#if XII_BACKEND_GL
    std::unique_ptr<Screen::GLCanvas> gl_canvas;
#endif
    SDL_GLContext gl_context = nullptr;
    SDL_Surface* surface = nullptr;
    SDL_Surface* gWinSfc = nullptr;

    if (want_gpu) {
#if XII_BACKEND_GL
        gl_context = SDL_GL_CreateContext(gWindow);
        if (!gl_context) {
            std::cerr << "SDL_GL_CreateContext failed: " << SDL_GetError() << std::endl;
            return -1;
        }
        SDL_GL_SetSwapInterval(0);

        gl_canvas = Screen::GLCanvas::create(gWindow, {SCREEN_WIDTH, SCREEN_HEIGHT});
        if (!gl_canvas) {
            std::cerr << "GLCanvas::create failed: " << Screen::GLCanvas::getLastError() << std::endl;
            return -1;
        }
        camera.setCanvas(gl_canvas.get());
        cout << "GPU backend: " << gl_canvas->name() << endl;
#endif
    } else {
        gWinSfc = SDL_GetWindowSurface(gWindow);
        if (!gWinSfc) return -1;

        surface = SDL_CreateSurfaceFrom(
            camera.getScreenR().getSize().x,
            camera.getScreenR().getSize().y,
            SDL_PIXELFORMAT_BGRA8888,
            camera.getScreenR().getData(),
            camera.getScreenR().getSize().x * 4);
        if (!surface) {
            std::cerr << "Failed to create surface: " << SDL_GetError() << std::endl;
            return -1;
        }

        // Buffer is already composited and opaque -> skip SDL's per-pixel blending
        SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_NONE);
    }

    while (!isDone) {
        static SDL_Event ev = {};

        while (SDL_PollEvent(&ev) != 0)
            if (ev.type == SDL_EVENT_QUIT)
                isDone = true;

        if (want_gpu) {
            camera.Shoot();
            SDL_GL_SwapWindow(gWindow);
        } else {
            camera.getScreenR().clear();
            camera.Shoot();
            SDL_BlitSurface(surface, NULL, gWinSfc, NULL);
            SDL_UpdateWindowSurface(gWindow);
        }

        frame_count.fetch_add(1, std::memory_order_relaxed);
    }

    isDone.store(true, std::memory_order_relaxed);
    fps_pt.join();

#if XII_BACKEND_GL
    gl_canvas.reset();
#endif
    if (gl_context) SDL_GL_DestroyContext(gl_context);
    if (surface) SDL_DestroySurface(surface);
    SDL_DestroyWindow(gWindow);
    SDL_Quit();

    return 0;
}
