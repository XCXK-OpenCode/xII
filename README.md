**xII**是一个由`C++`编写的项目(不知道具体啥类型)  
## 食用
在使用以前请遵循`LICENSE`协议,一旦使用该项目,则视为你已经同意了协议  
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
