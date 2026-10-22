// 
// Created by qpwser from XCXK on 2026-10-20
// Powered by DeepSeek V4.1 Flash
//

#pragma once

#include "Base.h"

#include "Shape/Rectangle.h"
#include "Screen/PoSize.h"
#include "Space/Point.h"

#include <cstdint>
#include <memory>
#include <string>
#include <utility>

// 只前向声明 FreeType 的 face 类型，避免所有包含 Text.h 的编译单元都被迫引入 ft2build.h。
typedef struct FT_FaceRec_* FT_Face;

namespace xII::Item {
    // 基于 FreeType 的文字 Item。
    //
    // 坐标约定：字体高度、行间距、行宽全部使用“场景单位”（相机变换之前的坐标，
    // 与 Space::Point / Item::Shape::Rectangle 同一坐标系）。渲染时再按相机的缩放
    // （screen_size / range）换算成屏幕像素，所以文字会随相机一起缩放。
    //
    // - 位置与裁剪：_shape 就是文本块的 Item::Shape::Rectangle（场景单位）。
    //         _shape.getCenter()       = 位置锚点（Align::Left=左边缘，Center=中心，Right=右边缘）
    //         _shape.getWidth()        = 文本块总宽（裁剪框宽）
    //         _shape.getHeight()       = 文本块总高（裁剪框高）
    //         width/height <= 0 视为该方向不限制；两者都 <= 0 就是不裁剪。
    //         超出框外的像素不绘制；恰好跨在边界上的字形只绘制框内可见部分。
    //         width/height 与字体高度、行间距无关：字体高度由 _height 单独决定。
    // - 颜色：Text 自己持有颜色；Renderer::Args 的 alpha 会乘到颜色上，
    //         因此放在 General 里配合 Fadein/Fadeout/SetAlpha 仍然可以淡入淡出。
    class Text : public Base {
    public:
        enum class Align : uint8_t {
            Left,
            Center,
            Right,
        };

        // 文本块尺寸（场景单位，已包含换行结果）。
        struct Measure {
            double width = 0.0;
            double height = 0.0;
        };

    private:
        std::string _text;
        std::string _font_path;
        double _height;
        Shape::Rectangle _shape;
        Screen::Screen::Pixel _color;
        Align _align = Align::Left;
        double _line_spacing = 0.0;
        double _line_width = 0.0;

        FT_Face _face = nullptr;
        std::string _error;
        mutable int64_t _applied_char_size = -1; // 已应用的 26.6 字号，避免每帧重复设置

        // measure() 的缓存：isInRange() 和 Render() 每帧都会用到文本块尺寸，
        // 排版（逐字 FT_Load_Glyph）不应每帧做两遍。
        mutable Measure _measure_cache{};
        mutable bool _measure_cache_valid = false;
        void _invalidateMeasure() const { _measure_cache_valid = false; }

        void _loadFace();
        [[nodiscard]] bool _applySize(double em) const;
        void _renderStandard(Camera::Standard& camera, Item::Renderer::Args& args, double ratio) const;
        void _renderOutline(Camera::Outline& camera, double ratio) const;

    public:
        // 主构造：位置与裁剪框都由同一个 Rectangle 描述。
        // shape.getCenter() 是位置锚点，shape.getWidth()/getHeight() 是文本块总宽/总高
        // （<= 0 表示该方向不限制），均为场景单位。
        // font_path 为空时会尝试若干系统字体；加载失败不会抛异常，
        // 可以通过 isValid()/getError() 检查，Render() 在失败时是空操作。
        // height 是场景单位下的字体高度（EM），支持小数，与 shape 的宽高无关。
        Text(const Shape::Rectangle& shape,
             std::string text,
             std::string font_path = "",
             double height = 32.0,
             const Screen::Screen::Pixel& color = {255, 255, 255, 255});

        // 便利构造：只给位置，宽高按 0（不裁剪）处理。
        Text(const std::shared_ptr<Space::Point>& position,
             std::string text,
             std::string font_path = "",
             double height = 32.0,
             const Screen::Screen::Pixel& color = {255, 255, 255, 255});
        ~Text() override;

        Text(const Text&) = delete;
        Text& operator=(const Text&) = delete;

        [[nodiscard]] bool isValid() const { return _face != nullptr; }
        [[nodiscard]] const std::string& getError() const { return _error; }

        [[nodiscard]] const std::string& getText() const { return _text; }
        void setText(std::string text) {
            _text = std::move(text);
            _invalidateMeasure();
        }

        [[nodiscard]] const std::string& getFontPath() const { return _font_path; }
        void setFontPath(std::string font_path);

        // 字体高度（场景单位，支持小数）
        [[nodiscard]] double getHeight() const { return _height; }
        void setHeight(double height);

        // 位置就是 shape 的 center，这里只是代理。
        [[nodiscard]] std::shared_ptr<Space::Point> getPosition() const { return _shape.getCenter(); }
        void setPosition(const std::shared_ptr<Space::Point>& position);

        [[nodiscard]] Screen::Screen::Pixel getColor() const { return _color; }
        void setColor(const Screen::Screen::Pixel& color) { _color = color; }

        [[nodiscard]] Align getAlign() const { return _align; }
        void setAlign(Align align) {
            _align = align;
            _invalidateMeasure();
        }

        // 行间距（场景单位）：字体自然行高之外额外增加的间距，可为负。
        [[nodiscard]] double getLineSpacing() const { return _line_spacing; }
        void setLineSpacing(double line_spacing) {
            _line_spacing = line_spacing;
            _invalidateMeasure();
        }

        // 行宽（场景单位）：每行最大宽度，<= 0 表示不自动换行。
        [[nodiscard]] double getLineWidth() const { return _line_width; }
        void setLineWidth(double line_width) {
            _line_width = (line_width > 0.0) ? line_width : 0.0;
            _invalidateMeasure();
        }

        // 文本块矩形（场景单位）：center=位置锚点，width/height=总宽/总高。
        // 直接修改返回值的拷贝不会影响 Text，请用 setShape。
        [[nodiscard]] const Shape::Rectangle& getShape() const { return _shape; }
        void setShape(const Shape::Rectangle& shape);
        // 清除裁剪：宽高置 0，位置保持不变。
        void clearShape();

        // 文本块尺寸（场景单位）。只依赖字体/字号/内容/行间距/行宽，与相机无关。
        [[nodiscard]] Measure measure() const;

        void Render(Camera::Standard& camera, Item::Renderer::Args args) const override;
        void Render(Camera::Outline& camera) const override;

        // 文本块（_shape 的 center/width/height）与相机视口（Camera 的 Rectangle）
        // 的轴对齐包围盒是否相交。false 时父级（Item::Base / General）会直接跳过
        // 本 Item 的 Render，不会产生任何像素。
        // 只做“整块完全离开视口”的剔除：跨在边界上的块返回 true（继续画，
        // 具体可见部分由 Rectangle 裁剪负责）。
        [[nodiscard]] bool isInRange(const Camera::Base& camera) const override;
    };
} // namespace xII::Item