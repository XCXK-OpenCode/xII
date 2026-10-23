> 本项目采用 GNU General Public License v3.0（GPLv3）许可。  
> 使用、复制、修改或分发本项目前，请阅读并遵守根目录下的 LICENSE 文件。  
> 根据 GPLv3 第9条，接收或运行本程序副本无需接受本许可证；但分发本作品或基于本作品的修改版本时，即表示你接受 GPLv3 的条款。  
> 本项目不提供任何担保，详见许可证第15、16条。  

**xII**是一个由`C++`编写的项目（偏向于渲染类型）
## 食用
**xII**现在还没有教程，并不适合学习  
项目源码结构:
- docs/ 文档文件夹
- src/ 源码文件夹
- libs/ 依赖库文件夹
- test/ 测试文件夹
- examples/ 示例文件夹
- CMakeLists.txt  

### Line Renderer Example (测试)
`examples`是示例文件夹，里面有一些示例，你可以使用它进行测试
```bash
    # 请在执行权
    # - 安装依赖(如git cmake gcc(或者其他编译器))
    # - 克隆和进入项目
    # - 下载模块

    cmake -S . -B cmake-build  # 创建生成目录
    cmake --build cmake-build -j  # 编译项目
    cmake-build/examples/line_renderer/example_line_renderer  # 执行测试
```
运行`line_renderer`示例你将看到一个名为`Line Renderer Example`的窗口  
其中绘制了一个白色边框的长方形
### examples
在`examples`文件夹中提供了丰富的示例程序，例如`examples/rainbow_renderer`这些  
可以试着使用`examples`的示例程序去学习
### test
在`test`文件夹中有一些调试用的程序，可以试着去运行  
`test`的代码质量很差，一般还是得看`examples`
### docs
`docs`文件夹是数据手册文件夹  
里面主要是那些接口的定义等
