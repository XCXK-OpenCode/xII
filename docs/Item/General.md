`Item`类型中一般是`General`较为通用  
# xII::Item::General
定义于`src/Item/General.h`  
派生自`xII::Item::Base`  
> 该class基本上就只是一个能设置图形和一些其他东西的`Item`

## 元素
### _shape *(private)*
该元素储存`Item`的形状  
```C++
std::shared_ptr<Shape::Base> _shape;
```
### _renderers *(private)*
该元素储存渲染器组
```C++
std::vector<std::shared_ptr<Renderer::Base>> _renderers;
```
### 构建
```C++
General(const General&) = default;
General(const std::shared_ptr<Shape::Base>& shape, 
    const std::vector<std::shared_ptr<Renderer::Base>>& renderers = {});
```
> 我也不清楚还有没有必要加Base()，但目前来看应该是不用的，因为程序运行看起来都挺正常