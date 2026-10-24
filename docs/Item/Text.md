# xII::Item::Text
文本类型的`Item`  
定义于`src/Item/Text.h`  
派生自`xII::Item::Base`
## 元素
### 私有元素没啥好了解的, 此处先不做介绍
### 构建 析构
```C++
Text(const Shape::Rectangle& shape,
             std::string text,
             std::string font_path = "",
             double height = 32.0,
             const Screen::Screen::Pixel& color = {255, 255, 255, 255});
Text(const std::shared_ptr<Space::Point>& position,
             std::string text,
             std::string font_path = "",
             double height = 32.0,
             const Screen::Screen::Pixel& color = {255, 255, 255, 255});
```
## 使用
### 示例
```C++
    /* Created camera */
    // 该程序未验证

    camera.addItem(std::make_shared<xII::Item::Text>(
        text_shape,
        "Hello World",
        "",
        100,
        {255, 255, 255, 255}
    ));
```