# xII::Item
`Item`是**xII**中的重要角色，我们所渲染的目标也是`Item`  
# xII::Item::Base
该class定义于`src/Item/Base.h`  
是所有`Items`的基类  
## 元素
### _subitems *(private)*
`Item`中是可以添加子元素的，所以有`_subitems`的存在  
```C++
std::vector<std::shared_ptr<Base>> _subitems;
```
`vector`里面存储目标`Item`的`shared_ptr`
### 构建 析构
`Base`构建析构不需要参数
### addItem(std::shared_ptr<Base>&& item)
该函数用于添加子元素  
> 显然名称是当时精神状态不加时写的，按理来说应该用`addSubitem`的  
> 管他呢，以后加个就行了
```C++
void addItem(std::shared_ptr<Base>&& item);
```
### erase(const size_t index)
该函数用于删除子元素，指定位置
> 我都不知道为啥当时我不改名称为`eraseSubitem`
```C++
void erase(const size_t index);
```
### clear()
这个用于清除子元素
### getSubitem(const size_t index)
获取一个子元素的引用
> 名称和实际效果有点歧义，以后改
```C++
std::shared_ptr<Base>& getSubitem(const size_t index);
```
### getSubitemsR()
获取`_subitems`的引用
```C++
std::vector<std::shared_ptr<Base>>& getSubitemsR();
```
### isInRange(const Camera::Base& camera)
判断该元素是否在范围中  
返回`true`该元素将被渲染
```C++
virtual bool isInRange(const Camera::Base& camera) const;
```
### Render(Camera::Standard& camera, Item::Renderer::Args args)
`Standard`摄像机的渲染(正常渲染)  
```C++
virtual void Render(Camera::Standard& camera, Item::Renderer::Args args) const;
```
### Render(Camera::Outline& camera)
`Outline`摄像机的渲染(轮廓渲染)
```C++
virtual void Render(Camera::Outline& camera) const;
```
### Render(Camera::Base& camera)
降级渲染, 未必能发挥作用  
```C++
virtual void Render(Camera::Base& camera) const;
```
## 使用
### 构建
构建不需要参数，但一般都需要搞个`std::shared_ptr`
```C++
    auto item = std::make_shared<xII::Item::Base>();
```