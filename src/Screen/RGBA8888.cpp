//
// Created by qpwser on 10/1/26.
//

#include "RGBA8888.h"
#include "Screen/PoSize.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <stdexcept>

namespace xII::Screen {
    void RGBA8888::_clear() {
        // Pixel 的内存布局是 {A, R, G, B}（与 SDL_PIXELFORMAT_BGRA8888 的掩码一致）。
        // 清屏必须是“不透明黑色”，即 A=255、R=G=B=0；
        // 直接写 Pixel{0, 0, 0, 255} 会得到 A=0、B=255 的透明蓝色，
        // 后续 alpha 混合就会在透明背景上做，屏幕上会被当成纯色显示。
        std::fill_n(_data, _size.x * _size.y, Pixel{.A = 255, .R = 0, .G = 0, .B = 0});
    }

    void RGBA8888::_resize(const PoSize& new_size) {
        auto old_data = _data;
        _data = new Pixel[new_size.x * new_size.y];
        delete[] old_data;
        _size = new_size;
    }

    void RGBA8888::_fill(const Pixel& pixel) {
        std::fill_n(_data, _size.x * _size.y, pixel);
    }

    void RGBA8888::_fill(const Pixel& pixel, const PoSize& pos, const PoSize& size) {
        const auto copy_w = std::min(_size.x - pos.x, size.x);
        const auto copy_h = std::min(_size.y - pos.y, size.y);
        const auto end_y = pos.y + copy_h;

        for (size_t y = pos.y; y < end_y; y++)
            std::fill_n(_data + _size.x * y + pos.x, copy_w, pixel);
    }

    void RGBA8888::_set(const PoSize& pos, const Pixel& pixel) {
        _data[pos.y * _size.x + pos.x] = pixel;
    }

    void RGBA8888::_copy(const RGBA8888& src_screen, RGBA8888& dest_screen) {
        const auto copy_w = std::min(src_screen._size.x, dest_screen._size.x);
        const auto copy_h = std::min(src_screen._size.y, dest_screen._size.y);

        for (size_t y = 0; y < copy_h; y++)
            std::copy_n(src_screen._data + src_screen._size.x * y, copy_w, dest_screen._data + dest_screen._size.x * y);
    }

    void RGBA8888::_copy(const RGBA8888& src_screen, RGBA8888& dest_screen, const PoSize& src_pos, const PoSize& dest_pos, const PoSize& size) {
        // 位置越界时裁到 0（原先直接相减，越界会得到巨大值）。
        const int64_t copy_w = std::max<int64_t>(
            0, std::min({size.x, src_screen._size.x - src_pos.x, dest_screen._size.x - dest_pos.x}));
        const int64_t copy_h = std::max<int64_t>(
            0, std::min({size.y, src_screen._size.y - src_pos.y, dest_screen._size.y - dest_pos.y}));

        for (int64_t y = 0; y < copy_h; y++) {
            // 行宽必须用 _size.x（原先误用 _size.y，非正方形时整块错位），
            // 并且要加上 x 偏移（原先漏了，拷贝区总是贴在最左边）。
            const Pixel* src_row = src_screen._data + src_screen._size.x * (src_pos.y + y) + src_pos.x;
            Pixel* dest_row = dest_screen._data + dest_screen._size.x * (dest_pos.y + y) + dest_pos.x;
            std::copy_n(src_row, static_cast<size_t>(copy_w), dest_row);
        }
    }

    void RGBA8888::clear() {
        _clear();
    }

    void RGBA8888::resize(const PoSize& new_size) {
        _resize(new_size);
    }

    void RGBA8888::set(const PoSize& pos, const Pixel& pixel) {
        if (pos.x >= _size.x || pos.y >= _size.y)
            throw std::runtime_error("In function void xII::Screen::RGBA8888::set(const PoSize &pos, const Pixel &pixel): pos.x >= _size.x || pos.y >= _size.y");
        _set(pos, pixel);
    }

    void RGBA8888::fill(const Pixel& pixel) {
        _fill(pixel);
    }

    void RGBA8888::fill(const Pixel& pixel, const PoSize& pos, const PoSize& size) {
        if (pos.x + size.x >= _size.x || pos.y + size.y >= _size.y)
            throw std::runtime_error("In function void xII::Screen::RGBA8888::fill(const Pixel &pixel, const PoSize &pos, const PoSize &size): pos.x + size.x >= _size.x || pos.y + size.y >= _size.y");
        _fill(pixel, pos, size);
    }

    void RGBA8888::copy(const RGBA8888& src_screen, RGBA8888& dest_screen) {
        _copy(src_screen, dest_screen);
    }

    void RGBA8888::copy(const RGBA8888& src_screen, RGBA8888& dest_screen, const PoSize& src_pos, const PoSize& dest_pos, const PoSize& size) {
        _copy(src_screen, dest_screen, src_pos, dest_pos, size);
    }

    RGBA8888::Pixel RGBA8888::_blend_alpha_pixel_(const Pixel& a, const Pixel& b) {
        // 旧的浮点实现只做了加权相加（没有除以合成后的 alpha），
        // 在目标带 alpha 时会得到过亮的颜色，这里统一走正确的 source-over。
        return _blend_alpha_pixel(a, b);
    }

    inline RGBA8888::Pixel RGBA8888::_blend_alpha_pixel(const Pixel& src, const Pixel& dest) {
        const uint32_t sa = src.A;
        if (sa == 0)   return dest;   // 源全透明 → 结果就是 dest
        if (sa == 255) return src;    // 源全不透明 → 直接覆盖

        const uint32_t da     = dest.A;
        const uint32_t inv_sa = 255u - sa;

        // 标准 source-over（直通 alpha，非预乘）：
        //   out_a = sa + da * (1 - sa)
        //   out_c = (src_c * sa + dest_c * da * (1 - sa)) / out_a
        const uint32_t out_a = sa + (da * inv_sa + 127u) / 255u;

        const auto blend_channel = [&](uint32_t sc, uint32_t dc) -> uint8_t {
            const uint32_t dst_weighted = (dc * da * inv_sa + 127u) / 255u;
            const uint32_t value = (sc * sa + dst_weighted + out_a / 2u) / out_a;
            return static_cast<uint8_t>(std::min(value, 255u));
        };

        return {
            .A = static_cast<uint8_t>(out_a),
            .R = blend_channel(src.R, dest.R),
            .G = blend_channel(src.G, dest.G),
            .B = blend_channel(src.B, dest.B),
        };
    }

    void RGBA8888::_blend_alpha_(const RGBA8888& a, RGBA8888& b) {
        auto w = std::min(a._size.x, b._size.x);
        auto h = std::min(a._size.y, b._size.y);

        for (int64_t y = 0; y < h; y++)
            for (int64_t x = 0; x < w; x++)
                b.at({x, y}) = _blend_alpha_pixel_(a.at({x, y}), b.at({x, y}));
    }

    void RGBA8888::_blend_alpha(const RGBA8888& src, RGBA8888& dest) {
        auto w = std::min(src._size.x, dest._size.x);
        auto h = std::min(src._size.y, dest._size.y);

        for (int64_t y = 0; y < h; y++)
            for (int64_t x = 0; x < w; x++)
                dest.at({x, y}) = _blend_alpha_pixel(src.at({x, y}), dest.at({x, y}));
    }

    void RGBA8888::blend_alpha(const RGBA8888& src, RGBA8888& dest) {
        _blend_alpha(src, dest);
    }

    RGBA8888::Pixel RGBA8888::blend_alpha_pixel(const Pixel& src, const Pixel& dest) {
        return _blend_alpha_pixel(src, dest);
    }
} // namespace xII::Screen
