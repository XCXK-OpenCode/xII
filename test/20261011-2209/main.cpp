/*****************************************************************************
 * @file        main.cpp
 * @brief       ${BRIEF_DESCRIPTION}
 *
 * @project     xII (XCXK (OC) II)
 * @organization XCXK-OC (XCXK OpenCode)
 *              (A subsidiary organization under XCXK)
 *
 * @copyright   Copyright (C) XCXK-OC. All Rights Reserved.
 *
 * @license     Use and distribution of this source code, binary forms, or any
 *              derivative works are permitted ONLY under the terms and
 *              conditions explicitly stipulated by XCXK-OC.
 *
 * @legal       DISCLAIMER & INTERPRETATION RIGHTS:
 *              XCXK-OC reserves the right of final interpretation for this
 *              software, its source code, documentation, and related materials.
 *              As the parent organization, XCXK retains the overriding right
 *              of final interpretation over all projects under XCXK-OC,
 *              including this one (xII / XCXK (OC) II).
 *
 *              Unauthorized distribution, modification, or use that violates
 *              the stipulated regulations is strictly prohibited.
 *****************************************************************************/
#include <iostream>
#include <memory>

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
#include "Time/Point.h"
#include "Time/OffsetPoint.h"

//屏幕尺寸常量
const int SCREEN_WIDTH = 1280;
const int SCREEN_HEIGHT = 720;

int main() {
    std::cout << "Hello world!" << std::endl;

    auto root_object = std::make_shared<xII::Item::Base>();
    auto root_space_point = std::make_shared<xII::Space::Point>();
    auto camera_center = std::make_shared<xII::Space::OffsetPoint>(root_space_point, xII::Space::Offset(0, 0));
    auto root_time_point = std::make_shared<xII::Time::Point>();
    auto time_point = std::make_shared<xII::Time::OffsetPoint>(root_time_point, xII::Time::Offset(0));
    auto camera = xII::Camera::Standard(
        xII::Item::Shape::Rectangle(camera_center, 1920, 1080),
        {SCREEN_WIDTH, SCREEN_HEIGHT},
        root_object,
        time_point);

    auto st_item_color = std::make_shared<xII::Item::Renderer::SetColor>(xII::Screen::Screen::RGBA8888::Pixel{.A = 255, .R = 255, .G = 0, .B = 0});
    auto st_item_width = std::make_shared<xII::Item::Renderer::SetWidth>(5);
    auto st_item_line = std::make_shared<xII::Item::Renderer::Line>();

    auto st_item1 = std::make_shared<xII::Item::General>(
        std::make_shared<xII::Item::Shape::Rectangle>(std::make_shared<xII::Space::OffsetPoint>(root_space_point, xII::Space::Offset(400, 200)), 600, 600),
        std::vector<std::shared_ptr<xII::Item::Renderer::Base>>());
    auto st_item2 = std::make_shared<xII::Item::General>(
        std::make_shared<xII::Item::Shape::Rectangle>(std::make_shared<xII::Space::OffsetPoint>(root_space_point, xII::Space::Offset(-100, 100)), 800, 500),
        std::vector<std::shared_ptr<xII::Item::Renderer::Base>>());
    st_item1->getRenderersR().push_back(st_item_color);
    st_item1->getRenderersR().push_back(st_item_width);
    st_item1->getRenderersR().push_back(st_item_line);

    st_item2->getRenderersR().push_back(st_item_color);
    st_item2->getRenderersR().push_back(st_item_line);
    st_item2->getRenderersR().push_back(st_item_width);
    root_object->addItem(st_item1);
    root_object->addItem(st_item2);

    camera.Shoot();

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
    bool isDone = false;
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
    }

    //销毁窗口
    SDL_DestroyWindow(gWindow);
    //清理所有初始化的子系统
    SDL_Quit();

    return 0;
}
