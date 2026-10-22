// 
// Created by qpwser from XCXK on 2026-10-21
// Powered by DeepSeek V4.1 Flash
//

// `image.getScreen() << camera.getScreen()`（屏幕拷贝 / 引用）测试。
//
// 1) 无窗口逐像素校验 operator<<：
//      同尺寸深拷贝、尺寸不同自动 resize、空句柄安全、自拷贝、链式 a << b << c
// 2) 端到端：mini 相机渲染 -> 拷进 Image 的屏幕 -> 主相机把它当贴图画出来
// 3) 零拷贝（Ref）对照：camera.setScreen(image.getScreen()) 共享同一块缓冲
// 4) 开窗演示：画中画（PiP），Space 在 copy / ref 两种方式之间切换；
//    R/G/B 三块与画中画 Image 都在动
// 5) 可选：XII_PIP_DUMP=/path/frame.bmp 导出画面（配合 XII_PIP_DUMP_ONLY=1）
//          XII_PIP_HEADLESS=1 只跑校验不开窗

#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "Camera/Standard.h"
#include "Item/Base.h"
#include "Item/General.h"
#include "Item/Image.h"
#include "Item/Renderer/Fill.h"
#include "Item/Renderer/SetColor.h"
#include "Item/Shape/Rectangle.h"
#include "SDL3/SDL.h"
#include "Screen/Screen.h"
#include "Space/Offset.h"
#include "Space/OffsetPoint.h"
#include "Space/Point.h"
#include "Time/OffsetPoint.h"
#include "Time/Point.h"

//屏幕尺寸常量
constexpr int SCREEN_WIDTH = 1280;
constexpr int SCREEN_HEIGHT = 720;

//画中画源屏幕尺寸
constexpr int PIP_WIDTH = 480;
constexpr int PIP_HEIGHT = 270;

//主相机视口（场景单位）
constexpr double RANGE_W = 1920.0;
constexpr double RANGE_H = 1080.0;

using namespace xII;
using Val = xII::Base::Quantity::Base_Value_Type;
using ScreenRef = Screen::ScreenRef;

namespace {
    //帧监控
    std::atomic<size_t> frame_count{0};
    std::atomic<bool> isDone{false};

    int check_failed = 0;

    void fps_ptf() {
        using namespace std::chrono;

        auto last_time = steady_clock::now(); // Last time
        size_t last_count = 0;                // Last frame count

        while (!isDone.load(std::memory_order_relaxed)) {
            // check every 100ms
            std::this_thread::sleep_for(milliseconds(100));

            auto now = steady_clock::now();
            auto elapsed = duration_cast<milliseconds>(now - last_time).count();

            if (elapsed >= 1000) {
                size_t cur = frame_count.load(std::memory_order_relaxed);
                double fps = static_cast<double>(cur - last_count) * 1000.0 / static_cast<double>(elapsed);

                std::cout << "\rFPS: " << std::fixed << std::setprecision(1)
                          << fps
                          << "   frames: " << cur
                          << "        " << std::flush;

                last_count = cur;
                last_time = now;
            }
        }
        std::cout << std::endl;
    }

    // ------------------------------------------------------------ 小工具
    bool samePixel(const Screen::Screen::Pixel& a, const Screen::Screen::Pixel& b) {
        return a.A == b.A && a.R == b.R && a.G == b.G && a.B == b.B;
    }

    bool sameScreen(const Screen::Screen& a, const Screen::Screen& b) {
        if (a.getSize().x != b.getSize().x || a.getSize().y != b.getSize().y)
            return false;
        for (int64_t y = 0; y < a.getSize().y; ++y)
            for (int64_t x = 0; x < a.getSize().x; ++x)
                if (!samePixel(a.at({x, y}), b.at({x, y})))
                    return false;
        return true;
    }

    int64_t diffCount(const Screen::Screen& a, const Screen::Screen& b) {
        if (a.getSize().x != b.getSize().x || a.getSize().y != b.getSize().y)
            return -1;
        int64_t n = 0;
        for (int64_t y = 0; y < a.getSize().y; ++y)
            for (int64_t x = 0; x < a.getSize().x; ++x)
                if (!samePixel(a.at({x, y}), b.at({x, y})))
                    ++n;
        return n;
    }

    // 每个像素都不一样的确定性图，便于逐像素比对。
    Screen::Screen makePattern(const int w, const int h, const int seed) {
        Screen::Screen s({w, h});
        for (int y = 0; y < h; ++y)
            for (int x = 0; x < w; ++x)
                s.at({x, y}) = {
                    255,
                    static_cast<uint8_t>((x * 17 + seed * 31) & 0xFF),
                    static_cast<uint8_t>((y * 23 + seed * 13) & 0xFF),
                    static_cast<uint8_t>(((x + y) * 11 + seed * 7) & 0xFF)};
        return s;
    }

    void check(const char* name, const bool ok, const std::string& detail) {
        std::cout << "[check] " << std::left << std::setw(38) << name
                  << (ok ? "PASS" : "FAIL") << "   " << detail << std::endl;
        if (!ok)
            ++check_failed;
    }

    std::string sizeText(const Screen::Screen& s) {
        return std::to_string(s.getSize().x) + "x" + std::to_string(s.getSize().y);
    }

    std::string pxText(const Screen::Screen::Pixel& p) {
        return "(" + std::to_string(p.R) + "," + std::to_string(p.G) + "," + std::to_string(p.B) + ")";
    }

    // 建一台相机 + 它的场景根节点。
    struct MiniScene {
        std::shared_ptr<Space::Point> root = std::make_shared<Space::Point>();
        std::shared_ptr<Time::Point> time_root = std::make_shared<Time::Point>();
        Camera::Standard camera;

        MiniScene(const double range_w, const double range_h, const Screen::PoSize& screen)
            : camera(Item::Shape::Rectangle(
                         std::make_shared<Space::OffsetPoint>(root, Space::Offset(0, 0)),
                         static_cast<Val>(range_w), static_cast<Val>(range_h)),
                     screen,
                     std::make_shared<Item::Base>(),
                     std::make_shared<Time::OffsetPoint>(time_root, Time::Offset(0))) {}

        // 加一块纯色块，同时返回它的 Rectangle（General::getShape() 只给 Base*，
        // 动画里要改中心点，所以这里直接把 Rectangle 的句柄带出去）。
        struct SolidBlock {
            std::shared_ptr<Item::General> item;
            std::shared_ptr<Item::Shape::Rectangle> shape;
        };

        SolidBlock addSolid(const Screen::Screen::Pixel& color,
                            const double w, const double h,
                            const double x = 0.0, const double y = 0.0) {
            auto rect = std::make_shared<Item::Shape::Rectangle>(
                std::make_shared<Space::OffsetPoint>(root, Space::Offset(x, y)),
                static_cast<Val>(w), static_cast<Val>(h));
            auto item = std::make_shared<Item::General>(
                rect,
                std::vector<std::shared_ptr<Item::Renderer::Base>>{
                    std::make_shared<Item::Renderer::SetColor>(color),
                    std::make_shared<Item::Renderer::Fill>()});
            camera.getObject()->addItem(item);
            return {item, rect};
        }
    };
} // namespace

int main() {
    std::cout << "Test 20261021-210918: image.getScreen() << camera.getScreen()" << std::endl;

    // ============================================================ 无窗口校验
    // ---------------- 1) 同尺寸深拷贝 ----------------
    {
        auto src = makePattern(32, 32, 7);
        auto dst = makePattern(32, 32, 1);
        const auto dst_before = makePattern(32, 32, 1);

        dst << src;

        const bool ok = dst.getSize().x == 32 && dst.getSize().y == 32 &&
                        sameScreen(dst, src) &&
                        diffCount(dst, dst_before) > 0; // 真的被覆盖了
        check("same size deep copy", ok,
              "src=" + sizeText(src) + " dst=" + sizeText(dst) +
                  " diff(dst,src)=" + std::to_string(diffCount(dst, src)));
    }

    // ---------------- 2) 尺寸不同自动 resize ----------------
    {
        auto src = makePattern(40, 24, 3);
        auto dst = makePattern(8, 8, 0);
        const int64_t old_w = dst.getSize().x;

        dst << src;

        const bool ok = dst.getSize().x == 40 && dst.getSize().y == 24 && sameScreen(dst, src);
        check("size change resizes dest", ok,
              std::to_string(old_w) + "x8 <- " + sizeText(src) + " -> " + sizeText(dst) +
                  " diff=" + std::to_string(diffCount(dst, src)));
    }

    // ---------------- 3) 空句柄安全 ----------------
    {
        ScreenRef null_dest;
        auto src = std::make_shared<Screen::Screen>(makePattern(4, 4, 5));
        null_dest << src; // 不应该崩

        auto dst = std::make_shared<Screen::Screen>(makePattern(4, 4, 5));
        const auto dst_before = makePattern(4, 4, 5);
        ScreenRef null_src;
        dst << null_src; // 不应该崩，也不应该改动 dst

        const bool ok = null_dest == nullptr && sameScreen(*dst, dst_before);
        check("null handles are no-op", ok,
              "null dest stays null=1, dst unchanged=" +
                  std::to_string(sameScreen(*dst, dst_before) ? 1 : 0));
    }

    // ---------------- 4) 自拷贝 ----------------
    {
        auto self = makePattern(6, 6, 9);
        const auto before = self.at({0, 0});
        const int64_t before_size = self.getSize().x;

        self << self;

        const bool ok = self.getSize().x == before_size && self.getSize().y == 6 &&
                        samePixel(before, self.at({0, 0})) && diffCount(self, self) == 0;
        check("self copy is safe", ok,
              "size=" + sizeText(self) + " px(0,0)=" + pxText(self.at({0, 0})));
    }

    // ---------------- 5) 链式 a << b << c ----------------
    {
        auto a = makePattern(8, 8, 1);
        auto b = makePattern(8, 8, 2);
        auto c = makePattern(8, 8, 3);

        a << b << c; // (a << b) 返回 a 的句柄，再 << c

        const bool ok = sameScreen(a, c);
        check("chained a << b << c", ok,
              "a==c diff=" + std::to_string(diffCount(a, c)) +
                  " a==b diff=" + std::to_string(diffCount(a, b)));
    }

    // ---------------- 6) 端到端：image.getScreen() << camera2.getScreen() ----------------
    {
        const Screen::Screen::Pixel red{255, 220, 40, 40};

        MiniScene main_scene(RANGE_W, RANGE_H, {SCREEN_WIDTH, SCREEN_HEIGHT});
        MiniScene mini_scene(RANGE_W, RANGE_H, {PIP_WIDTH, PIP_HEIGHT});
        mini_scene.addSolid(red, RANGE_W * 2.0, RANGE_H * 2.0);

        // 铺满主视口的贴图，方便直接取中心像素
        auto pip = std::make_shared<Item::Image>(
            Screen::PoSize{PIP_WIDTH, PIP_HEIGHT},
            Item::Shape::Rectangle(
                std::make_shared<Space::OffsetPoint>(main_scene.root, Space::Offset(0, 0)),
                static_cast<Val>(RANGE_W), static_cast<Val>(RANGE_H)));
        main_scene.camera.getObject()->addItem(pip);

        // mini 先渲染到自己的屏幕
        mini_scene.camera.getScreen()->clear();
        mini_scene.camera.Shoot();
        const auto mini_px = mini_scene.camera.getScreen()->at({PIP_WIDTH / 2, PIP_HEIGHT / 2});

        // 关键一行：把 mini 的画面拷进贴图的屏幕
        pip->getScreen() << mini_scene.camera.getScreen();

        main_scene.camera.getScreen()->clear();
        main_scene.camera.Shoot();
        const auto main_px = main_scene.camera.getScreen()->at({SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2});

        const bool ok = samePixel(mini_px, red) && samePixel(main_px, red) &&
                        pip->getScreen()->getSize().x == PIP_WIDTH &&
                        pip->getScreen()->getSize().y == PIP_HEIGHT &&
                        pip->getScreen().get() != mini_scene.camera.getScreen().get();
        check("image.getScreen() << camera.getScreen()", ok,
              "mini=" + pxText(mini_px) + " pip=" + sizeText(*pip->getScreen()) +
                  " main=" + pxText(main_px) + " (separate buffers)");
    }

    // ---------------- 7) 零拷贝对照：setScreen 共享同一块缓冲 ----------------
    {
        const Screen::Screen::Pixel green{255, 40, 200, 60};

        MiniScene main_scene(RANGE_W, RANGE_H, {SCREEN_WIDTH, SCREEN_HEIGHT});
        MiniScene mini_scene(RANGE_W, RANGE_H, {PIP_WIDTH, PIP_HEIGHT});
        mini_scene.addSolid(green, RANGE_W * 2.0, RANGE_H * 2.0);

        auto pip = std::make_shared<Item::Image>(
            Screen::PoSize{PIP_WIDTH, PIP_HEIGHT},
            Item::Shape::Rectangle(
                std::make_shared<Space::OffsetPoint>(main_scene.root, Space::Offset(0, 0)),
                static_cast<Val>(RANGE_W), static_cast<Val>(RANGE_H)));
        main_scene.camera.getObject()->addItem(pip);

        // Ref 写法：相机直接把画面渲染进贴图的屏幕，完全不需要拷贝
        mini_scene.camera.setScreen(pip->getScreen());

        const bool shared = mini_scene.camera.getScreen().get() == pip->getScreen().get();
        const void* pip_data = static_cast<const void*>(pip->getScreen()->getData());

        mini_scene.camera.getScreen()->clear();
        mini_scene.camera.Shoot();

        const auto pip_px = pip->getScreen()->at({PIP_WIDTH / 2, PIP_HEIGHT / 2});
        const bool same_buffer = pip_data == static_cast<const void*>(pip->getScreen()->getData());

        main_scene.camera.getScreen()->clear();
        main_scene.camera.Shoot();
        const auto main_px = main_scene.camera.getScreen()->at({SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2});

        const bool ok = shared && same_buffer && samePixel(pip_px, green) && samePixel(main_px, green);
        check("ref (shared buffer, zero copy)", ok,
              "same ptr=" + std::to_string(shared ? 1 : 0) +
                  " no realloc=" + std::to_string(same_buffer ? 1 : 0) +
                  " pip=" + pxText(pip_px) + " main=" + pxText(main_px));
    }

    // ---------------- 8) 拷贝开销 ----------------
    {
        auto big_src = makePattern(SCREEN_WIDTH, SCREEN_HEIGHT, 1);
        auto big_dst = makePattern(SCREEN_WIDTH, SCREEN_HEIGHT, 2);
        auto small_src = makePattern(PIP_WIDTH, PIP_HEIGHT, 3);
        auto small_dst = makePattern(PIP_WIDTH, PIP_HEIGHT, 4);

        const auto timeCopy = [](ScreenRef dest, ScreenRef src, const int iterations) {
            using clock = std::chrono::steady_clock;
            const auto t0 = clock::now();
            for (int i = 0; i < iterations; ++i)
                dest << src;
            const auto t1 = clock::now();
            return std::chrono::duration<double, std::milli>(t1 - t0).count() / iterations;
        };

        // 预热
        timeCopy(std::make_shared<Screen::Screen>(big_dst), std::make_shared<Screen::Screen>(big_src), 3);

        const double big_ms = timeCopy(std::make_shared<Screen::Screen>(big_dst),
                                       std::make_shared<Screen::Screen>(big_src), 50);
        const double small_ms = timeCopy(std::make_shared<Screen::Screen>(small_dst),
                                         std::make_shared<Screen::Screen>(small_src), 200);

        const bool ok = big_ms < 8.0 && small_ms < 8.0;
        std::cout << "[bench] " << std::left << std::setw(34) << "operator<< per frame"
                  << (ok ? "PASS" : "FAIL") << "   "
                  << "1280x720 " << std::fixed << std::setprecision(3) << big_ms << " ms, "
                  << "480x270 " << std::fixed << std::setprecision(3) << small_ms << " ms" << std::endl;
        if (!ok)
            ++check_failed;
    }

    std::cout << (check_failed == 0 ? "\nALL PIP CHECKS PASS" : "\nSOME PIP CHECKS FAILED") << std::endl;

    if (const char* headless = std::getenv("XII_PIP_HEADLESS"); headless != nullptr) {
        std::cout << "(headless: skip window)" << std::endl;
        return check_failed == 0 ? 0 : 1;
    }

    // ============================================================ 可视演示
    //主相机场景：三块色块（R / G / B），每块各自沿椭圆轨迹移动
    MiniScene scene(RANGE_W, RANGE_H, {SCREEN_WIDTH, SCREEN_HEIGHT});
    auto r_block = scene.addSolid({255, 255, 63, 63}, 500, 500, -400, -220);
    auto g_block = scene.addSolid({255, 63, 255, 63}, 500, 500, 0, 0);
    auto b_block = scene.addSolid({255, 63, 63, 255}, 500, 500, 400, 220);

    // 每个色块的初始中心，动画围绕它做椭圆运动（场景单位）
    struct Motion {
        MiniScene::SolidBlock block;
        double base_x, base_y; // 初始中心
        double radius_x, radius_y; // 椭圆半径
        double phase;              // 相位
        double speed;              // 角速度倍率
    };

    const Motion motions[] = {
        {r_block, -400, -220, 260, 150, 0.0, 1.0},
        {g_block, 0, 0, 300, 180, 2.1, 0.85},
        {b_block, 400, 220, 260, 150, 4.2, 1.15},
    };

    //画中画相机场景：底色铺满视口 + 两块绕圈运动的色块
    constexpr double PIP_RANGE_W = 900.0;
    constexpr double PIP_RANGE_H = 506.0;
    MiniScene mini(PIP_RANGE_W, PIP_RANGE_H, {PIP_WIDTH, PIP_HEIGHT});
    mini.addSolid({255, 250, 240, 120}, PIP_RANGE_W * 2.0, PIP_RANGE_H * 2.0, 0, 0);
    auto orbit_a = mini.addSolid({255, 60, 240, 250}, 260, 260, 0, 0);
    auto orbit_b = mini.addSolid({255, 250, 160, 60}, 140, 140, 0, 0);

    //画中画贴图（场景单位 640x360，会绕屏幕中心公转并循环缩放）
    constexpr double PIP_SCENE_W = 640.0;
    constexpr double PIP_SCENE_H = 360.0;
    constexpr double PIP_BORDER = 16.0; // 白色底框每边的宽度（场景单位）

    // 白底框与贴图共用同一个中心点，所以动画只需移动这一个点。
    auto pip_center = std::make_shared<Space::OffsetPoint>(scene.root, Space::Offset(0, 0));

    // 白色背景框：比贴图四周各大 PIP_BORDER，作为“相框”垫在贴图下面。
    // 先 addItem，所以它先画，贴图会盖在它上面。
    auto pip_frame_rect = std::make_shared<Item::Shape::Rectangle>(
        pip_center,
        static_cast<Val>(PIP_SCENE_W + PIP_BORDER * 2.0),
        static_cast<Val>(PIP_SCENE_H + PIP_BORDER * 2.0));

    auto pip_frame = std::make_shared<Item::General>(
        pip_frame_rect,
        std::vector<std::shared_ptr<Item::Renderer::Base>>{
            std::make_shared<Item::Renderer::SetColor>(Screen::Screen::Pixel{255, 255, 255, 255}),
            std::make_shared<Item::Renderer::Fill>()});
    scene.camera.getObject()->addItem(pip_frame);

    auto pip_image = std::make_shared<Item::Image>(
        Screen::PoSize{PIP_WIDTH, PIP_HEIGHT},
        Item::Shape::Rectangle(pip_center, static_cast<Val>(PIP_SCENE_W), static_cast<Val>(PIP_SCENE_H)));
    scene.camera.getObject()->addItem(pip_image);

    //拷贝模式：每帧 image.getScreen() << mini.getScreen()
    //引用模式：mini.setScreen(image.getScreen())，相机直接画进贴图，零拷贝
    bool copy_mode = true;
    auto mini_own_screen = std::make_shared<Screen::Screen>(Screen::PoSize{PIP_WIDTH, PIP_HEIGHT});

    //初始化SDL子系统
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return -1;
    }

    //创建窗口
    SDL_Window* gWindow = SDL_CreateWindow("Test 20261021-210918",
                                          SCREEN_WIDTH, SCREEN_HEIGHT,
                                          0);

    if (!gWindow) {
        SDL_Log("can not  create window:%s", SDL_GetError());
        SDL_Quit();
        return -1;
    }

    // 获取表面
    auto gWinSfc = SDL_GetWindowSurface(gWindow);
    if (!gWinSfc) {
        SDL_DestroyWindow(gWindow);
        SDL_Quit();
        return -1;
    }

    // ----- SDL3 从我们的缓冲创建 Surface -----
    SDL_Surface* surface = SDL_CreateSurfaceFrom(
        scene.camera.getScreen()->getSize().x,
        scene.camera.getScreen()->getSize().y,
        SDL_PIXELFORMAT_BGRA8888,
        scene.camera.getScreen()->getData(),
        static_cast<int>(scene.camera.getScreen()->getSize().x * 4));

    if (!surface) {
        std::cerr << "创建 Surface 失败: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(gWindow);
        SDL_Quit();
        return -1;
    }

    // 缓冲永远是不透明的最终合成结果，呈现时不需要 SDL 再做一次混合。
    if (!SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_NONE)) {
        std::cerr << "SetSurfaceBlendMode 失败: " << SDL_GetError() << std::endl;
    }

    std::thread fps_pt(fps_ptf);
    std::cout << "Space: toggle copy / ref mode, Esc: quit" << std::endl;

    double t = 0.0;
    double copy_ms_sum = 0.0;
    size_t copy_frames = 0;
    bool dumped = false;

    while (!isDone.load(std::memory_order_relaxed)) {
        //事件处理
        static SDL_Event ev = {};
        while (SDL_PollEvent(&ev) != 0) {
            if (ev.type == SDL_EVENT_QUIT) {
                isDone.store(true);
            } else if (ev.type == SDL_EVENT_KEY_DOWN && !ev.key.repeat) {
                if (ev.key.key == SDLK_ESCAPE) {
                    isDone.store(true);
                } else if (ev.key.key == SDLK_SPACE) {
                    copy_mode = !copy_mode;
                    if (copy_mode)
                        mini.camera.setScreen(mini_own_screen);
                    else
                        mini.camera.setScreen(pip_image->getScreen());
                    std::cout << "\nmode: " << (copy_mode ? "copy (operator<<)" : "ref (shared buffer)")
                              << " avg copy " << (copy_frames > 0 ? copy_ms_sum / static_cast<double>(copy_frames) : 0.0)
                              << " ms/frame" << std::endl;
                }
            }
        }

        t += 0.016;

        // ---------- 主场景：R/G/B 三块各自绕初始中心做椭圆运动 ----------
        for (const auto& m : motions) {
            const double a = t * m.speed + m.phase;
            m.block.shape->setCenter(std::make_shared<Space::OffsetPoint>(
                scene.root,
                Space::Offset(m.base_x + std::cos(a) * m.radius_x,
                              m.base_y + std::sin(a * 1.4) * m.radius_y)));
        }

        // ---------- 画中画：白框 + 贴图共用同一个中心点，只动这个点 ----------
        {
            constexpr double ORBIT_X = 560.0;
            constexpr double ORBIT_Y = 300.0;
            const double breath = 1.0 + 0.18 * std::sin(t * 1.7);

            // 公转：改共用点的偏移即可，白框与贴图会一起移动
            pip_center->setOffset(Space::Offset(std::cos(t * 0.6) * ORBIT_X, std::sin(t * 0.6) * ORBIT_Y));

            // 呼吸缩放：贴图与白框同步改变宽高，白框始终保持 PIP_BORDER 的恒定厚度
            const auto w = static_cast<Val>(PIP_SCENE_W * breath);
            const auto h = static_cast<Val>(PIP_SCENE_H * breath);

            auto& img_rect = pip_image->getShapeRef();
            img_rect.setWidth(w);
            img_rect.setHeight(h);

            pip_frame_rect->setWidth(w + static_cast<Val>(PIP_BORDER * 2.0));
            pip_frame_rect->setHeight(h + static_cast<Val>(PIP_BORDER * 2.0));
        }

        //画中画场景动画
        const double orbit = 200.0;
        orbit_a.shape->setCenter(
            std::make_shared<Space::OffsetPoint>(mini.root, Space::Offset(std::cos(t) * orbit, std::sin(t) * orbit)));
        orbit_b.shape->setCenter(
            std::make_shared<Space::OffsetPoint>(mini.root, Space::Offset(std::cos(t * 2.0 + 1.0) * orbit,
                                                                        std::sin(t * 2.0 + 1.0) * orbit)));

        mini.camera.getTimePoint()->add();
        if (copy_mode) {
            mini.camera.getScreen()->clear();
            mini.camera.Shoot();

            // 关键一行：把画中画的渲染结果拷进贴图的屏幕
            const auto t0 = std::chrono::steady_clock::now();
            pip_image->getScreen() << mini.camera.getScreen();
            const auto t1 = std::chrono::steady_clock::now();
            copy_ms_sum += std::chrono::duration<double, std::milli>(t1 - t0).count();
            ++copy_frames;
        } else {
            // ref 模式：mini 的屏幕就是贴图的屏幕，直接渲染
            mini.camera.getScreen()->clear();
            mini.camera.Shoot();
        }

        scene.camera.getTimePoint()->add();
        scene.camera.getScreen()->clear();
        scene.camera.Shoot();

        SDL_BlitSurface(surface, nullptr, gWinSfc, nullptr);
        SDL_UpdateWindowSurface(gWindow);
        frame_count.fetch_add(1, std::memory_order_relaxed);

        // 无交互环境下的可视验证：XII_PIP_DUMP=/path/frame.bmp
        // 可选 XII_PIP_DUMP_FRAME=N：等到第 N 帧再导出（验证动画确实在动）
        if (!dumped) {
            if (const char* dump_path = std::getenv("XII_PIP_DUMP"); dump_path != nullptr) {
                long dump_at = 0;
                if (const char* at = std::getenv("XII_PIP_DUMP_FRAME"); at != nullptr)
                    dump_at = std::strtol(at, nullptr, 10);

                if (static_cast<long>(frame_count.load(std::memory_order_relaxed)) >= dump_at) {
                    if (SDL_SaveBMP(surface, dump_path))
                        std::cout << "dumped frame to " << dump_path << " at t=" << t << std::endl;
                    else
                        std::cerr << "SDL_SaveBMP failed: " << SDL_GetError() << std::endl;
                    dumped = true;
                    if (const char* only = std::getenv("XII_PIP_DUMP_ONLY"); only != nullptr && std::string(only) == "1")
                        isDone.store(true);
                }
            }
        }
    }

    //停止帧监控
    isDone.store(true, std::memory_order_relaxed);
    fps_pt.join();

    if (copy_frames > 0)
        std::cout << "copy: avg " << std::fixed << std::setprecision(3)
                  << copy_ms_sum / static_cast<double>(copy_frames) << " ms/frame ("
                  << copy_frames << " frames)" << std::endl;

    //销毁窗口
    SDL_DestroySurface(surface);
    SDL_DestroyWindow(gWindow);
    //清理所有初始化的子系统
    SDL_Quit();

    return check_failed == 0 ? 0 : 1;
}
