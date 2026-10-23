# xII::Space::Point
> [!WARNING]
> 该class非正式，后续可能进行变更，请做好更新准备

该class定义于`src/Space/Point.h`
派生自`std::enable_shared_from_this<Point>`  
## 元素
### 构造 析构
该class中构造无需参数
### getX()
该虚函数用于获取X轴坐标  
```C++
Base::Quantity::Base_Value_Type getX() const;
```  
在`xII::Space::Point`中其返回`0`  
### getY()
该虚函数用于获取Y轴坐标  
```C++
Base::Quantity::Base_Value_Type getY() const;
```  
在`xII::Space::Point`中其返回`0`  
### getRoot()
该虚函数用于获取root的共享指针
```C++
std::shared_ptr<Point> getRoot();
```  
在`xII::Space::Point`中其返回`shared_from_this()`  
## 使用
### 构建
一般地，我们应使用`std::shared_ptr<xII::Space::Point>`来安全的储存它  
推荐使用`std::make_shared<xII::Space::Point>`来构建它  
```C++
    // 构建一个 xII::Space::Point
    auto point = std::make_shared<xII::Space::Point>();
```
### 获取
```C++
    auto point = std::make_shared<xII::Space::Point>();
    auto x = point->getX(); // 获取 X
    auto y = point->getY(); // 获取 Y
    auto root = point->getRoot();   // 获取 Root
```