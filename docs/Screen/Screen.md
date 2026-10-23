# xII::Screen::Screen
这是`xII::Screen::RGBA8888`的一个别名  
定义于`src/Screen/Screen.h`  
```C++
using Screen = RGBA8888;
```
> [!WARNING]
> 这偏向于一个临时的方法，后续需要进行更改
# xII::Screen::ScreenRef
这是`std::shared_ptr<Screen>`的一个别名  
定义于`src/Screen/Screen.h`  
```C++
using ScreenRef = std::shared_ptr<Screen>;
```
## 相关运算
### operator<<(const ScreenRef& dest, const ScreenRef& src)
把右侧整块画面深拷贝进左侧缓冲区
```C++
inline ScreenRef operator<<(const ScreenRef& dest, const ScreenRef& src);
```