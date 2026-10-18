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
