# xII::Item::Image
该`Item`用于渲染图像  
其内有一个`Shape::Rectangle`作为其目标框  
一个`std::shared_ptr<Screen::Screen>`作为渲染来源  
渲染会进行缩放
## 元素
### _screen *(private)*
这是其图像来源
```C++
std::shared_ptr<Screen::Screen> _screen;
```
### _shape *(private)*
这是渲染目标位置
```C++
Shape::Rectangle _shape;
```
### 构建 析构
#### 左值复制
```C++
Image(const Image&) = default;
```
> [!WARNING]
> 这将造成内存复制，使用前请确定你要这么做
#### 右值转移
```C++
Image(Image&&) = default;
```
#### 由已有的shared screen构建
```C++
Image(const std::shared_ptr<Screen::Screen> &screen, const Shape::Rectangle& shape);
```
#### 由已有的screen复制
```C++
Image(const Screen::Screen &screen, const Shape::Rectangle& shape);
```
> [!WARNING]
> 这将造成内存复制，使用前请确定你要这么做
#### 由已有的screen转移
```C++
Image(Screen::Screen &&screen, const Shape::Rectangle& shape);
```
#### 由size创建一个新画布
```C++
Image(const Screen::PoSize &size, const Shape::Rectangle& shape);
```
## 使用
### 示例
```C++
    /* Create Subcamera */
    /* 更完整的camera初始化你可以在docs/Camera/Standard.md中查看 */
    auto sub_camera = Camera::Standard(
        Item::Shape::Rectangle(camera_center, CAMERA_SHAPE_WIDTH, CAMERA_SHAPE_HEIGHT),  // Camera shape
        {SCREEN_WIDTH, SCREEN_HEIGHT},      // Screen size
        std::make_shared<Item::Base>(),     // Root object
        camera_time_point                   // Time point
    );

    auto image_item = std::make_shared<xII::Item::Image>(
        sub_camera.getScreen(),
        Item::Shape::Rectangle(image_center, IMAGE_SHAPE_WIDTH, IMAGE_SHAPE_HEIGHT),  // shape
    )
```