//
// Created by qpwser from XCXK on 2026-10-18
//

#include <iostream>

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

int main(int argc, char** argv) {
    cout << "Outline Camera Example" << endl;

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
    SDL_Window* gWindow = SDL_CreateWindow("Outline Camera Example",
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
    bool isDone = false;
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
    }

    // Clean up and exit
    SDL_DestroyWindow(gWindow);
    SDL_Quit();

    return 0;
}