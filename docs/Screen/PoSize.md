# xII::Screen::PoSize
该结构体定义于`src/Screen/PointPos.h`  
用于存储画布中的位置或者大小  
含有两个元素，分别储存`x`和`y`的信息
```C++
struct PoSize {
    int64_t x, y;
};
```
## 相关运算
### operator+(const PoSize &l, const PoSize &r)
将两个`PoSize`相加
```C++
inline PoSize operator+(const PoSize &l, const PoSize &r);
```
### operator-(const PoSize &l, const PoSize &r) 
将两个`PoSize`做差
```C++
inline PoSize operator-(const PoSize &l, const PoSize &r);
```