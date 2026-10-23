# xII::Space::Offset
> [!WARNING]
> 该class非正式，后续可能进行变更，请做好更新准备

定义于`src/Space/Offset.h`  
是`xII::Space::OffsetRoot`的一个实例化  
```C++
    using Offset = OffsetRoot<Base::Quantity::Base_Value_Type>;
```
用于存储偏移量，比如在`OffsetPoint`中有使用
## 使用
### 构建
该函数一般直接构建，如
```C++
    // 构造一个xII::Space::Offset，值为(0, 0)
    xII::Space::Offset offset(1, 1);
```
### 值操作
```C++
    xII::Space::Offset offset(1, 1);
    auto x = offset.getX(); // 获取X
    auto y = offset.getY(); // 获取Y
    offset.setX(x + y); // 设置X
    offset.setY(x + y); // 设置Y
```

# xII::Space::OffsetRoot
> [!WARNING]
> 该class非正式，后续可能进行变更，请做好更新准备

该class定义于`src/Space/Offset.h`  
具有模板参数`<typename Value_Type>`，用于设置`_x`和`_y`的类型
## 元素
### _x *(private)*
储存x轴的偏移值  
该元素类型为`Value_Type`  
### _y *(private)*
储存y轴的偏移值  
该元素类型为`Value_Type`  
### 构造 析构
构造要求传入`x`和`y`，用以初始化`_x`和`_y`  
```C++
OffsetRoot(const Value_Type &x, const Value_Type &y);
```
### getX()
该函数用于获取x轴坐标，即返回`_x`的值  
```C++
Value_Type getX() const;
```  
### getY()
该函数用于获取y轴坐标，即返回`_y`的值  
```C++
Value_Type getY() const;
```  
### setX(const Value_Type &x) 
该函数用于设置x的坐标，即设置`_x`的值  
```C++
void setX(const Value_Type &x);
```  
### setY(const Value_Type &y)
该函数用于设置y轴坐标，即设置`_y`的值  
```C++
void setY(const Value_Type &y);
````
