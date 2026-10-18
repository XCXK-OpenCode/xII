#include <iostream>
#include <memory>
#include <atomic>
#include <thread>
#include <chrono>
#include <iomanip>
#include <cstring>
#include <cstdlib>
#include <algorithm>
#include <cstdint>

#include "Camera/Standard.h"
#include "Camera/Outline.h"
#include "Item/Base.h"
#include "Item/General.h"
#include "Item/Renderer/Base.h"
#include "Item/Shape/Rectangle.h"
#include "SDL3/SDL.h"
#include "SDL3/SDL_video.h"
#include "Screen/Screen.h"
#include "Screen/Backend.h"
#if XII_BACKEND_GL
#   include "Screen/GLCanvas.h"
#endif
#include "Space/Offset.h"
#include "Space/Point.h"
#include "Space/OffsetPoint.h"
#include "Item/Renderer/Line.h"
#include "Item/Renderer/SetColor.h"
#include "Item/Renderer/SetWidth.h"
#include "Item/Renderer/Rainbow.h"
#include "Item/Renderer/Fill.h"
#include "Time/Point.h"
#include "Time/OffsetPoint.h"
#include "Item/Renderer/SetAlpha.h"
#include "Item/Renderer/Curves.h"
#include "Item/Renderer/Fadeout.h"
#include "Item/Renderer/Fadein.h"

//屏幕尺寸常量
const int SCREEN_WIDTH = 1280;
const int SCREEN_HEIGHT = 720;

using namespace xII;

//帧监控
std::atomic<size_t> frame_count{0};
std::atomic<bool> isDone{false};

void fps_ptf() {
    using namespace std::chrono;

    auto   last_time  = steady_clock::now(); // Last time
    size_t last_count = 0;                   // Last frame count

    while (!isDone.load(std::memory_order_relaxed)) {
        // check every 100ms
        std::this_thread::sleep_for(milliseconds(100));

        // check if 1 second has passed
        auto now     = steady_clock::now();
        auto elapsed = duration_cast<milliseconds>(now - last_time).count();

        if (elapsed >= 1000) {
            size_t cur = frame_count.load(std::memory_order_relaxed);
            double fps = (cur - last_count) * 1000.0 / elapsed;

            std::cout << "\rFPS: " << std::fixed << std::setprecision(1)
                      << fps
                      << "   frames: " << cur
                      << "        " << std::flush;

            last_count = cur;
            last_time  = now;
        }
    }
    // Flush output
    std::cout << std::endl;
}

int main(int argc, char** argv) {
    std::cout << "Test 20261018-173742" << std::endl;

    // ---- 选择后端：--gl / --cpu ----
    bool want_gpu = false;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--gl") == 0)  want_gpu = true;
        if (std::strcmp(argv[i], "--cpu") == 0) want_gpu = false;
    }

    std::cout << "backends compiled: cpu=" << Screen::Backend::cpu
              << " gl=" << Screen::Backend::gl << std::endl;

#if !XII_BACKEND_GL
    if (want_gpu) {
        std::cerr << "OpenGL 后端未编译进此二进制，请用 -DXII_ENABLE_OPENGL=ON 重新配置" << std::endl;
        return 1;
    }
#endif

    //启动帧监控线程
    std::thread fps_pt(fps_ptf);

    auto root_space_point = std::make_shared<Space::Point>();
    auto root_time_point = std::make_shared<Time::Point>();

    auto camera_space_point = std::make_shared<Space::OffsetPoint>(root_space_point, Space::Offset(0, 0));

    auto camera = Camera::Standard(
        Item::Shape::Rectangle(camera_space_point, 1920, 1080),
        {SCREEN_WIDTH, SCREEN_HEIGHT},
        std::make_shared<Item::Base>(),
        std::make_shared<Time::OffsetPoint>(root_time_point, Time::Offset(0))
    );

    auto red_block_point = std::make_shared<Space::OffsetPoint>(root_space_point, Space::Offset(-200, -200));
    auto green_block_point = std::make_shared<Space::OffsetPoint>(root_space_point, Space::Offset(0, 0));
    auto blue_block_point = std::make_shared<Space::OffsetPoint>(root_space_point, Space::Offset(200, 200));

    auto red_block = std::make_shared<Item::General>(
        std::make_shared<Item::Shape::Rectangle>(red_block_point, 500, 500),
        std::vector<std::shared_ptr<Item::Renderer::Base>>{
            std::make_shared<Item::Renderer::SetColor>(Screen::Screen::Pixel{255, 255, 63, 63}),
            std::make_shared<Item::Renderer::Fill>()
        }
    );

    auto green_block = std::make_shared<Item::General>(
        std::make_shared<Item::Shape::Rectangle>(green_block_point, 500, 500),
        std::vector<std::shared_ptr<Item::Renderer::Base>>{
            std::make_shared<Item::Renderer::SetColor>(Screen::Screen::Pixel{255, 63, 255, 63}),
            std::make_shared<Item::Renderer::Fill>()
        }
    );

    auto blue_block = std::make_shared<Item::General>(
        std::make_shared<Item::Shape::Rectangle>(blue_block_point, 500, 500),
        std::vector<std::shared_ptr<Item::Renderer::Base>>{
            std::make_shared<Item::Renderer::SetColor>(Screen::Screen::Pixel{255, 63, 63, 255}),
            std::make_shared<Item::Renderer::Fill>()
        }
    );

    camera.getObject()->addItem(red_block);
    camera.getObject()->addItem(green_block);
    camera.getObject()->addItem(blue_block);


    //初始化SDL子系统
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        // 加上 SDL_GetError() 获取详细错误
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return -1;
    }

    if (want_gpu) {
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    }

    //创建窗口
    SDL_Window* gWindow = SDL_CreateWindow("Test",
                                           SCREEN_WIDTH, SCREEN_HEIGHT,
                                           want_gpu ? SDL_WINDOW_OPENGL : 0);

    //判断是否创建成功
    if (!gWindow) {
        SDL_Log("can not  create window:%s", SDL_GetError());
        return -1;
    }

    // ---- GPU 后端初始玌 ----
#if XII_BACKEND_GL
    std::unique_ptr<Screen::GLCanvas> gl_canvas;
#endif
    SDL_GLContext gl_context = nullptr;

#if XII_BACKEND_GL
    if (want_gpu) {
        gl_context = SDL_GL_CreateContext(gWindow);
        if (!gl_context) {
            std::cerr << "SDL_GL_CreateContext 失败: " << SDL_GetError() << std::endl;
            return -1;
        }
        SDL_GL_SetSwapInterval(0); // 关掉垂直同步，方便看到真实帧率

        gl_canvas = Screen::GLCanvas::create(gWindow, {SCREEN_WIDTH, SCREEN_HEIGHT});
        if (!gl_canvas) {
            std::cerr << "GLCanvas::create 失败: " << Screen::GLCanvas::getLastError() << std::endl;
            return -1;
        }

        std::cout << "GPU 后端已启用: " << gl_canvas->name() << std::endl;
        camera.setCanvas(gl_canvas.get());
    }
#endif

    // ---- CPU 后端初始化 ----
    SDL_Surface* surface = nullptr;
    SDL_Surface* gWinSfc = nullptr;

    if (!want_gpu) {
        gWinSfc = SDL_GetWindowSurface(gWindow);
        if (!gWinSfc) {
            return -1;
        }
        // ----- SDL3 创建 Surface -----
        surface = SDL_CreateSurfaceFrom(
            camera.getScreenR().getSize().x, // 宽度
            camera.getScreenR().getSize().y, // 高度
            SDL_PIXELFORMAT_BGRA8888,
            camera.getScreenR().getData(),
            camera.getScreenR().getSize().x * 4 // pitch（每行字节数）
        );

        if (!surface) {
            std::cerr << "创建 Surface 失败: " << SDL_GetError() << std::endl;
            return -1;
        }

        // 屏幕缓冲里已经是“合成完成的最终结果”，而且始终不透明
        // （clear() 写 A=255；alpha 混合后 A = sa + da*(1-sa) 仍为 255），
        // 所以呈现时不需要 SDL 再做一次逐像素 alpha 合成。
        if (!SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_NONE)) {
            std::cerr << "SetSurfaceBlendMode 失败: " << SDL_GetError() << std::endl;
            return -1;
        }
    }

    // ---- 首帧：GPU 与 CPU 结果逐像素对比 ----
    int parity_failures = 0;
#if XII_BACKEND_GL
    if (want_gpu) {
        // 用同一场景分别走 GPU / CPU，逐像素对比，返回是否在容差内
        auto verify = [&](const char* label) -> bool {
        camera.getTimePointR()->add();
        camera.Shoot();                       // GPU 渲染到默认帧缓冲

        Screen::Screen gpu_pixels({SCREEN_WIDTH, SCREEN_HEIGHT});
        gl_canvas->read_pixels(gpu_pixels);

        camera.setCanvas(nullptr);
        camera.getScreenR().clear();
        camera.Shoot();                       // CPU 渲染到 _screen
        camera.setCanvas(gl_canvas.get());

        const Screen::Screen& cpu_pixels = camera.getScreenR();
        const auto size = cpu_pixels.getSize();

        uint64_t compared = 0, diff_pixels = 0;
        int max_diff = 0;
        double sum_abs = 0.0;

        for (int64_t y = 0; y < size.y; ++y) {
            for (int64_t x = 0; x < size.x; ++x) {
                const auto& a = cpu_pixels.at({x, y});
                const auto& b = gpu_pixels.at({x, y});

                const int dr = std::abs(int(a.R) - int(b.R));
                const int dg = std::abs(int(a.G) - int(b.G));
                const int db = std::abs(int(a.B) - int(b.B));
                const int d = std::max({dr, dg, db});

                ++compared;
                if (d != 0) {
                    ++diff_pixels;
                    if (diff_pixels <= 12 && std::getenv("XII_DIFF_VERBOSE") != nullptr) {
                        std::cout << "  diff (" << x << "," << y << ") cpu=("
                                  << int(a.R) << "," << int(a.G) << "," << int(a.B)
                                  << ") gpu=(" << int(b.R) << "," << int(b.G) << ","
                                  << int(b.B) << ")\n";
                    }
                }
                max_diff = std::max(max_diff, d);
                sum_abs += dr + dg + db;
            }
        }

        const double pct = 100.0 * double(diff_pixels) / double(compared);
        const double mean = sum_abs / (double(compared) * 3.0);

        std::cout << "\n--- GPU vs CPU: " << label << " ---\n";
        std::cout << "  compared      : " << compared << "\n";
        std::cout << "  differing     : " << diff_pixels
                  << " (" << std::fixed << std::setprecision(4) << pct << "%)\n";
        std::cout << "  max channel d : " << max_diff << "\n";
        std::cout << "  mean abs d    : " << std::setprecision(3) << mean << "\n";
        std::cout << "  draw calls    : " << gl_canvas->getDrawCalls()
                  << ", vertices: " << gl_canvas->getVertexCount() << "\n";

        const bool ok = (pct <= 2.0);
        std::cout << (ok ? "[PASS] " : "[FAIL] ") << label << " parity\n";
        return ok;
        }; // verify

        if (!verify("fills only")) ++parity_failures;

        // 追加描边：不透明 + 半透明，多种线宽，用来覆盖 GPU 的 stroke 路径
        auto add_outline = [&](float cx, float cy, float w, float h,
                               uint8_t alpha, Screen::Screen::Pixel color, uint32_t width) {
            auto item = std::make_shared<Item::General>(
                std::make_shared<Item::Shape::Rectangle>(
                    std::make_shared<Space::OffsetPoint>(root_space_point,
                                                         Space::Offset(cx, cy)),
                    w, h));
            item->getRenderersR().push_back(std::make_shared<Item::Renderer::SetAlpha>(alpha));
            item->getRenderersR().push_back(std::make_shared<Item::Renderer::SetColor>(color));
            item->getRenderersR().push_back(std::make_shared<Item::Renderer::SetWidth>(width));
            item->getRenderersR().push_back(std::make_shared<Item::Renderer::Line>());
            camera.getObject()->addItem(item);
        };

        add_outline(0, 0, 300, 300, 255, {255, 255, 255, 0}, 2);      // 不透明黄
        add_outline(-400, 200, 400, 300, 96, {255, 0, 255, 255}, 3);  // 半透明青
        add_outline(400, -300, 500, 200, 64, {255, 255, 0, 255}, 4);  // 更淡的品红

        if (!verify("fills + strokes")) ++parity_failures;
    }
#endif

    // CPU 首次呈现
    if (!want_gpu) {
        SDL_BlitSurface(surface, NULL, gWinSfc, NULL);
        SDL_UpdateWindowSurface(gWindow);
    }

    //主循环标志
    while (!isDone) {
        //事件处理
        static SDL_Event ev = {};

        //处理队列上的事件
        while (SDL_PollEvent(&ev) != 0) {
            //用户请求退出
            if (ev.type == SDL_EVENT_QUIT) {
                isDone = true;
            }
        }

        camera.getTimePointR()->add();

        if (want_gpu) {
            camera.Shoot();                  // GPU：内部完成清屏 + 提交
            SDL_GL_SwapWindow(gWindow);
        } else {
            camera.getScreenR().clear();
            camera.Shoot();
            SDL_BlitSurface(surface, NULL, gWinSfc, NULL);
            SDL_UpdateWindowSurface(gWindow);
        }

        frame_count.fetch_add(1, std::memory_order_relaxed);
    }

    //停止帧监控
    isDone.store(true, std::memory_order_relaxed);
    fps_pt.join();

    //销毁窗口
#if XII_BACKEND_GL
    gl_canvas.reset();
#endif
    if (gl_context != nullptr)
        SDL_GL_DestroyContext(gl_context);
    if (surface != nullptr)
        SDL_DestroySurface(surface);
    SDL_DestroyWindow(gWindow);
    //清理所有初始化的子系统
    SDL_Quit();

    return parity_failures;
}
