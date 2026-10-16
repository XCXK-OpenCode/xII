#include "Rainbow.h"

#include <cmath>
#include <algorithm>

xII::Screen::RGBA8888::Pixel hsv_to_rgba(double h, double s, double v, uint8_t a = 255)
{
    // 归一化色相到 [0, 360)
    h = std::fmod(h, 360.0);
    if (h < 0.0) h += 360.0;

    const double c  = v * s;
    const double hp = h / 60.0;
    const double x  = c * (1.0 - std::fabs(std::fmod(hp, 2.0) - 1.0));
    const double m  = v - c;

    double r = 0.0, g = 0.0, b = 0.0;
    switch (static_cast<int>(hp)) {
        case 0: r = c; g = x; b = 0; break;
        case 1: r = x; g = c; b = 0; break;
        case 2: r = 0; g = c; b = x; break;
        case 3: r = 0; g = x; b = c; break;
        case 4: r = x; g = 0; b = c; break;
        default:r = c; g = 0; b = x; break;
    }

    auto u8 = [](double f) {
        const int i = static_cast<int>(std::lround(f * 255.0));
        return static_cast<uint8_t>(std::clamp(i, 0, 255));
    };

    return xII::Screen::RGBA8888::Pixel{
        a,           // A
        u8(r + m),   // R
        u8(g + m),   // G
        u8(b + m)    // B
    };
}


namespace xII::Item::Renderer {
    void Rainbow1::Render(Camera::Base& camera, std::vector<std::shared_ptr<Space::Point>>& points, Args &args, const Screen::PoSize& item_pos, const Screen::PoSize& item_size) const {
        args.getColor = [](const Args::DrawContext& ctx) -> Screen::RGBA8888::Pixel {
            // 当前像素相对 item 原点的位置
            const double lx = static_cast<double>(ctx.screen_pos.x - ctx.item_pos.x);
            const double ly = static_cast<double>(ctx.screen_pos.y - ctx.item_pos.y);

            // 归一化到 [0,1)，item 宽/高各走一个周期（或合成一个相位）
            const double sx = (ctx.item_size.x > 0)
                ? lx / static_cast<double>(ctx.item_size.x) : 0.0;
            const double sy = (ctx.item_size.y > 0)
                ? ly / static_cast<double>(ctx.item_size.y) : 0.0;

            // 这里选一种「彩虹方向」：
            const double t = (sx + sy) * 0.5;        // 斜向渐变，0→1
            // 或 const double t = sx;               // 横向
            // 或 const double t = sy;               // 纵向
            // 或 const double t = std::hypot(sx-0.5, sy-0.5) * 2.0;  // 径向

            const double hue = t * 360.0;            // 一个周期 360°
            return hsv_to_rgba(hue, 1.0, 1.0, 255);
        };
    }
} // namespace xII::Item::Renderer