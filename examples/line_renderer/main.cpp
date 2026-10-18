//
// Created by qpwser from XCXK on 2026-10-18
//

#include <atomic>
#include <iostream>
#include <thread>
#include <chrono>
#include <iomanip>

#include <Camera/Standard.h>
#include <Item/General.h>

#include <Item/Renderer/SetWidth.h>
#include <Item/Renderer/SetColor.h>
#include <Item/Renderer/Line.h>

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

    auto   last_time  = steady_clock::now();  // 上一次刷新的时刻
    size_t last_count = 0;                    // 上一次刷新时的帧数

    while(!isDone.load(std::memory_order_relaxed)) {
        this_thread::sleep_for(milliseconds(100));

        auto now     = steady_clock::now();
        auto elapsed = duration_cast<milliseconds>(now - last_time).count();

        if (elapsed >= 1000) {                        // 满 1 秒才刷新
            size_t cur = frame_count.load(std::memory_order_relaxed);
            double fps = (cur - last_count) * 1000.0 / elapsed;

            // \r 回到行首，末尾补空格清掉上一次的残留字符
            cout << "\rFPS: " << fixed << setprecision(1)
                      << fps
                      << "   frames: " << cur
                      << "        " << flush;   // flush 才能真正显示

            last_count = cur;
            last_time  = now;
        }
    }
    cout << endl;
}

int main(int argc, char** argv) {
    cout << "Line Renderer Example" << endl;

    thread fps_pt(fps_ptf);

    auto root_space_point = std::make_shared<Space::Point>();
    auto root_time_point = std::make_shared<Time::Point>();

    auto camera_time_point = std::make_shared<Time::OffsetPoint>(root_time_point, Time::Offset(0));
    auto camera_center = std::make_shared<Space::OffsetPoint>(root_space_point, Space::Offset(0, 0));

    auto camera = Camera::Standard(
        Item::Shape::Rectangle(camera_center, 1920, 1080),  // Camera shape
        {SCREEN_WIDTH, SCREEN_HEIGHT},      // Screen size
        std::make_shared<Item::Base>(),     // Root object
        camera_time_point                   // Time point
    );

    camera.getObject()->addItem(std::make_shared<Item::General>(
        std::make_shared<Item::Shape::Rectangle>( // Shape
            std::make_shared<Space::OffsetPoint>(root_space_point, Space::Offset(-100, -100)), // Center
            640, 480 // Width and Height
        ),
        std::vector<std::shared_ptr<Item::Renderer::Base>>{ // Renderers
            std::make_shared<Item::Renderer::SetColor>(Screen::Screen::Pixel{255, 255, 255, 255}),  // Color
            std::make_shared<Item::Renderer::SetWidth>(2),  // Width
            std::make_shared<Item::Renderer::Line>()    // Line 
        }
    ));

    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        // Add SDL_GetError() for detailed error
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return -1;
    }

    // Create window
    SDL_Window* gWindow = SDL_CreateWindow("Line Renderer Example",
                                           SCREEN_WIDTH, SCREEN_HEIGHT,
                                           0);

    // Check if window creation was successful
    if (!gWindow) {
        SDL_Log("can not  create window:%s", SDL_GetError());
        return -1;
    }

    // Get window surface
    auto gWinSfc = SDL_GetWindowSurface(gWindow);
    if (!gWinSfc) {
        return -1;
    }

    // Create SDL surface from camera's screen data
    SDL_Surface* surface = SDL_CreateSurfaceFrom(
        camera.getScreenR().getSize().x, // Width
        camera.getScreenR().getSize().y, // Height
        SDL_PIXELFORMAT_BGRA8888,
        camera.getScreenR().getData(),
        camera.getScreenR().getSize().x * 4 // Pitch (bytes per row)
    );

    if (!surface) {
        std::cerr << "Failed to create surface: " << SDL_GetError() << std::endl;
        return -1;
    }

    // Main loop
    while (!isDone) {
        // Handle events
        static SDL_Event ev = {};

        while (SDL_PollEvent(&ev) != 0)
            if (ev.type == SDL_EVENT_QUIT)
                isDone = true;

        // Update camera and render to window
        camera.Shoot();
        SDL_BlitSurface(surface, NULL, gWinSfc, NULL);
        SDL_UpdateWindowSurface(gWindow);
        frame_count.fetch_add(1, std::memory_order_relaxed);
    }

    // Clean up and exit
    fps_pt.join();
    SDL_DestroyWindow(gWindow);
    SDL_Quit();

    return 0;
}