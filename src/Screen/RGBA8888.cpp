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
        std::fill_n(_data, _size.x * _size.y, Pixel{0, 0, 0, 0});
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
        const auto copy_w = std::min({size.x, src_screen._size.x - src_pos.x, dest_screen._size.x - dest_pos.x});
        const auto copy_h = std::min({size.y, src_screen._size.y - src_pos.y, dest_screen._size.y - dest_pos.y});

        for (size_t y = 0; y < copy_h; y++)
            std::copy_n(src_screen._data + src_screen._size.x * (src_pos.y + y), copy_w, dest_screen._data + src_screen._size.y * (dest_pos.y + y));
    }

    void RGBA8888::clear() {
        clear();
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
        auto a_a_ = a.A / 255.0f;
        auto b_a_ = b.A / 255.0f;

        float vr = a.R * a_a_ + b.R * b_a_;
        float vg = a.G * a_a_ + b.G * b_a_;
        float vb = a.B * a_a_ + b.B * b_a_;

        Pixel pixel;
        pixel.R = static_cast<uint8_t>(std::clamp(vr, 0.0f, 255.0f));
        pixel.G = static_cast<uint8_t>(std::clamp(vg, 0.0f, 255.0f));
        pixel.B = static_cast<uint8_t>(std::clamp(vb, 0.0f, 255.0f));
        pixel.A = 255;
        return pixel;
    }

    RGBA8888::Pixel RGBA8888::_blend_alpha_pixel(const Pixel& src, const Pixel& dest) {
        auto sa = src.A / 255.0f;
        auto ia = 1.0f - sa;

        uint8_t R = std::lround(src.R * sa + dest.R * ia);
        uint8_t G = std::lround(src.G * sa + dest.G * ia);
        uint8_t B = std::lround(src.B * sa + dest.B * ia);
        uint8_t A = std::lround(src.A * sa + dest.A * ia);
        return {.A = A, .R = R, .G = G, .B = B};
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
