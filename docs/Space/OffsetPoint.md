# xII::Space:OffsetPoint
> [!WARNING]
> 该class非正式，后续可能进行变更，请做好更新准备

定义于`src/Space/OffsetPoint.h`  
是`xII::Space::OffsetPointRoot`的一个实例化
```C++
    using OffsetPoint = OffsetPointRoot<Offset>;
```
用作含有偏移的点
## 使用
### 构建
应使用`xII::Space::Point`类似的方法进行构建
```C++
    auto root = std::make_shared<xII::Space::Point>(); // 构建原点
    auto point = std::make_shared<xII::Space::OffsetPoint>(root, xII::Space::Offset(127, 63)); // 构建 OffsetPoint
```
### 值操作
```C++
    auto root = std::make_shared<xII::Space::Point>();
    auto point = std::make_shared<xII::Space::OffsetPoint>(root, xII::Space::Offset(127, 63));

    auto x = point->getX(); // 获取X值
    auto y = point->getY(); // 获取Y值
    point->getOffsetRef().setX(x + y); // 设置X值
    point->getOffsetRef().setY(x + y); // 设置Y值

    auto &offset_ref = point->getOffsetRef(); // 获取_offset的引用
    auto offset = point->getOffset(); // 获取_offset的值

```

# xII::Space::OffsetPointRoot
> [!WARNING]
> 该class非正式，后续可能进行变更，请做好更新准备

该class定义于`src/Space/OffsetPoint.h`  
有模板参数`<typename Offset_Type>`，用于设置`_offset`的类型  
派生自`xII::Space::Point`
## 元素
### _origin *(private)*
该元素为点的起点（原点）  
类型为`std::shared_ptr<xII::Space::Point>`
### _offset *(private)*
该元素为点的偏移量，由原点进行偏移  
类型为`Offset_Type`
### 构造 析构
该class提供了一个构造方法 
```C++
OffsetPointRoot(const std::shared_ptr<Point>& origin, const Offset_Type& offset)
```  
需要传入两个参数  
- `origin`为原点，用于初始化`_origin`  
- `offset`为偏移值，用于初始化`_offset`  
### getX()
该函数用于获取X的偏移量  
```C++
Base::Quantity::Base_Value_Type getX() const override;
```
在该类中将获取`_origin->getX()`与`_offset.getX()`的和  
### getY()
该函数用于获取Y的偏移量
```C++
Base::Quantity::Base_Value_Type getY() const override;
```
在该类中将获取`_origin->getY()`与`_offset.getY()`的和  
### getRoot()
该函数用于获取根点
```C++
std::shared_ptr<Point> getRoot() override;
```
在该类中将返回`_origin->getRoot()`
> 这里面会层层拨开，直到回到类型为`xII::Space::Point`的时会返回`shared_from_this()`
### getOrigin()
该函数用于获取`_origin`
```C++
std::shared_ptr<Point> getOrigin() const;
```
### setOrigin(const std::shared_ptr\<Point>& origin)
该函数用于设置`_origin`
```C++
void setOrigin(const std::shared_ptr<Point>& origin);
```
### getOffset()
该函数用于获取`_offset`
```C++
Offset_Type getOffset() const;
```
### getOffsetRef()
该函数用于获取`_offset`的引用
```C++
Offset_Type &getOffsetRef();
const Offset_Type &getOffsetRef() const; // Read-only
```
### setOffset(const Offset_Type& offset)
该函数用于设置`_offset`的值
```C++
void setOffset(const Offset_Type& offset);
```
### operator==(const OffsetPointRoot& v)
该运算进行比较相等运算  
满足`_offset`和`_origin`相等返回true
```C++
bool operator==(const OffsetPointRoot& v) const;
```
### operator!=(const OffsetPointRoot& v)
该运算进行不等运算  
满足`_offset`和`_origin`任意一个不相等则返回true
```C++
bool operator!=(const OffsetPointRoot& v) const;
```