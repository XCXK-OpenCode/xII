// 
// Created by qpwser from XCXK on 2026-10-21
// Powered by DeepSeek V4.1 Flash
//

// Item::General 矩形压力测试。
//
// 按 --count 创建 count 个纯色矩形（Item::General + SetColor + Fill）：
//   * 颜色随机（RGB 全随机 + 随机透明度）
//   * 尺寸在 [--min-size, --max-size] 内随机；给了 --weights 时改为
//     「按权重抽自动提取的采样点」（第 k 个点在 k/(N+1) 处）
//   * 各自带随机速度漂移，撞视口边界反弹
// 每帧统计 Item::isInRange 的可见/剔除数量，并输出 clear / draw / total 耗时。
// Ctrl + 滚轮缩放相机（只改相机视口，不动世界），Ctrl+0 复位。
//
// 时间是固定的 1/60 步长，因此同样的 --seed 在任何机器上都得到同样的画面。
//
// 用法见 `--help`。

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <memory>
#include <numbers>
#include <random>
#include <sstream>
#include <string>
#include <vector>

#include "Camera/Standard.h"
#include "Item/Base.h"
#include "Item/General.h"
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

namespace {
    // ---------------------------------------------------------------- 常量
    constexpr int SCREEN_WIDTH = 1280;
    constexpr int SCREEN_HEIGHT = 720;

    constexpr double RANGE_W = 1920.0; // 相机初始视口（场景单位）
    constexpr double RANGE_H = 1080.0;

    // 相机缩放范围与步长（Ctrl + 滚轮）
    constexpr double MIN_VIEW_W = 120.0;
    constexpr double MAX_VIEW_W = 7680.0;
    constexpr double ZOOM_STEP = 1.25;

    // 世界尺寸范围（Alt + 滚轮），单位是“相机初始视口的倍数”
    constexpr double MIN_WORLD_SCALE = 0.25;
    constexpr double MAX_WORLD_SCALE = 16.0;

    constexpr double DT = 1.0 / 60.0;  // 固定步长：同种子 -> 同画面

    using Val = xII::Base::Quantity::Base_Value_Type;
    using Pixel = xII::Screen::Screen::Pixel;

    // ---------------------------------------------------------------- 参数
    struct Options {
        int count = 200;
        long seed = 0;
        double min_size = 40.0;
        double max_size = 180.0;
        int min_alpha = 96;
        int frames = -1;             // < 0 表示一直运行
        std::string dump;            // 非空则导出 BMP
        std::vector<double> weights; // 空 = 尺寸在 [min,max] 内均匀分布
        double view_width = RANGE_W; // 初始相机视口宽度（可脚本化验证缩放）
        double world_scale = 2.0;    // 世界尺寸 = RANGE_W * world_scale
        bool help = false;
    };

    void printHelp(const char* exe) {
        std::cout <<
            "Usage: " << exe << " [options]\n"
            "\n"
            "Creates N randomly colored rectangles that drift and bounce inside a\n"
            "world `world` times larger than the camera viewport, so some of them\n"
            "leave the viewport and get culled by Item::isInRange().\n"
            "\n"
            "Options:\n"
            "  --count N        number of blocks                    (default 200)\n"
            "  --seed S         RNG seed; same seed = same picture  (default: time, 0 = time)\n"
            "  --min-size D     minimum edge length, scene units    (default 40)\n"
            "  --max-size D     maximum edge length, scene units    (default 180)\n"
            "  --weights LIST   comma separated probabilities, e.g. 0.1,0.2,0.3,0.4\n"
            "                   N weights => N sample points auto-placed at\n"
            "                   1/(N+1) .. N/(N+1) of the size range, picked by\n"
            "                   weight (normalized), instead of a uniform size.\n"
            "                   default: uniform in [min-size, max-size]\n"
            "  --min-alpha A    minimum alpha, 0..255               (default 96)\n"
            "  --view W         initial camera view width, scene units\n"
            "                   (default 1920; also the value Ctrl + 0 resets to)\n"
            "  --world K        world size = viewport * K           (default 2.0)\n"
            "                   (also the value Alt + 0 resets to)\n"
            "  --frames N       run N frames then exit              (default: run forever)\n"
            "  --dump PATH      save the last rendered frame to a BMP file\n"
            "  --help           show this message and exit\n"
            "\n"
            "Both `--opt value` and `--opt=value` are accepted.\n"
            "\n"
            "Keys (interactive mode):\n"
            "  drag left/right      pan the camera (content follows the cursor)\n"
            "  Ctrl + wheel         zoom camera in / out (center anchored)\n"
            "  Ctrl + 0             reset camera position and zoom\n"
            "  Alt + wheel          grow / shrink the world on the fly\n"
            "  Alt + 0              reset world size\n"
            "  Esc / window close   quit\n";
    }

    [[noreturn]] void failUsage(const std::string& message) {
        std::cerr << "error: " << message << "\n"
                  << "try --help" << std::endl;
        std::exit(2);
    }

    // 解析 "0.1,0.2,0.3,0.4"（允许逗号前后有空格）。
    std::vector<double> parseWeightList(const std::string& text) {
        std::vector<double> weights;
        std::stringstream stream(text);
        std::string item;

        while (std::getline(stream, item, ',')) {
            const auto first = item.find_first_not_of(" \t");
            if (first == std::string::npos)
                failUsage("--weights has an empty entry");

            const auto last = item.find_last_not_of(" \t");
            item = item.substr(first, last - first + 1);

            size_t used = 0;
            double value = 0.0;
            try {
                value = std::stod(item, &used);
            } catch (const std::exception&) {
                failUsage("--weights: '" + item + "' is not a number");
            }
            if (used != item.size())
                failUsage("--weights: '" + item + "' is not a number");
            if (value < 0.0)
                failUsage("--weights values must be >= 0");

            weights.push_back(value);
        }

        if (weights.empty())
            failUsage("--weights needs at least one value");
        return weights;
    }

    Options parseArgs(int argc, char** argv) {
        Options opt;

        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            std::string value;

            // 支持 --opt=value
            if (const auto eq = arg.find('='); eq != std::string::npos) {
                value = arg.substr(eq + 1);
                arg = arg.substr(0, eq);
            }

            const auto takeValue = [&](void) -> std::string {
                if (!value.empty())
                    return value;
                if (i + 1 >= argc)
                    failUsage(arg + " needs a value");
                return argv[++i];
            };

            if (arg == "--help" || arg == "-h") {
                opt.help = true;
            } else if (arg == "--count") {
                opt.count = std::stoi(takeValue());
            } else if (arg == "--seed") {
                opt.seed = std::stol(takeValue());
            } else if (arg == "--min-size") {
                opt.min_size = std::stod(takeValue());
            } else if (arg == "--max-size") {
                opt.max_size = std::stod(takeValue());
            } else if (arg == "--weights") {
                opt.weights = parseWeightList(takeValue());
            } else if (arg == "--min-alpha") {
                opt.min_alpha = std::stoi(takeValue());
            } else if (arg == "--view") {
                opt.view_width = std::stod(takeValue());
            } else if (arg == "--world") {
                opt.world_scale = std::stod(takeValue());
            } else if (arg == "--frames") {
                opt.frames = std::stoi(takeValue());
            } else if (arg == "--dump") {
                opt.dump = takeValue();
            } else {
                failUsage("unknown option '" + arg + "'");
            }
        }

        if (opt.count < 0)
            failUsage("--count must be >= 0");
        if (opt.min_size <= 0.0)
            failUsage("--min-size must be > 0");
        if (opt.max_size < opt.min_size)
            failUsage("--max-size must be >= --min-size");
        if (opt.min_alpha < 0 || opt.min_alpha > 255)
            failUsage("--min-alpha must be within 0..255");
        if (opt.view_width < MIN_VIEW_W || opt.view_width > MAX_VIEW_W)
            failUsage("--view must be within " + std::to_string(static_cast<long>(MIN_VIEW_W)) +
                      " .. " + std::to_string(static_cast<long>(MAX_VIEW_W)) + " scene units");
        if (opt.world_scale < MIN_WORLD_SCALE || opt.world_scale > MAX_WORLD_SCALE)
            failUsage("--world must be within " + std::to_string(MIN_WORLD_SCALE) +
                      " .. " + std::to_string(static_cast<long>(MAX_WORLD_SCALE)) +
                      " (multiples of the initial viewport)");
        if (opt.frames == 0)
            failUsage("--frames must be > 0 (or omitted to run forever)");

        return opt;
    }

    // ---------------------------------------------------------------- 尺寸分布
    // 把“权重”变成“自动提取的采样点”：
    //   N 个权重 -> N 个点，第 k 个点落在尺寸区间的 (k+1)/(N+1) 处。
    //   --weights 0.1,0.2,0.3,0.4
    //     => 4 个点，位于 20% / 40% / 60% / 80% 处，
    //        被抽中的概率分别是 0.1 / 0.2 / 0.3 / 0.4（自动归一化）。
    // 没给 --weights 时退化为 [min-size, max-size] 均匀分布。
    struct SizeSampler {
        bool weighted = false;
        std::vector<double> points;        // 自动提取出的尺寸点（场景单位）
        std::vector<double> fractions;     // 点在区间里的位置，0..1
        std::vector<double> probabilities; // 归一化后的概率
        std::vector<double> cumulative;    // 累积概率，用于抽样

        double min_size = 0.0;
        double max_size = 0.0;

        // 抽一个尺寸；hits 不为空时顺带累加命中计数（验证概率用）。
        double sample(std::mt19937& rng, std::vector<long>* hits = nullptr) const {
            if (!weighted) {
                std::uniform_real_distribution<double> dist(min_size, max_size);
                return dist(rng);
            }

            std::uniform_real_distribution<double> unit(0.0, 1.0);
            const double u = unit(rng);

            const auto it = std::upper_bound(cumulative.begin(), cumulative.end(), u);
            const size_t index = std::min<size_t>(
                static_cast<size_t>(std::distance(cumulative.begin(), it)), points.size() - 1);

            if (hits != nullptr && index < hits->size())
                ++(*hits)[index];

            return points[index];
        }
    };

    SizeSampler makeSampler(const Options& opt) {
        SizeSampler sampler;
        sampler.min_size = opt.min_size;
        sampler.max_size = opt.max_size;

        if (opt.weights.empty())
            return sampler;

        double total = 0.0;
        for (const double weight : opt.weights)
            total += weight;
        if (total <= 0.0)
            failUsage("--weights must contain at least one positive value");

        sampler.weighted = true;
        const size_t n = opt.weights.size();
        sampler.points.reserve(n);
        sampler.fractions.reserve(n);
        sampler.probabilities.reserve(n);
        sampler.cumulative.reserve(n);

        double running = 0.0;
        for (size_t k = 0; k < n; ++k) {
            // 自动提取点：第 k 个点位于区间的 (k+1)/(n+1) 处
            const double fraction = static_cast<double>(k + 1) / static_cast<double>(n + 1);
            sampler.fractions.push_back(fraction);
            sampler.points.push_back(opt.min_size + (opt.max_size - opt.min_size) * fraction);

            const double probability = opt.weights[k] / total;
            sampler.probabilities.push_back(probability);

            running += probability;
            sampler.cumulative.push_back(running);
        }

        return sampler;
    }

    // ---------------------------------------------------------------- 场景
    using Clock = std::chrono::steady_clock;

    double msSince(const Clock::time_point& from) {
        return std::chrono::duration<double, std::milli>(Clock::now() - from).count();
    }

    // 一个漂移中的方块：几何只建一次，之后每帧只改中心点的 offset。
    struct Block {
        std::shared_ptr<xII::Item::General> item;
        std::shared_ptr<xII::Item::Shape::Rectangle> rect;
        std::shared_ptr<xII::Space::OffsetPoint> center;

        double vx = 0.0; // 场景单位 / 秒
        double vy = 0.0;
        double half_w = 0.0;
        double half_h = 0.0;
    };

    struct Scene {
        std::shared_ptr<xII::Space::Point> root = std::make_shared<xII::Space::Point>();
        std::shared_ptr<xII::Time::Point> time_root = std::make_shared<xII::Time::Point>();

        // 相机中心点单独持有：拖动平移 / 复位都只改它的 offset。
        // Point 是活引用，Rectangle 每帧重新读取，无需重建相机。
        std::shared_ptr<xII::Space::OffsetPoint> camera_center =
            std::make_shared<xII::Space::OffsetPoint>(root, xII::Space::Offset(0, 0));

        xII::Camera::Standard camera{
            xII::Item::Shape::Rectangle(
                camera_center,
                static_cast<Val>(RANGE_W), static_cast<Val>(RANGE_H)),
            {SCREEN_WIDTH, SCREEN_HEIGHT},
            std::make_shared<xII::Item::Base>(),
            std::make_shared<xII::Time::OffsetPoint>(time_root, xII::Time::Offset(0))};

        std::vector<Block> blocks;
    };

    // ---------------------------------------------------------------- 相机缩放
    // 只改相机视口（保持 16:9），世界坐标不动。
    void setViewWidth(xII::Camera::Standard& camera, const double width) {
        const double clamped = std::clamp(width, MIN_VIEW_W, MAX_VIEW_W);
        const double height = clamped * SCREEN_HEIGHT / SCREEN_WIDTH;

        const auto shape = camera.getShape();
        // 中心点原样传回去（Point 是活引用，重建 Rectangle 不影响其它持有者）
        camera.setShape(xII::Item::Shape::Rectangle(
            shape.getCenter(), static_cast<Val>(clamped), static_cast<Val>(height)));
    }

    double viewWidth(const xII::Camera::Standard& camera) {
        return static_cast<double>(camera.getShape().getWidth());
    }

    // ---------------------------------------------------------------- 世界
    // 世界 = 相机初始视口 * scale，方块在里面漂移/反弹。
    // Alt + 滚轮只改 scale，相机视口由 Ctrl + 滚轮单独控制。
    struct World {
        double scale = 2.0;
        double half_w = RANGE_W * 0.5 * 2.0;
        double half_h = RANGE_H * 0.5 * 2.0;

        void setScale(const double new_scale) {
            scale = std::clamp(new_scale, MIN_WORLD_SCALE, MAX_WORLD_SCALE);
            half_w = RANGE_W * 0.5 * scale;
            half_h = RANGE_H * 0.5 * scale;
        }
    };

    // 把方块拉回世界内（缩小世界时必需）。
    // 方块比世界还大时把中心钉在 0，避免区间上下限反掉。
    void clampIntoWorld(Block& block, const World& world) {
        const auto offset = block.center->getOffset();
        const double limit_x = std::max(0.0, world.half_w - block.half_w);
        const double limit_y = std::max(0.0, world.half_h - block.half_h);

        block.center->setOffset(xII::Space::Offset(
            static_cast<Val>(std::clamp<double>(offset.getX(), -limit_x, limit_x)),
            static_cast<Val>(std::clamp<double>(offset.getY(), -limit_y, limit_y))));
    }

    // ---------------------------------------------------------------- 相机平移
    // 每屏幕像素对应的场景单位。
    double scenePerPixel(const xII::Camera::Standard& camera) {
        return viewWidth(camera) / static_cast<double>(SCREEN_WIDTH);
    }

    // 把相机中心夹在世界内，保证视口不跑出世界（世界比视口小时钉在 0）。
    void clampCameraIntoWorld(Scene& scene, const World& world) {
        const double half_view_w = viewWidth(scene.camera) / 2.0;
        const double half_view_h = half_view_w * SCREEN_HEIGHT / SCREEN_WIDTH;

        const double limit_x = std::max(0.0, world.half_w - half_view_w);
        const double limit_y = std::max(0.0, world.half_h - half_view_h);

        const auto offset = scene.camera_center->getOffset();
        scene.camera_center->setOffset(xII::Space::Offset(
            static_cast<Val>(std::clamp<double>(offset.getX(), -limit_x, limit_x)),
            static_cast<Val>(std::clamp<double>(offset.getY(), -limit_y, limit_y))));
    }

    // 拖动平移：内容跟随光标，所以相机中心按 -delta 移动。
    // 屏幕 y 向下 = 场景 y 增大（Point2PoSize 不翻转 y），因此 x/y 公式一致。
    void panCamera(Scene& scene, const World& world, const double dx_px, const double dy_px) {
        const double scale = scenePerPixel(scene.camera);
        const auto offset = scene.camera_center->getOffset();

        scene.camera_center->setOffset(xII::Space::Offset(
            static_cast<Val>(offset.getX() - dx_px * scale),
            static_cast<Val>(offset.getY() - dy_px * scale)));

        clampCameraIntoWorld(scene, world);
    }

    void resetCamera(Scene& scene, const Options& opt) {
        scene.camera_center->setOffset(xII::Space::Offset(0, 0));
        setViewWidth(scene.camera, opt.view_width);
    }

    Block makeBlock(Scene& scene, std::mt19937& rng, const Options& opt,
                    const SizeSampler& sampler, std::vector<long>* hits,
                    const World& world) {
        std::uniform_real_distribution<double> unit(0.0, 1.0);
        std::uniform_int_distribution<int> channel(0, 255);
        std::uniform_int_distribution<int> alpha(opt.min_alpha, 255);

        // 尺寸超出世界时会被 clamp，避免出生点区间反掉
        const double w = std::min(sampler.sample(rng, hits), world.half_w * 2.0);
        const double h = std::min(sampler.sample(rng, hits), world.half_h * 2.0);
        const double half_w = w / 2.0;
        const double half_h = h / 2.0;

        // 出生点：世界内均匀分布（保证整块都在世界内）
        const double x = std::uniform_real_distribution<double>(-world.half_w + half_w, world.half_w - half_w)(rng);
        const double y = std::uniform_real_distribution<double>(-world.half_h + half_h, world.half_h - half_h)(rng);

        // 速度：随机方向 + 随机速率
        const double angle = unit(rng) * 2.0 * std::numbers::pi;
        const double speed = std::uniform_real_distribution<double>(80.0, 420.0)(rng);

        const Pixel color{
            static_cast<uint8_t>(alpha(rng)),
            static_cast<uint8_t>(channel(rng)),
            static_cast<uint8_t>(channel(rng)),
            static_cast<uint8_t>(channel(rng))};

        auto center = std::make_shared<xII::Space::OffsetPoint>(
            scene.root, xII::Space::Offset(x, y));

        auto rect = std::make_shared<xII::Item::Shape::Rectangle>(
            center, static_cast<Val>(w), static_cast<Val>(h));

        auto item = std::make_shared<xII::Item::General>(
            rect,
            std::vector<std::shared_ptr<xII::Item::Renderer::Base>>{
                std::make_shared<xII::Item::Renderer::SetColor>(color),
                std::make_shared<xII::Item::Renderer::Fill>()});

        scene.camera.getObject()->addItem(item);

        return {item, rect, center, std::cos(angle) * speed, std::sin(angle) * speed, half_w, half_h};
    }

    void advance(Block& block, const double dt, const World& world) {
        const auto offset = block.center->getOffset(); // 当前偏移
        double x = offset.getX() + block.vx * dt;
        double y = offset.getY() + block.vy * dt;

        // 撞世界边界反弹；方块比世界大时把边界压到 0（不会反掉）
        const double limit_x = std::max(0.0, world.half_w - block.half_w);
        const double limit_y = std::max(0.0, world.half_h - block.half_h);

        if (x < -limit_x) {
            x = -limit_x;
            block.vx = std::abs(block.vx);
        } else if (x > limit_x) {
            x = limit_x;
            block.vx = -std::abs(block.vx);
        }

        if (y < -limit_y) {
            y = -limit_y;
            block.vy = std::abs(block.vy);
        } else if (y > limit_y) {
            y = limit_y;
            block.vy = -std::abs(block.vy);
        }

        block.center->setOffset(xII::Space::Offset(static_cast<Val>(x), static_cast<Val>(y)));
    }

    // ---------------------------------------------------------------- 输出
    std::string trimNumber(const double value, const int precision = 3) {
        std::ostringstream out;
        out << std::fixed << std::setprecision(precision) << value;
        return out.str();
    }

    void printSummary(const Options& opt, const long frames,
                      const size_t visible, const size_t culled,
                      const double clear_ms, const double draw_ms, const double total_ms,
                      const SizeSampler& sampler, const std::vector<long>& hits,
                      const double view_w, const World& world) {
        const auto average = [frames](const double sum) {
            return frames > 0 ? sum / static_cast<double>(frames) : 0.0;
        };

        const double total_avg = average(total_ms);
        const double fps = total_avg > 0.0 ? 1000.0 / total_avg : 0.0;

        std::cout << "\n=== summary ===\n"
                  << "seed          " << opt.seed << "\n"
                  << "blocks        " << opt.count << "\n"
                  << "view width    " << trimNumber(view_w, 1) << " scene units\n"
                  << "world         " << trimNumber(world.scale, 2) << " x viewport  ("
                  << trimNumber(world.half_w * 2.0, 1) << " x " << trimNumber(world.half_h * 2.0, 1)
                  << " scene units)\n"
                  << "size range    " << trimNumber(opt.min_size, 1) << " .. "
                  << trimNumber(opt.max_size, 1) << " scene units"
                  << (sampler.weighted ? " (weighted)" : " (uniform)") << "\n";

        // 权重的自动取点结果，并用实际命中数验证概率是否生效
        if (sampler.weighted) {
            long total_hits = 0;
            for (const long hit : hits)
                total_hits += hit;

            std::cout << "sample points " << sampler.points.size()
                      << " auto-extracted at k/(N+1)\n";

            for (size_t k = 0; k < sampler.points.size(); ++k) {
                const double actual =
                    total_hits > 0 ? static_cast<double>(hits[k]) / static_cast<double>(total_hits) : 0.0;
                const long drawn = static_cast<long>(
                    std::llround(opt.count * 2 * sampler.probabilities[k]));

                std::cout << "  #" << (k + 1)
                          << "  at " << std::setw(5) << trimNumber(sampler.fractions[k] * 100.0, 1) << "%"
                          << "  size " << std::setw(7) << trimNumber(sampler.points[k], 1)
                          << "  weight " << std::setw(5) << trimNumber(opt.weights[k], 3)
                          << "  expect " << std::setw(5) << trimNumber(sampler.probabilities[k] * 100.0, 1) << "%"
                          << " (" << std::setw(5) << drawn << ")"
                          << "  actual " << std::setw(5) << trimNumber(actual * 100.0, 1) << "%"
                          << " (" << std::setw(5) << hits[k] << ")\n";
            }
        }

        std::cout << "frames        " << frames << "\n"
                  << "visible       " << visible << "   (last frame, passes isInRange)\n"
                  << "culled        " << culled << "   (last frame, skipped)\n"
                  << "clear         " << trimNumber(average(clear_ms)) << " ms/frame\n"
                  << "draw          " << trimNumber(average(draw_ms)) << " ms/frame\n"
                  << "total         " << trimNumber(total_avg) << " ms/frame  ("
                  << trimNumber(fps, 1) << " FPS)\n"
                  << "              render only: SDL present is not included\n"
                  << "===============" << std::endl;
    }
} // namespace

int main(int argc, char** argv) {
    Options opt = parseArgs(argc, argv);
    if (opt.help) {
        printHelp(argv[0]);
        return 0;
    }

    // 无 --seed 时用当前时间，并打印出来以便复现
    if (opt.seed == 0)
        opt.seed = static_cast<long>(std::time(nullptr));

    std::mt19937 rng(static_cast<std::mt19937::result_type>(opt.seed));

    const SizeSampler sampler = makeSampler(opt);
    std::vector<long> hits(sampler.points.size(), 0); // 每个采样点的实际命中次数

    // 世界 = 初始视口 * --world；Alt + 滚轮运行时改它
    World world;
    world.setScale(opt.world_scale);

    std::cout << "Test 20261021-213739: " << opt.count << " drifting blocks, seed " << opt.seed
              << (sampler.weighted ? ", weighted sizes" : ", uniform sizes")
              << ", world " << std::to_string(world.scale) << "x viewport" << std::endl;

    Scene scene;
    scene.blocks.reserve(static_cast<size_t>(opt.count));
    for (int i = 0; i < opt.count; ++i)
        scene.blocks.push_back(makeBlock(scene, rng, opt, sampler, &hits, world));

    if (opt.view_width != RANGE_W)
        setViewWidth(scene.camera, opt.view_width);

    // ---------------------------------------------------------------- 窗口
    const bool interactive = opt.frames < 0;

    if (!SDL_Init(SDL_INIT_VIDEO))
        std::cerr << "SDL_Init warning: " << SDL_GetError() << std::endl;

    SDL_Window* window = nullptr;
    SDL_Surface* window_surface = nullptr;
    if (interactive) {
        window = SDL_CreateWindow("Test 20261021-213739", SCREEN_WIDTH, SCREEN_HEIGHT, 0);
        if (!window) {
            std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << std::endl;
            SDL_Quit();
            return 1;
        }
        window_surface = SDL_GetWindowSurface(window);
        if (!window_surface) {
            std::cerr << "SDL_GetWindowSurface failed: " << SDL_GetError() << std::endl;
            SDL_DestroyWindow(window);
            SDL_Quit();
            return 1;
        }
    }

    // 我们的缓冲始终是不透明的最终合成结果，呈现时不需要 SDL 再混合一次
    SDL_Surface* surface = SDL_CreateSurfaceFrom(
        SCREEN_WIDTH, SCREEN_HEIGHT, SDL_PIXELFORMAT_BGRA8888,
        scene.camera.getScreen()->getData(), SCREEN_WIDTH * 4);
    if (!surface) {
        std::cerr << "SDL_CreateSurfaceFrom failed: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_NONE);

    // ---------------------------------------------------------------- 主循环
    long frames = 0;
    long next_report = 1; // 下一份每秒统计的时间点（秒）
    size_t last_visible = 0;
    size_t last_culled = 0;

    double clear_sum = 0.0;
    double draw_sum = 0.0;
    double total_sum = 0.0;

    const auto start = Clock::now();
    bool running = true;

    if (interactive)
        std::cout << "Drag (left/right): pan camera / Ctrl + wheel: zoom / Alt + wheel: world size\n"
                     "Ctrl + 0: reset camera / Alt + 0: reset world / Esc: quit\n" << std::flush;

    // 拖动平移状态
    bool dragging = false;
    float drag_x = 0.0f;
    float drag_y = 0.0f;

    while (running) {
        if (interactive) {
            SDL_Event ev{};
            while (SDL_PollEvent(&ev) != 0) {
                if (ev.type == SDL_EVENT_QUIT) {
                    running = false;
                } else if (ev.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                    if (ev.button.button == SDL_BUTTON_LEFT || ev.button.button == SDL_BUTTON_RIGHT) {
                        dragging = true;
                        drag_x = ev.button.x;
                        drag_y = ev.button.y;
                    }
                } else if (ev.type == SDL_EVENT_MOUSE_BUTTON_UP) {
                    if (ev.button.button == SDL_BUTTON_LEFT || ev.button.button == SDL_BUTTON_RIGHT)
                        dragging = false;
                } else if (ev.type == SDL_EVENT_MOUSE_MOTION) {
                    if (dragging) {
                        // 内容跟随光标：向右拖 = 相机向左移
                        panCamera(scene, world, static_cast<double>(ev.motion.x - drag_x),
                                  static_cast<double>(ev.motion.y - drag_y));
                        drag_x = ev.motion.x;
                        drag_y = ev.motion.y;
                    }
                } else if (ev.type == SDL_EVENT_MOUSE_WHEEL) {
                    // SDL3 的 wheel 事件没有 mod 字段，要查全局修饰键
                    const SDL_Keymod mod = SDL_GetModState();

                    if ((mod & SDL_KMOD_CTRL) != 0 && ev.wheel.y != 0.0f) {
                        // Ctrl + 滚轮：只改相机视口（上滚放大）
                        const double factor = std::pow(ZOOM_STEP, -static_cast<double>(ev.wheel.y));
                        setViewWidth(scene.camera, viewWidth(scene.camera) * factor);
                        clampCameraIntoWorld(scene, world);
                        std::cout << "\nview width: " << trimNumber(viewWidth(scene.camera), 1)
                                  << " scene units  (x" << trimNumber(RANGE_W / viewWidth(scene.camera), 2)
                                  << " relative to the initial view)" << std::endl;
                    } else if ((mod & SDL_KMOD_ALT) != 0 && ev.wheel.y != 0.0f) {
                        // Alt + 滚轮：快速改世界尺寸（上滚变大），并把方块/相机拉回新世界内
                        world.setScale(world.scale * std::pow(ZOOM_STEP, static_cast<double>(ev.wheel.y)));
                        for (auto& block : scene.blocks)
                            clampIntoWorld(block, world);
                        clampCameraIntoWorld(scene, world);
                        std::cout << "\nworld: " << trimNumber(world.scale, 2) << " x viewport  ("
                                  << trimNumber(world.half_w * 2.0, 1) << " x "
                                  << trimNumber(world.half_h * 2.0, 1) << " scene units)" << std::endl;
                    }
                } else if (ev.type == SDL_EVENT_KEY_DOWN && !ev.key.repeat) {
                    if (ev.key.key == SDLK_ESCAPE) {
                        running = false;
                    } else if (ev.key.key == SDLK_0 && (ev.key.mod & SDL_KMOD_CTRL) != 0) {
                        // 相机复位：位置 + 缩放一起回初始值
                        resetCamera(scene, opt);
                        std::cout << "\ncamera reset (center 0,0 / view width "
                                  << trimNumber(viewWidth(scene.camera), 1) << ")" << std::endl;
                    } else if (ev.key.key == SDLK_0 && (ev.key.mod & SDL_KMOD_ALT) != 0) {
                        world.setScale(opt.world_scale);
                        for (auto& block : scene.blocks)
                            clampIntoWorld(block, world);
                        clampCameraIntoWorld(scene, world);
                        std::cout << "\nworld reset to " << trimNumber(world.scale, 2)
                                  << " x viewport" << std::endl;
                    }
                }
            }
            if (!running)
                break;
        }

        for (auto& block : scene.blocks)
            advance(block, DT, world);

        scene.camera.getTimePoint()->add();

        const auto frame_start = Clock::now();
        scene.camera.getScreen()->clear();
        const double clear_ms = msSince(frame_start);

        const auto draw_start = Clock::now();
        scene.camera.Shoot();
        const double draw_ms = msSince(draw_start);

        const double total_ms = msSince(frame_start);

        clear_sum += clear_ms;
        draw_sum += draw_ms;
        total_sum += total_ms;
        ++frames;

        if (window_surface != nullptr) {
            SDL_BlitSurface(surface, nullptr, window_surface, nullptr);
            SDL_UpdateWindowSurface(window);
        }

        // 剔除统计放在计时区外，避免污染上面的耗时
        size_t visible = 0;
        for (const auto& block : scene.blocks)
            if (block.item->isInRange(scene.camera))
                ++visible;
        last_visible = visible;
        last_culled = scene.blocks.size() - visible;

        // 每秒一行实时统计
        const double elapsed_s =
            std::chrono::duration<double>(Clock::now() - start).count();
        while (elapsed_s >= static_cast<double>(next_report)) {
            const double mean_total = total_sum / static_cast<double>(frames);
            std::cout << "[t=" << std::setw(4) << next_report << "s]"
                      << " frames " << std::setw(6) << frames
                      << "  visible " << std::setw(6) << last_visible
                      << "  culled " << std::setw(6) << last_culled
                      << "  view " << std::setw(7) << trimNumber(viewWidth(scene.camera), 0)
                      << "  world " << std::setw(6) << trimNumber(world.scale, 2)
                      << "  cam (" << trimNumber(scene.camera_center->getX(), 0) << ","
                      << trimNumber(scene.camera_center->getY(), 0) << ")"
                      << "  fps " << std::setw(6) << trimNumber(mean_total > 0.0 ? 1000.0 / mean_total : 0.0, 1)
                      << std::endl;
            ++next_report;
        }

        if (opt.frames > 0 && frames >= opt.frames)
            running = false;
    }

    // ---------------------------------------------------------------- 导出
    if (!opt.dump.empty()) {
        if (SDL_SaveBMP(surface, opt.dump.c_str()))
            std::cout << "dumped frame to " << opt.dump << " (" << frames << " frames rendered)" << std::endl;
        else
            std::cerr << "SDL_SaveBMP failed: " << SDL_GetError() << std::endl;
    }

    printSummary(opt, frames, last_visible, last_culled, clear_sum, draw_sum, total_sum,
                 sampler, hits, viewWidth(scene.camera), world);

    SDL_DestroySurface(surface);
    if (window != nullptr)
        SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
