// 
// Created by qpwser from XCXK on 2026-10-20
// Powered by DeepSeek V4.1 Flash
//
// FreeType based text rendering for the CPU Screen backend.
//

#include "Text.h"

#include "Camera/Outline.h"
#include "Camera/Standard.h"
#include "Item/Renderer/Args.h"
#include "Item/Renderer/Line.h"
#include "Item/Shape/Rectangle.h"
#include "Screen/PointPos.h"
#include "Space/Point.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace {
    using xII::Item::Text;
    using xII::Screen::PoSize;
    using xII::Screen::RGBA8888;

    // FreeType 库只需要初始化一次；进程退出时由静态析构函数释放。
    struct FreeTypeLibrary {
        FT_Library library = nullptr;

        FreeTypeLibrary() {
            if (FT_Init_FreeType(&library) != 0)
                library = nullptr;
        }
        ~FreeTypeLibrary() {
            if (library != nullptr)
                FT_Done_FreeType(library);
        }

        FreeTypeLibrary(const FreeTypeLibrary&) = delete;
        FreeTypeLibrary& operator=(const FreeTypeLibrary&) = delete;
    };

    FT_Library getFreeTypeLibrary() {
        static FreeTypeLibrary instance;
        return instance.library;
    }

    // font_path 为空时按顺序尝试的字体。
    const char* const kFallbackFonts[] = {
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/TTF/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
        "/usr/share/fonts/opentype/noto/NotoSans-Regular.ttf",
        "/usr/share/fonts/noto/NotoSans-Regular.ttf",
    };

    // 极简 UTF-8 解码：非法序列返回 U+FFFD，并至少前进 1 字节，避免死循环。
    uint32_t decodeUtf8(const std::string& s, size_t& i) {
        const auto c = static_cast<unsigned char>(s[i]);
        if (c < 0x80) {
            ++i;
            return c;
        }

        int extra = 0;
        uint32_t cp = 0;
        if ((c & 0xE0) == 0xC0) {
            extra = 1;
            cp = c & 0x1Fu;
        } else if ((c & 0xF0) == 0xE0) {
            extra = 2;
            cp = c & 0x0Fu;
        } else if ((c & 0xF8) == 0xF0) {
            extra = 3;
            cp = c & 0x07u;
        } else {
            ++i;
            return 0xFFFD;
        }

        ++i;
        for (int k = 0; k < extra; ++k) {
            if (i >= s.size())
                return 0xFFFD;
            const auto cc = static_cast<unsigned char>(s[i]);
            if ((cc & 0xC0) != 0x80)
                return 0xFFFD;
            cp = (cp << 6) | (cc & 0x3Fu);
            ++i;
        }
        return cp;
    }

    double from26_6(FT_Pos v) {
        return static_cast<double>(v) / 64.0;
    }

    bool isSpaceCodepoint(const uint32_t cp) {
        return cp == ' ' || cp == 0x00A0 || cp == 0x3000;
    }

    // CJK 等字符可以在任意字之间换行。
    bool isCjkCodepoint(const uint32_t cp) {
        return (cp >= 0x1100 && cp <= 0x11FF) ||
               (cp >= 0x2E80 && cp <= 0x33FF) ||
               (cp >= 0x3400 && cp <= 0x4DBF) ||
               (cp >= 0x4E00 && cp <= 0x9FFF) ||
               (cp >= 0xAC00 && cp <= 0xD7AF) ||
               (cp >= 0xF900 && cp <= 0xFAFF) ||
               (cp >= 0xFE30 && cp <= 0xFE4F) ||
               (cp >= 0xFF00 && cp <= 0xFF64);
    }

    struct GlyphPlacement {
        uint32_t codepoint = 0;
        double x = 0.0;        // 相对文本块锚点的 x（排版单位）
        double baseline = 0.0; // 相对文本块顶部的基线 y（排版单位）
    };

    struct TextLayout {
        std::vector<GlyphPlacement> glyphs;
        double width = 0.0;      // 内容宽度（最大行宽）
        double height = 0.0;     // 内容高度（行数 * 行距）
        double ascent = 0.0;
        double line_step = 0.0;
        double box_width = 0.0;  // 对齐用框宽（排版单位）
    };

    struct LayoutOptions {
        Text::Align align = Text::Align::Left;
        double line_spacing = 0.0; // 排版单位
        double line_width = 0.0;   // 排版单位，<= 0 表示不自动换行
        double box_width = 0.0;    // 排版单位，<= 0 表示按内容宽度对齐
    };

    struct CharInfo {
        uint32_t codepoint = 0;
        double kern = 0.0;
        double advance = 0.0;
        bool space = false;
        bool break_before = false;
        bool break_after = false;
    };

    std::vector<CharInfo> measureCharacters(FT_Face face, const std::vector<uint32_t>& codepoints) {
        std::vector<CharInfo> infos;
        infos.reserve(codepoints.size());

        FT_UInt previous = 0;
        for (const uint32_t cp : codepoints) {
            CharInfo info;
            info.codepoint = cp;

            const FT_UInt glyph_index = FT_Get_Char_Index(face, cp);

            if (previous != 0 && glyph_index != 0 && FT_HAS_KERNING(face)) {
                FT_Vector delta{};
                // UNFITTED：不做整数取整，让排版随字号严格线性缩放
                if (FT_Get_Kerning(face, previous, glyph_index, FT_KERNING_UNFITTED, &delta) == 0)
                    info.kern = from26_6(delta.x);
            }

            // NO_HINTING：advance 与字号成严格比例，
            // 这样“场景单位排版”和“屏幕像素排版”得到一致的结果。
            if (FT_Load_Glyph(face, glyph_index, FT_LOAD_NO_BITMAP | FT_LOAD_NO_HINTING) == 0)
                info.advance = from26_6(face->glyph->advance.x);

            info.space = isSpaceCodepoint(cp);
            info.break_before = isCjkCodepoint(cp);
            info.break_after = info.space || isCjkCodepoint(cp);

            infos.push_back(info);
            previous = glyph_index;
        }
        return infos;
    }

    // 计算每个字形的放置位置。
    // 调用前必须已按目标字号设置好 face（FT_Set_Char_Size），
    // 返回坐标以该字号下的“排版单位”（= 该字号下的像素）表示。
    TextLayout buildLayout(FT_Face face, const std::string& text, const LayoutOptions& options) {
        TextLayout layout;
        if (face == nullptr || face->size == nullptr)
            return layout;

        layout.ascent = from26_6(face->size->metrics.ascender);
        layout.line_step = from26_6(face->size->metrics.height) + options.line_spacing;
        if (layout.line_step <= 0.0)
            layout.line_step = 1.0;

        // 1. 按 '\n' 拆成段落。
        std::vector<std::vector<uint32_t>> paragraphs(1);
        for (size_t i = 0; i < text.size();) {
            const uint32_t cp = decodeUtf8(text, i);
            if (cp == '\n') {
                paragraphs.emplace_back();
            } else if (cp == '\r') {
                continue;
            } else if (cp == '\t') {
                for (int k = 0; k < 4; ++k)
                    paragraphs.back().push_back(' ');
            } else {
                paragraphs.back().push_back(cp);
            }
        }

        struct LaidOutLine {
            std::vector<GlyphPlacement> glyphs;
            double width = 0.0;
        };
        std::vector<LaidOutLine> lines;

        // 2. 逐段断行。
        for (const auto& paragraph : paragraphs) {
            const std::vector<CharInfo> infos = measureCharacters(face, paragraph);
            const size_t n = infos.size();

            if (n == 0) {
                lines.emplace_back(); // 空行（连续的 '\n'）
                continue;
            }

            // pen[i] = 第 i 个字符左边缘的位置（含该字符的 kerning）
            std::vector<double> pen(n + 1, 0.0);
            for (size_t i = 0; i < n; ++i)
                pen[i + 1] = pen[i] + infos[i].kern + infos[i].advance;

            // 允许断行的位置：开头、空格之后、CJK 字符前后
            std::vector<size_t> breaks{0};
            for (size_t i = 0; i < n; ++i) {
                if (infos[i].break_before)
                    breaks.push_back(i);
                if (infos[i].break_after)
                    breaks.push_back(i + 1);
            }
            std::sort(breaks.begin(), breaks.end());
            breaks.erase(std::unique(breaks.begin(), breaks.end()), breaks.end());

            size_t start = 0;
            while (start < n) {
                while (start < n && infos[start].space)
                    ++start; // 行首空格不参与排版
                if (start >= n)
                    break;

                size_t end = n;
                if (options.line_width > 0.0 && pen[n] - pen[start] > options.line_width) {
                    size_t best = start; // start 表示“没有可用的断点”
                    for (const size_t candidate : breaks) {
                        if (candidate <= start)
                            continue;
                        if (pen[candidate] - pen[start] > options.line_width)
                            break;
                        best = candidate;
                    }

                    if (best <= start) {
                        // 单个“词”就超出行宽：硬断行（至少放一个字符，保证前进）
                        size_t k = start + 1;
                        while (k < n && pen[k] - pen[start] <= options.line_width)
                            ++k;
                        best = (k > start + 1) ? k - 1 : start + 1;
                    }
                    end = best;
                }

                while (end > start && infos[end - 1].space)
                    --end; // 行尾空格不计入宽度
                if (end <= start)
                    end = std::min(n, start + 1);

                LaidOutLine line;
                double x = 0.0;
                for (size_t i = start; i < end; ++i) {
                    if (i > start)
                        x += infos[i].kern;
                    line.glyphs.push_back(GlyphPlacement{infos[i].codepoint, x, 0.0});
                    x += infos[i].advance;
                }
                line.width = x;
                layout.width = std::max(layout.width, x);
                lines.push_back(std::move(line));

                start = end;
            }
        }

        if (lines.empty())
            return layout;

        size_t glyph_count = 0;
        for (const auto& line : lines)
            glyph_count += line.glyphs.size();
        if (glyph_count == 0)
            return layout;

        // 3. 水平对齐（相对框的左边缘）+ 基线定位。
        //    box_width <= 0 时按内容宽度对齐；所有坐标都是相对“文本块左上角”的偏移。
        const double box_w = (options.box_width > 0.0) ? options.box_width : layout.width;
        layout.box_width = box_w;

        for (size_t li = 0; li < lines.size(); ++li) {
            const double shift = (options.align == Text::Align::Center) ? (box_w - lines[li].width) / 2.0
                               : (options.align == Text::Align::Right)  ? (box_w - lines[li].width)
                                                                        : 0.0;
            const double baseline = layout.ascent + static_cast<double>(li) * layout.line_step;

            for (const auto& glyph : lines[li].glyphs)
                layout.glyphs.push_back(GlyphPlacement{glyph.codepoint, glyph.x + shift, baseline});
        }

        layout.height = static_cast<double>(lines.size()) * layout.line_step;
        return layout;
    }

    // ---------------------------- Outline 相机 ----------------------------

    // 屏幕像素上的裁剪矩形（半开区间 [x0,x1) × [y0,y1)）。
    struct ClipRect {
        bool enabled = false;
        bool limit_x = false;
        bool limit_y = false;
        double x0 = 0.0;
        double y0 = 0.0;
        double x1 = 0.0;
        double y1 = 0.0;

        [[nodiscard]] bool contains(const int64_t x, const int64_t y) const {
            if (limit_x && (static_cast<double>(x) < x0 || static_cast<double>(x) >= x1))
                return false;
            if (limit_y && (static_cast<double>(y) < y0 || static_cast<double>(y) >= y1))
                return false;
            return true;
        }
    };

    // Liang-Barsky 线段裁剪：保留矩形内的部分（跨越边界的线段被截断，而不是整段丢弃）。
    bool clipSegment(const ClipRect& clip, int64_t& ax, int64_t& ay, int64_t& bx, int64_t& by) {
        if (!clip.enabled)
            return true;

        const double dx = static_cast<double>(bx - ax);
        const double dy = static_cast<double>(by - ay);

        double t0 = 0.0;
        double t1 = 1.0;

        const auto test = [&](const double p, const double q) {
            if (p == 0.0)
                return q >= 0.0; // 平行于边界：在内侧才保留
            const double t = q / p;
            if (p < 0.0) {
                if (t > t1) return false;
                if (t > t0) t0 = t;
            } else {
                if (t < t0) return false;
                if (t < t1) t1 = t;
            }
            return true;
        };

        if (clip.limit_x) {
            if (!test(-dx, static_cast<double>(ax) - clip.x0)) return false;
            if (!test(dx, clip.x1 - static_cast<double>(ax))) return false;
        }
        if (clip.limit_y) {
            if (!test(-dy, static_cast<double>(ay) - clip.y0)) return false;
            if (!test(dy, clip.y1 - static_cast<double>(ay))) return false;
        }

        const double x0 = static_cast<double>(ax);
        const double y0 = static_cast<double>(ay);

        ax = static_cast<int64_t>(std::llround(x0 + t0 * dx));
        ay = static_cast<int64_t>(std::llround(y0 + t0 * dy));
        bx = static_cast<int64_t>(std::llround(x0 + t1 * dx));
        by = static_cast<int64_t>(std::llround(y0 + t1 * dy));
        return true;
    }

    struct OutlineContext {
        RGBA8888* screen = nullptr;
        PoSize anchor{};
        double glyph_x = 0.0;
        double glyph_baseline = 0.0;
        const std::function<RGBA8888::Pixel(const xII::Item::Renderer::Args::DrawContext&)>* get_color = nullptr;
        size_t width = 1;
        PoSize item_pos{};
        PoSize item_size{};
        std::vector<uint8_t>* coverage = nullptr;
        ClipRect clip{};

        PoSize last{};
        bool has_last = false;
    };

    PoSize outlinePointToScreen(const OutlineContext& ctx, const FT_Vector& v) {
        return {ctx.anchor.x + static_cast<int64_t>(std::llround(ctx.glyph_x + from26_6(v.x))),
                ctx.anchor.y + static_cast<int64_t>(std::llround(ctx.glyph_baseline - from26_6(v.y)))};
    }

    void drawOutlineSegment(OutlineContext& ctx, const PoSize& a, const PoSize& b) {
        int64_t ax = a.x, ay = a.y, bx = b.x, by = b.y;
        if (!clipSegment(ctx.clip, ax, ay, bx, by))
            return;
        xII::Item::Renderer::Line::draw_line(*ctx.screen, {ax, ay}, {bx, by}, *ctx.get_color,
                                             ctx.width, ctx.item_pos, ctx.item_size, ctx.coverage);
    }

    int segmentsFor(const PoSize& a, const PoSize& b, const PoSize& c) {
        const double len = std::hypot(static_cast<double>(a.x - b.x), static_cast<double>(a.y - b.y)) +
                           std::hypot(static_cast<double>(b.x - c.x), static_cast<double>(b.y - c.y));
        return std::clamp(static_cast<int>(len / 3.0), 4, 24);
    }

    int segmentsFor(const PoSize& a, const PoSize& b, const PoSize& c, const PoSize& d) {
        const double len = std::hypot(static_cast<double>(a.x - b.x), static_cast<double>(a.y - b.y)) +
                           std::hypot(static_cast<double>(b.x - c.x), static_cast<double>(b.y - c.y)) +
                           std::hypot(static_cast<double>(c.x - d.x), static_cast<double>(c.y - d.y));
        return std::clamp(static_cast<int>(len / 3.0), 6, 32);
    }

    int outlineMoveTo(const FT_Vector* to, void* user) {
        auto& ctx = *static_cast<OutlineContext*>(user);
        ctx.last = outlinePointToScreen(ctx, *to);
        ctx.has_last = true;
        return 0;
    }

    int outlineLineTo(const FT_Vector* to, void* user) {
        auto& ctx = *static_cast<OutlineContext*>(user);
        if (!ctx.has_last)
            return outlineMoveTo(to, user);

        const PoSize point = outlinePointToScreen(ctx, *to);
        drawOutlineSegment(ctx, ctx.last, point);
        ctx.last = point;
        return 0;
    }

    int outlineConicTo(const FT_Vector* control, const FT_Vector* to, void* user) {
        auto& ctx = *static_cast<OutlineContext*>(user);
        if (!ctx.has_last)
            return outlineMoveTo(to, user);

        const PoSize p0 = ctx.last;
        const PoSize p1 = outlinePointToScreen(ctx, *control);
        const PoSize p2 = outlinePointToScreen(ctx, *to);

        const int segments = segmentsFor(p0, p1, p2);
        for (int i = 1; i <= segments; ++i) {
            const double t = static_cast<double>(i) / segments;
            const double mt = 1.0 - t;
            const double x = mt * mt * p0.x + 2.0 * mt * t * p1.x + t * t * p2.x;
            const double y = mt * mt * p0.y + 2.0 * mt * t * p1.y + t * t * p2.y;
            const PoSize point{static_cast<int64_t>(std::llround(x)),
                               static_cast<int64_t>(std::llround(y))};
            drawOutlineSegment(ctx, ctx.last, point);
            ctx.last = point;
        }
        ctx.last = p2;
        return 0;
    }

    int outlineCubicTo(const FT_Vector* control1, const FT_Vector* control2, const FT_Vector* to, void* user) {
        auto& ctx = *static_cast<OutlineContext*>(user);
        if (!ctx.has_last)
            return outlineMoveTo(to, user);

        const PoSize p0 = ctx.last;
        const PoSize p1 = outlinePointToScreen(ctx, *control1);
        const PoSize p2 = outlinePointToScreen(ctx, *control2);
        const PoSize p3 = outlinePointToScreen(ctx, *to);

        const int segments = segmentsFor(p0, p1, p2, p3);
        for (int i = 1; i <= segments; ++i) {
            const double t = static_cast<double>(i) / segments;
            const double mt = 1.0 - t;
            const double x = mt * mt * mt * p0.x + 3.0 * mt * mt * t * p1.x +
                             3.0 * mt * t * t * p2.x + t * t * t * p3.x;
            const double y = mt * mt * mt * p0.y + 3.0 * mt * mt * t * p1.y +
                             3.0 * mt * t * t * p2.y + t * t * t * p3.y;
            const PoSize point{static_cast<int64_t>(std::llround(x)),
                               static_cast<int64_t>(std::llround(y))};
            drawOutlineSegment(ctx, ctx.last, point);
            ctx.last = point;
        }
        ctx.last = p3;
        return 0;
    }

    const FT_Outline_Funcs kOutlineFuncs{
        outlineMoveTo,
        outlineLineTo,
        outlineConicTo,
        outlineCubicTo,
        0, // shift
        0, // delta
    };

    // 相机把“场景单位”映射到“屏幕像素”时的缩放倍数。
    // 文字保持纵横比不变，所以统一使用纵向缩放（screen_h / range_h）。
    double cameraScale(xII::Camera::Base& camera) {
        const auto range = camera.getRange();
        const auto size = camera.getScreenR().getSize();
        const double height = static_cast<double>(range.getY());
        if (height <= 0.0 || size.y <= 0)
            return 1.0;
        return static_cast<double>(size.y) / height;
    }

    // 屏幕像素上的裁剪矩形（半开区间 [x0,x1) × [y0,y1)）。
    // 把 Text 的裁剪 Rectangle（场景单位）换算成屏幕像素矩形。
    // Rectangle 的 width/height 是文本块的总宽/总高，<= 0 表示该方向不限制。
    ClipRect clipFromShape(const xII::Item::Shape::Rectangle* shape, xII::Camera::Base& camera) {
        ClipRect clip;
        if (shape == nullptr || shape->getCenter() == nullptr)
            return clip;

        const double shape_w = static_cast<double>(shape->getWidth());
        const double shape_h = static_cast<double>(shape->getHeight());
        if (shape_w <= 0.0 && shape_h <= 0.0)
            return clip;

        const auto range = camera.getRange();
        const auto screen_size = camera.getScreenR().getSize();
        const auto map_point = camera.getPoint();

        const double range_x = static_cast<double>(range.getX());
        const double range_y = static_cast<double>(range.getY());
        if (range_x <= 0.0 || range_y <= 0.0)
            return clip;

        // 用浮点计算中心，避免 Point2PoSize 的整数截断带来 1 像素偏移。
        const double scale_x = static_cast<double>(screen_size.x) / range_x;
        const double scale_y = static_cast<double>(screen_size.y) / range_y;

        const double center_x = (static_cast<double>(shape->getCenter()->getX()) -
                                 static_cast<double>(map_point->getX())) * scale_x;
        const double center_y = (static_cast<double>(shape->getCenter()->getY()) -
                                 static_cast<double>(map_point->getY())) * scale_y;

        clip.enabled = true;
        clip.limit_x = shape_w > 0.0;
        clip.limit_y = shape_h > 0.0;

        const double half_w = shape_w * scale_x / 2.0;
        const double half_h = shape_h * scale_y / 2.0;

        clip.x0 = center_x - half_w;
        clip.x1 = center_x + half_w;
        clip.y0 = center_y - half_h;
        clip.y1 = center_y + half_h;
        return clip;
    }
} // namespace

namespace xII::Item {
    Text::Text(const Shape::Rectangle& shape,
               std::string text,
               std::string font_path,
               const double height,
               const Screen::Screen::Pixel& color)
        : _text(std::move(text)),
          _font_path(std::move(font_path)),
          _height(height > 0.0 ? height : 1.0),
          // Rectangle 的 center 是位置锚点，必须非空；宽高 <= 0 表示该方向不裁切。
          _shape(shape.getCenter() != nullptr
                     ? shape
                     : Shape::Rectangle(std::make_shared<Space::Point>(), shape.getWidth(), shape.getHeight())),
          _color(color) {
        _loadFace();
    }

    Text::Text(const std::shared_ptr<Space::Point>& position,
               std::string text,
               std::string font_path,
               const double height,
               const Screen::Screen::Pixel& color)
        : Text(Shape::Rectangle(position != nullptr ? position : std::make_shared<Space::Point>(), 0.0f, 0.0f),
               std::move(text), std::move(font_path), height, color) {}

    Text::~Text() {
        if (_face != nullptr) {
            FT_Done_Face(_face);
            _face = nullptr;
        }
    }

    void Text::_loadFace() {
        if (_face != nullptr) {
            FT_Done_Face(_face);
            _face = nullptr;
        }
        _error.clear();
        _applied_char_size = -1; // 新 face 需要重新设置字号

        const FT_Library library = getFreeTypeLibrary();
        if (library == nullptr) {
            _error = "FT_Init_FreeType failed";
            return;
        }

        std::vector<std::string> candidates;
        if (!_font_path.empty()) {
            candidates.push_back(_font_path);
        } else {
            for (const char* path : kFallbackFonts)
                candidates.emplace_back(path);
        }

        for (const auto& path : candidates) {
            FT_Face face = nullptr;
            const FT_Error error = FT_New_Face(library, path.c_str(), 0, &face);
            if (error == 0 && face != nullptr) {
                _face = face;
                _font_path = path;
                return;
            }

            if (_error.empty()) {
                _error = "FT_New_Face failed (" + std::to_string(error) + "): " + path;
            }
        }
    }

    bool Text::_applySize(const double em) const {
        if (_face == nullptr)
            return false;

        // FT_Set_Char_Size(..., 72, 72)：字符尺寸以 26.6 点表示，72dpi 下 1 点 = 1 像素，
        // 因此可以设置 1/64 像素精度的字号（即支持小数）。
        const auto char_size = static_cast<FT_F26Dot6>(
            std::llround(std::max(em, 1.0 / 64.0) * 64.0));
        if (char_size == _applied_char_size)
            return true;

        if (FT_Set_Char_Size(_face, 0, char_size, 72, 72) == 0) {
            _applied_char_size = char_size;
            return true;
        }

        // 非可缩放字体（纯位图字体）退回整数像素尺寸。
        const auto pixels = static_cast<FT_UInt>(std::max<int64_t>(1, std::llround(em)));
        if (FT_Set_Pixel_Sizes(_face, 0, pixels) != 0)
            return false;
        _applied_char_size = char_size;
        return true;
    }

    void Text::setFontPath(std::string font_path) {
        _font_path = std::move(font_path);
        _applied_char_size = -1;
        _loadFace();
    }

    void Text::setHeight(const double height) {
        _height = (height > 0.0) ? height : 1.0;
        _applied_char_size = -1; // 字号变了，重新设置
    }

    void Text::setPosition(const std::shared_ptr<Space::Point>& position) {
        _shape.setCenter(position != nullptr ? position : std::make_shared<Space::Point>());
    }

    void Text::setShape(const Shape::Rectangle& shape) {
        _shape = (shape.getCenter() != nullptr)
                     ? shape
                     : Shape::Rectangle(std::make_shared<Space::Point>(), shape.getWidth(), shape.getHeight());
    }

    void Text::clearShape() {
        _shape.setWidth(0.0f);
        _shape.setHeight(0.0f);
    }

    Text::Measure Text::measure() const {
        if (_face == nullptr || !_applySize(_height))
            return {};

        LayoutOptions options;
        options.align = _align;
        options.line_spacing = _line_spacing;
        options.line_width = _line_width;

        const TextLayout layout = buildLayout(_face, _text, options);
        return {layout.width, layout.height};
    }

    void Text::Render(Camera::Standard& camera, Item::Renderer::Args args) const {
        if (_face != nullptr) {
            const double scale = cameraScale(camera);
            const double em = std::max(_height * scale, 1.0 / 64.0);
            if (_applySize(em))
                _renderStandard(camera, args, em / _height);
        }

        for (const auto& subitem : _subitems)
            if (subitem->isInRange(camera))
                subitem->Render(camera, args);
    }

    void Text::_renderStandard(Camera::Standard& camera, Item::Renderer::Args& args, const double ratio) const {
        LayoutOptions options;
        options.align = _align;
        options.line_spacing = _line_spacing * ratio;
        options.line_width = _line_width * ratio;
        // shape.width 是场景单位的总宽，ratio 换算成屏幕像素（= 排版单位）。
        options.box_width = (_shape.getWidth() > 0.0f) ? static_cast<double>(_shape.getWidth()) * ratio : 0.0;

        const TextLayout layout = buildLayout(_face, _text, options);
        if (layout.glyphs.empty())
            return;

        auto& screen = camera.getScreenR();
        const int64_t width = screen.getSize().x;
        const int64_t height = screen.getSize().y;
        if (width <= 0 || height <= 0)
            return;

        Screen::Screen::Pixel* data = screen.getData();

        // Rectangle.center 是文本块的中心：
        // 宽高 > 0 用手指定的值，否则按内容自适应。
        const double box_w = (layout.box_width > 0.0) ? layout.box_width : layout.width;
        const double box_h = (_shape.getHeight() > 0.0f) ? static_cast<double>(_shape.getHeight()) * ratio
                                                         : layout.height;

        const Screen::PoSize center = Screen::Point2PoSize(
            camera.getPoint(), camera.getRange(), {0, 0}, screen.getSize(), _shape.getCenter());
        const Screen::PoSize anchor{center.x - static_cast<int64_t>(std::llround(box_w / 2.0)),
                                    center.y - static_cast<int64_t>(std::llround(box_h / 2.0))};

        // Text 自己保存颜色；args 的 alpha 再乘一次，这样 Fadein/Fadeout/SetAlpha
        // 作为 General 的 renderer 时也能影响文字。
        const uint32_t base_alpha = static_cast<uint32_t>(std::llround(
            static_cast<double>(_color.A) * static_cast<double>(args.getAlpha()) / 255.0));
        if (base_alpha == 0)
            return;

        // 裁剪框：超出 Rectangle（总宽/总高）的像素不绘制，
        // 跨在边界上的字形只绘制框内的可见部分。
        const ClipRect clip = clipFromShape(&_shape, camera);

        for (const auto& glyph : layout.glyphs) {
            if (FT_Load_Char(_face, glyph.codepoint, FT_LOAD_RENDER) != 0)
                continue;

            const FT_GlyphSlot slot = _face->glyph;
            const FT_Bitmap& bitmap = slot->bitmap;
            if (bitmap.buffer == nullptr || bitmap.width == 0 || bitmap.rows == 0)
                continue;

            const int64_t origin_x = anchor.x + static_cast<int64_t>(std::llround(glyph.x)) + slot->bitmap_left;
            const int64_t origin_y = anchor.y + static_cast<int64_t>(std::llround(glyph.baseline)) - slot->bitmap_top;

            for (unsigned int by = 0; by < bitmap.rows; ++by) {
                const int64_t py = origin_y + static_cast<int64_t>(by);
                if (py < 0 || py >= height)
                    continue;

                const unsigned char* row = nullptr;
                if (bitmap.pitch >= 0)
                    row = bitmap.buffer + static_cast<size_t>(by) * static_cast<size_t>(bitmap.pitch);
                else
                    row = bitmap.buffer + static_cast<size_t>(bitmap.rows - 1 - by) * static_cast<size_t>(-bitmap.pitch);

                for (unsigned int bx = 0; bx < bitmap.width; ++bx) {
                    const int64_t px = origin_x + static_cast<int64_t>(bx);
                    if (px < 0 || px >= width)
                        continue;
                    if (!clip.contains(px, py))
                        continue;

                    uint32_t coverage = 0;
                    switch (bitmap.pixel_mode) {
                        case FT_PIXEL_MODE_GRAY:
                            coverage = row[bx];
                            break;
                        case FT_PIXEL_MODE_MONO:
                            coverage = (row[bx >> 3] & (0x80u >> (bx & 7u))) != 0 ? 255u : 0u;
                            break;
                        default:
                            break;
                    }
                    if (coverage == 0)
                        continue;

                    const uint32_t alpha = base_alpha * coverage / 255u;
                    if (alpha == 0)
                        continue;

                    const Screen::Screen::Pixel color{
                        static_cast<uint8_t>(alpha), _color.R, _color.G, _color.B};

                    auto& destination = data[static_cast<size_t>(py) * static_cast<size_t>(width) +
                                             static_cast<size_t>(px)];
                    if (alpha == 255)
                        destination = color;
                    else
                        destination = Screen::RGBA8888::blend_alpha_pixel(color, destination);
                }
            }
        }
    }

    void Text::Render(Camera::Outline& camera) const {
        if (_face != nullptr) {
            const double scale = cameraScale(camera);
            const double em = std::max(_height * scale, 1.0 / 64.0);
            if (_applySize(em))
                _renderOutline(camera, em / _height);
        }

        for (const auto& subitem : _subitems)
            if (subitem->isInRange(camera))
                subitem->Render(camera);
    }

    void Text::_renderOutline(Camera::Outline& camera, const double ratio) const {
        LayoutOptions options;
        options.align = _align;
        options.line_spacing = _line_spacing * ratio;
        options.line_width = _line_width * ratio;
        options.box_width = (_shape.getWidth() > 0.0f) ? static_cast<double>(_shape.getWidth()) * ratio : 0.0;

        const TextLayout layout = buildLayout(_face, _text, options);
        if (layout.glyphs.empty())
            return;

        auto& screen = camera.getScreenR();
        const int64_t width = screen.getSize().x;
        const int64_t height = screen.getSize().y;
        if (width <= 0 || height <= 0)
            return;

        const double box_w = (layout.box_width > 0.0) ? layout.box_width : layout.width;
        const double box_h = (_shape.getHeight() > 0.0f) ? static_cast<double>(_shape.getHeight()) * ratio
                                                         : layout.height;

        const Screen::PoSize center = Screen::Point2PoSize(
            camera.getPoint(), camera.getRange(), {0, 0}, screen.getSize(), _shape.getCenter());
        const Screen::PoSize anchor{center.x - static_cast<int64_t>(std::llround(box_w / 2.0)),
                                    center.y - static_cast<int64_t>(std::llround(box_h / 2.0))};
        const Screen::PoSize item_pos = anchor;
        const Screen::PoSize item_size{static_cast<int64_t>(std::llround(box_w)),
                                        static_cast<int64_t>(std::llround(box_h))};

        // 轮廓线互相重叠时，没有 coverage 掩码会让 alpha 反复叠加。
        std::vector<uint8_t> coverage;
        std::vector<uint8_t>* coverage_ptr = nullptr;
        const auto probe_color = camera.getColor(
            Item::Renderer::Args::DrawContext{{0, 0}, screen.getSize(), item_pos, item_size});
        if (probe_color.A < 255) {
            coverage.assign(static_cast<size_t>(width) * static_cast<size_t>(height), 0);
            coverage_ptr = &coverage;
        }

        OutlineContext context;
        context.screen = &screen;
        context.anchor = anchor;
        context.get_color = &camera.getColor;
        context.width = camera.getWidth();
        context.item_pos = item_pos;
        context.item_size = item_size;
        context.coverage = coverage_ptr;
        context.clip = clipFromShape(&_shape, camera);

        for (const auto& glyph : layout.glyphs) {
            // 只要矢量轮廓，不要位图；不做 hinting，曲线更平滑。
            if (FT_Load_Char(_face, glyph.codepoint, FT_LOAD_NO_BITMAP | FT_LOAD_NO_HINTING) != 0)
                continue;

            context.glyph_x = glyph.x;
            context.glyph_baseline = glyph.baseline;
            context.has_last = false;

            FT_Outline_Decompose(&_face->glyph->outline, &kOutlineFuncs, &context);
        }
    }
} // namespace xII::Item
