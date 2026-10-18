**xII**是一个由`C++`编写的项目(不知道具体啥类型)  
## 食用
在使用以前请遵循`LICENSE`协议,一旦使用该项目,则视为你已经同意了协议  
项目源码结构:
- src/ 项目源码文件夹
- test/ 项目测试示例文件夹
- CMakeLists.txt  

test作为测试示例文件夹, 你可以执行里面的示例帮助你快速学习
示例
```bash
    cmake -S . -B cmake-build  # 创建生成目录
    cmake --build cmake-build -j  # 编译项目
    cmake-build/test/20261011-2209/test  # 执行测试
```
