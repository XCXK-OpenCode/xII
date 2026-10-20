// 
// Created by qpwser from XCXK on 2026-10-20
// Powered by DeepSeek V4.1 Flash
//

#include <iostream>
#include <memory>
#include <atomic>
#include <thread>
#include <chrono>
#include <iomanip>
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <string>

#include "Camera/Standard.h"
#include "Camera/Outline.h"
#include "Item/Base.h"
#include "Item/General.h"
#include "Item/Renderer/Base.h"
#include "Item/Shape/Rectangle.h"
#include "SDL3/SDL.h"
#include "SDL3/SDL_video.h"
#include "Screen/Screen.h"
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
#include "Item/Text.h"

//屏幕尺寸常量
const int SCREEN_WIDTH = 1280;
const int SCREEN_HEIGHT = 720;

//摄像机视口：宽度用场景单位表示，越小画面放得越大。
//视口固定为 16:9（与窗口一致），缩放的只是倍率，不改变宽高比。
constexpr double CAMERA_BASE_WIDTH = 3840.0; // 初始视口宽度（场景单位）
constexpr double CAMERA_ASPECT     = 16.0 / 9.0;
constexpr double CAMERA_MIN_WIDTH  = 40.0;    // 最大放大倍率 3840/40 = 96x
constexpr double CAMERA_MAX_WIDTH  = 38400.0; // 最大缩小倍率 0.1x
constexpr double CAMERA_ZOOM_STEP  = 1.25;    // 每次按键的倍率变化
constexpr double CAMERA_PAN_LIMIT  = 100000.0; // 平移上限（场景单位），防止拖到无限远

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

int main() {
    std::cout << "Test 20261018-173742" << std::endl;

    //启动帧监控线程
    std::thread fps_pt(fps_ptf);

    auto root_space_point = std::make_shared<Space::Point>();
    auto root_time_point = std::make_shared<Time::Point>();

    // 视口中心（场景坐标）：缩放围绕它，拖动平移时修改它的 offset。
    // Point 是“活的”：Camera 每帧重新读 getX()/getY()，所以平移只需 setOffset，
    // 不用重建对象，也不用再 setShape（只有缩放改宽高时才需要）。
    double pan_x = 0.0;
    double pan_y = 0.0;
    double camera_width = CAMERA_BASE_WIDTH;

    auto camera_center = std::make_shared<Space::OffsetPoint>(
        root_space_point,
        Space::Offset(static_cast<xII::Base::Quantity::Base_Value_Type>(pan_x),
                      static_cast<xII::Base::Quantity::Base_Value_Type>(pan_y)));

    Camera::Standard camera(
        Item::Shape::Rectangle(
            camera_center,
            static_cast<xII::Base::Quantity::Base_Value_Type>(camera_width),
            static_cast<xII::Base::Quantity::Base_Value_Type>(camera_width / CAMERA_ASPECT)),
        {SCREEN_WIDTH, SCREEN_HEIGHT},
        std::make_shared<Item::Base>(),
        std::make_shared<Time::OffsetPoint>(root_time_point, Time::Offset(0))
    );

    // 场景原点与四角各放一个标记，方便看出来平移/缩放的方向。
    auto add_label = [&](const double x, const double y, const std::string& s, const double h,
                         const Screen::Screen::Pixel& color) {
        camera.getObject()->addItem(std::make_shared<Item::Text>(
            std::make_shared<Space::OffsetPoint>(
                root_space_point,
                Space::Offset(static_cast<xII::Base::Quantity::Base_Value_Type>(x),
                              static_cast<xII::Base::Quantity::Base_Value_Type>(y))),
            s, std::string(""), h, color));
    };

    add_label(100, 100, "Hello World", 32, Screen::Screen::Pixel{255, 255, 255, 255});
    add_label(0, 0, "origin", 96, Screen::Screen::Pixel{255, 255, 200, 60});
    add_label(-1600, 900, "top-left", 72, Screen::Screen::Pixel{255, 120, 220, 120});
    add_label(1600, 900, "top-right", 72, Screen::Screen::Pixel{255, 120, 160, 220});
    add_label(-1600, -900, "bottom-left", 72, Screen::Screen::Pixel{255, 220, 160, 120});
    add_label(1600, -900, "bottom-right", 72, Screen::Screen::Pixel{255, 220, 120, 220});

    //初始化SDL子系统
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        // 加上 SDL_GetError() 获取详细错误
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return -1;
    }

    //创建窗口
    SDL_Window* gWindow = SDL_CreateWindow("Test",
                                           SCREEN_WIDTH, SCREEN_HEIGHT,
                                           0);

    //判断是否创建成功
    if (!gWindow) {
        SDL_Log("can not  create window:%s", SDL_GetError());
        return -1;
    }

    // ---- 摄像机视口 ----
    // 视口 = 场景中的一个 16:9 矩形：
    //   center = camera_center（活的点），width = camera_width，height = width / (16/9)
    // 缩放要改 width/height（Rectangle 里是值拷贝）→ setShape；
    // 平移只改 camera_center 的 offset，Camera 每帧自己读，不需要 setShape。
    auto update_status = [&](const bool log) {
        const double zoom = CAMERA_BASE_WIDTH / camera_width;

        std::ostringstream info;
        info << "zoom x" << std::fixed << std::setprecision(2) << zoom
             << "  viewport " << std::setprecision(0) << camera_width << "x"
             << camera_width / CAMERA_ASPECT
             << "  center(" << std::setprecision(0) << pan_x << ", " << pan_y << ")";

        SDL_SetWindowTitle(gWindow, ("Test  " + info.str()).c_str());

        if (log)
            std::cout << info.str() << std::endl;
    };

    // 把当前 camera_width 写回相机的 Rectangle（center 不用重建）。
    auto apply_zoom_shape = [&]() {
        camera.setShape(Item::Shape::Rectangle(
            camera_center,
            static_cast<xII::Base::Quantity::Base_Value_Type>(camera_width),
            static_cast<xII::Base::Quantity::Base_Value_Type>(camera_width / CAMERA_ASPECT)));
    };

    // factor > 1 放大（视口变窄），factor < 1 缩小。
    auto apply_zoom = [&](const double factor) {
        const double next = std::clamp(camera_width / factor, CAMERA_MIN_WIDTH, CAMERA_MAX_WIDTH);
        if (next == camera_width)
            return; // 已到上下限

        camera_width = next;
        apply_zoom_shape();
        update_status(true);
    };

    // 屏幕像素 -> 场景单位的缩放（与相机内部一致：screen_h / range_h）。
    auto view_scale = [&]() {
        return static_cast<double>(SCREEN_HEIGHT) / (camera_width / CAMERA_ASPECT);
    };

    // 拖动平移：画面跟着鼠标走，所以相机中心朝鼠标移动的反方向移动。
    // 只改 offset，下一帧 Camera 自己会读到新坐标。
    auto apply_pan_pixels = [&](const double dx_px, const double dy_px) {
        const double scale = view_scale();
        if (scale <= 0.0)
            return;

        pan_x = std::clamp(pan_x - dx_px / scale, -CAMERA_PAN_LIMIT, CAMERA_PAN_LIMIT);
        pan_y = std::clamp(pan_y - dy_px / scale, -CAMERA_PAN_LIMIT, CAMERA_PAN_LIMIT);
        camera_center->setOffset(
            static_cast<xII::Base::Quantity::Base_Value_Type>(pan_x),
            static_cast<xII::Base::Quantity::Base_Value_Type>(pan_y));

        // 拖动时每帧都改窗口标题太浪费，这里只更新标题不打印。
        update_status(false);
    };

    // 复位：倍率与中心都回到初始值。
    auto reset_view = [&]() {
        camera_width = CAMERA_BASE_WIDTH;
        pan_x = 0.0;
        pan_y = 0.0;
        camera_center->setOffset(static_cast<xII::Base::Quantity::Base_Value_Type>(0.0),
                                 static_cast<xII::Base::Quantity::Base_Value_Type>(0.0));
        apply_zoom_shape();
        update_status(true);
    };

    std::cout << "Ctrl + / - : zoom camera (base 16:9, viewport width "
              << CAMERA_BASE_WIDTH << ")\n"
              << "Ctrl + 0   : reset zoom + pan\n"
              << "Ctrl + wheel: zoom camera\n"
              << "drag (LMB/RMB): pan camera" << std::endl;

    apply_zoom_shape();
    update_status(true);

    // 获取表面
    auto gWinSfc = SDL_GetWindowSurface(gWindow);
    if (!gWinSfc) {
        return -1;
    }
    // ----- SDL3 创建 Surface -----
    SDL_Surface* surface = SDL_CreateSurfaceFrom(
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
    // 默认 BLENDMODE_BLEND 会让 SDL 每帧对 1280x720 的每个像素做混合，
    // 实测 13.7 ms/帧；改为 NONE 后降到 2.8 ms/帧（约 5 倍）。
    if (!SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_NONE)) {
        std::cerr << "SetSurfaceBlendMode 失败: " << SDL_GetError() << std::endl;
        return -1;
    }

    SDL_BlitSurface(surface, NULL, gWinSfc, NULL);
    SDL_UpdateWindowSurface(gWindow);

    // 拖动状态：左/右键按住时跟随鼠标位移平移相机。
    bool dragging = false;
    SDL_Cursor* move_cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_MOVE);

    //主循环标志
    while (!isDone) {
        //事件处理
        static SDL_Event ev = {};

        //处理队列上的事件
        while (SDL_PollEvent(&ev) != 0) {
            //用户请求退出
            if (ev.type == SDL_EVENT_QUIT) {
                isDone = true;
            } else if (ev.type == SDL_EVENT_KEY_DOWN) {
                // 只在按住 Ctrl 时响应缩放，避免和普通按键冲突。
                const bool ctrl = (ev.key.mod & SDL_KMOD_CTRL) != 0;
                if (!ctrl)
                    continue;

                switch (ev.key.key) {
                    // Ctrl + '+'：主键盘（= 键，含 Shift）与小键盘
                    case SDLK_EQUALS:
                    case SDLK_PLUS:
                    case SDLK_KP_PLUS:
                        apply_zoom(CAMERA_ZOOM_STEP);
                        break;
                    // Ctrl + '-'：主键盘与小键盘
                    case SDLK_MINUS:
                    case SDLK_KP_MINUS:
                        apply_zoom(1.0 / CAMERA_ZOOM_STEP);
                        break;
                    // Ctrl + '0'：回到初始倍率与位置
                    case SDLK_0:
                    case SDLK_KP_0:
                        reset_view();
                        break;
                    default:
                        break;
                }
            } else if (ev.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                // 左键按下开始拖动；右键也可以，方便鼠标垫。
                if (ev.button.button == SDL_BUTTON_LEFT || ev.button.button == SDL_BUTTON_RIGHT) {
                    dragging = true;
                    SDL_SetCursor(move_cursor);
                }
            } else if (ev.type == SDL_EVENT_MOUSE_BUTTON_UP) {
                if (ev.button.button == SDL_BUTTON_LEFT || ev.button.button == SDL_BUTTON_RIGHT) {
                    dragging = false;
                    SDL_SetCursor(SDL_GetDefaultCursor());
                }
            } else if (ev.type == SDL_EVENT_MOUSE_MOTION) {
                // 用相对位移拖动，鼠标碰到窗口边缘也不会卡住。
                if (dragging)
                    apply_pan_pixels(static_cast<double>(ev.motion.xrel),
                                     static_cast<double>(ev.motion.yrel));
            } else if (ev.type == SDL_EVENT_MOUSE_WHEEL) {
                // 按住 Ctrl 时滚轮也可以缩放；向上滚放大。
                // SDL3 的 wheel 事件没有 mod 字段，直接查询当前修饰键状态。
                if ((SDL_GetModState() & SDL_KMOD_CTRL) != 0 && ev.wheel.y != 0.0f)
                    apply_zoom(ev.wheel.y > 0.0f ? CAMERA_ZOOM_STEP : 1.0 / CAMERA_ZOOM_STEP);
            }
        }

        camera.getTimePointR()->add();
        camera.getScreenR().clear();
        camera.Shoot();

        SDL_BlitSurface(surface, NULL, gWinSfc, NULL);
        SDL_UpdateWindowSurface(gWindow);
        frame_count.fetch_add(1, std::memory_order_relaxed);
    }

    //停止帧监控
    isDone.store(true, std::memory_order_relaxed);
    fps_pt.join();

    SDL_DestroyCursor(move_cursor);

    //销毁窗口
    SDL_DestroyWindow(gWindow);
    //清理所有初始化的子系统
    SDL_Quit();

    return 0;
}
