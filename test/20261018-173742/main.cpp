#include <iostream>
#include <memory>
#include <atomic>
#include <thread>
#include <chrono>
#include <iomanip>

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

int main() {
    std::cout << "Test 20261018-173742" << std::endl;

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

    //创建窗口
    SDL_Window* gWindow = SDL_CreateWindow("Test",
                                           SCREEN_WIDTH, SCREEN_HEIGHT,
                                           0);

    //判断是否创建成功
    if (!gWindow) {
        SDL_Log("can not  create window:%s", SDL_GetError());
        return -1;
    }

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

    SDL_BlitSurface(surface, NULL, gWinSfc, NULL);
    SDL_UpdateWindowSurface(gWindow);

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
        camera.getScreenR().clear();
        camera.Shoot();

        SDL_BlitSurface(surface, NULL, gWinSfc, NULL);
        SDL_UpdateWindowSurface(gWindow);
        frame_count.fetch_add(1, std::memory_order_relaxed);
    }

    //停止帧监控
    isDone.store(true, std::memory_order_relaxed);
    fps_pt.join();

    //销毁窗口
    SDL_DestroyWindow(gWindow);
    //清理所有初始化的子系统
    SDL_Quit();

    return 0;
}
