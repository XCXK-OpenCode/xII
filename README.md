**xII**是一个由`C++`编写的项目(不知道具体啥类型)  
## 食用
本项目采用 GNU General Public License v3.0（GPLv3）许可。
使用、复制、修改或分发本项目前，请阅读并遵守根目录下的 LICENSE 文件。
根据 GPLv3 第9条，接收或运行本程序副本无需接受本许可证；但修改或分发本作品即表示你接受 GPLv3 的条款。
本项目不提供任何担保，详见许可证第15、16条。

项目源码结构:
- src/ 项目源码文件夹
- test/ 项目测试文件夹
- examples/ 项目示例文件夹
- CMakeLists.txt  

### 测试
examples是示例文件夹，里面有一些示例，你可以使用它进行测试
```bash
    cmake -S . -B cmake-build  # 创建生成目录
    cmake --build cmake-build -j  # 编译项目
    cmake-build/examples/line_renderer/example_line_renderer  # 执行测试
```
运行`line_renderer`示例你将看到一个名为`Line Renderer Example`的窗口  
其中绘制了一个白色边框的长方形

### 渲染后端（CPU / OpenGL）
默认只编译 CPU 光栅化后端。要额外编译 OpenGL 后端，在配置时打开开关：

```bash
    cmake -S . -B cmake-build -DXII_ENABLE_OPENGL=ON
    cmake --build cmake-build -j

    cmake-build/examples/gl_renderer/example_gl_renderer        # CPU
    cmake-build/examples/gl_renderer/example_gl_renderer --gl   # GPU
```

相关宏定义在 `src/Screen/Backend.h`：

| 宏 | 含义 |
| --- | --- |
| `XII_ENABLE_OPENGL` | CMake 传入的开关（`ON` → 1） |
| `XII_BACKEND_CPU` | 恒为 1，CPU 后端永远可用，同时是 GPU 的像素校验基准 |
| `XII_BACKEND_GL` | 等于 `XII_ENABLE_OPENGL`，为 1 时才编译 `GLCanvas` |

宏只决定“能不能用”，运行时用 `camera.setCanvas()` 选择：

```cpp
auto canvas = xII::Screen::GLCanvas::create(window, {W, H});  // 需要 GL 上下文
camera.setCanvas(canvas.get());   // 走 GPU
camera.setCanvas(nullptr);        // 回到 CPU
```

同一个场景下 GPU 与 CPU 的输出在测试中逐像素一致（`test/20261018-173742 --gl` 会打印对比结果）。

