//
// Created by qpwser on 10/1/26.
//

#pragma once
#include "PoSize.h"
#include <cstdint>

namespace xII::Screen {
    class RGBA8888 {
    public:
        struct Pixel {
            uint8_t A, R, G, B;
        };

    private:
        Pixel* _data;
        PoSize _size;

        void _clear();
        void _resize(const PoSize& new_size);

        void _fill(const Pixel& pixel);
        void _fill(const Pixel& pixel, const PoSize& pos, const PoSize& size);
        void _set(const PoSize& pos, const Pixel& pixel);

        static void _copy(const RGBA8888& src_screen, RGBA8888& dest_screen);
        static void _copy(const RGBA8888& src_screen, RGBA8888& dest_screen, const PoSize& src_pos, const PoSize& dest_pos, const PoSize& size);

        static Pixel _blend_alpha_pixel_(const Pixel& a, const Pixel& b);
        static Pixel _blend_alpha_pixel(const Pixel& src, const Pixel& dest);
        static void _blend_alpha_(const RGBA8888& a, RGBA8888& b);
        static void _blend_alpha(const RGBA8888& src, RGBA8888& dest);

    public:
        void clear();
        void resize(const PoSize& new_size);

        void fill(const Pixel& pixel);
        void fill(const Pixel& pixel, const PoSize& pos, const PoSize& size);
        void set(const PoSize& pos, const Pixel& pixel);

        static void copy(const RGBA8888& src_screen, RGBA8888& dest_screen, const PoSize& src_pos, const PoSize& dest_pos, const PoSize& size);
        static void copy(const RGBA8888& src_screen, RGBA8888& dest_screen);

        static void blend_alpha(const RGBA8888& src, RGBA8888& dest);
        static Pixel blend_alpha_pixel(const Pixel& src, const Pixel& dest);

    public:
        RGBA8888() = default;
        RGBA8888(const RGBA8888& s)
            : _data(new Pixel[s._size.x * s._size.y]),
              _size(s._size) { copy(s, *this); }
        explicit RGBA8888(const PoSize& size)
            : _data(new Pixel[size.x * size.y]),
              _size(size) {}

        ~RGBA8888() = default;

        [[nodiscard]] Pixel& at(const PoSize& pos) { return _data[pos.y * _size.x + pos.x]; }
        [[nodiscard]] const Pixel& at(const PoSize& pos) const { return _data[pos.y * _size.x + pos.x]; }

        [[nodiscard]] PoSize& getSize() { return _size; }
        [[nodiscard]] const PoSize& getSize() const { return _size; }

        [[nodiscard]] Pixel* getData() { return _data; }
        [[nodiscard]] const Pixel* getData() const { return _data; }
    };
} // namespace xII::Screen
