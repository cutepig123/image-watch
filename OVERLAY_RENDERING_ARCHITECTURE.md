# Overlay Rendering Architecture

## 概述

本文档详细说明 overlay 渲染如何与原有图像渲染管道组合，以及 `overlayGraphics_` 对象的工作机制。

## 整体渲染流程

```
┌─────────────────────────────────────────────────────────────┐
│                    NativeImageView::Render()                 │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
        ┌────────────────────────────────────┐
        │  1. 图像数据源检查                   │
        │     - sourceImage_ != nullptr?      │
        │     - 图像是否已释放?                 │
        └────────────────────────────────────┘
                              │
                              ▼
        ┌────────────────────────────────────┐
        │  2. 计算金字塔层级和变换矩阵          │
        │     - ComputeNearestLowerLevel()    │
        │     - ComputeWarpMatrix()           │
        └────────────────────────────────────┘
                              │
                              ▼
        ┌────────────────────────────────────┐
        │  3. 图像处理管道 (Transform Graph)  │
        │     graph[0]: Prescale/Offset       │
        │     graph[1]: ColorMap              │
        │     graph[2]: Convert to RGBA Float │
        │     graph[3]: Warp (缩放/平移)       │
        │     graph[4]: Pixel Grid            │
        │     graph[5]: Crop/Pad              │
        │     graph[6]: Alpha Blend           │
        └────────────────────────────────────┘
                              │
                              ▼
        ┌────────────────────────────────────┐
        │  4. 执行渲染任务                     │
        │     PushTransformTaskAndWait()      │
        │     ↓                               │
        │     输出: view_ (CRGBByteImg)       │
        └────────────────────────────────────┘
                              │
                              ▼
        ┌────────────────────────────────────┐
        │  5. ★ Overlay 渲染 (新增)          │
        │     if (!overlayGraphics_.IsEmpty())│
        │       overlayGraphics_.Render()     │
        └────────────────────────────────────┘
                              │
                              ▼
        ┌────────────────────────────────────┐
        │  6. 输出到 WPF                      │
        │     InteropBitmap 显示              │
        └────────────────────────────────────┘
```

## overlayGraphics_ 对象详解

### 1. 定义位置

`overlayGraphics_` 是 `NativeImageView` 类的私有成员变量：

```cpp
// NativeImageView.h (Line ~265)
private:
    OverlayGraphics overlayGraphics_;
```

### 2. 数据结构

```cpp
class OverlayGraphics
{
public:
    std::vector<OverlayPoint> points;  // 点集合
    std::vector<OverlayLine> lines;    // 线段集合
    std::vector<OverlayArc> arcs;      // 圆弧集合
    
    void Clear();                      // 清空所有图元
    bool IsEmpty() const;              // 检查是否为空
    void Render(...);                  // 渲染到目标图像
};
```

### 3. 图元类型

每种图元包含以下信息：

**OverlayPoint (点)**
```cpp
struct OverlayPoint {
    float x, y;        // 图像坐标 (像素)
    float radius;      // 点的大小 (scale=1时的像素半径)
    vt::CRGBAPix color; // RGBA颜色
    float lineWidth;   // 线宽
    bool visible;      // 是否可见
};
```

**OverlayLine (线段)**
```cpp
struct OverlayLine {
    float x0, y0;      // 起点 (图像坐标)
    float x1, y1;      // 终点 (图像坐标)
    vt::CRGBAPix color; // RGBA颜色
    float lineWidth;   // 线宽
    bool visible;      // 是否可见
};
```

**OverlayArc (圆弧)**
```cpp
struct OverlayArc {
    float cx, cy;         // 圆心 (图像坐标)
    float radius;         // 半径 (像素)
    float startAngle;     // 起始角度 (弧度)
    float endAngle;       // 结束角度 (弧度)
    vt::CRGBAPix color;   // RGBA颜色
    float lineWidth;      // 线宽
    bool visible;         // 是否可见
};
```

## Overlay 渲染如何工作

### 步骤 1: 添加图元

用户通过以下方法添加 overlay 图元：

```cpp
// NativeImageView.cpp
void NativeImageView::AddOverlayPoint(float x, float y, 
                                       vt::Byte r, vt::Byte g, 
                                       vt::Byte b, vt::Byte a,
                                       float radius, float lineWidth)
{
    OverlayPoint pt;
    pt.x = x;
    pt.y = y;
    pt.radius = radius;
    pt.lineWidth = lineWidth;
    pt.color.r = r;
    pt.color.g = g;
    pt.color.b = b;
    pt.color.a = a;
    
    overlayGraphics_.points.push_back(pt);  // 添加到集合
    MarkAsDirty();  // 标记需要重新渲染
}
```

**关键点：**
- 图元使用**图像坐标**（原始像素坐标）
- 添加后调用 `MarkAsDirty()` 触发重新渲染

### 步骤 2: 图像渲染完成后触发 Overlay 渲染

在 `NativeImageView::Render()` 中：

```cpp
// NativeImageView.cpp (Line 411-429)
VT_HR_EXIT_LABEL();

VT_ASSERT(hr == S_OK);

if (FAILED(hr))
{
    LastError = String::Format("Error: rendering failed: {0}", ...);
}
else
{
    // ★ 图像渲染成功后，渲染 overlay
    if (!overlayGraphics_.IsEmpty())
    {
        vt::CMtx3x3<float> transform;
        ComputeWarpMatrix(transform);  // 获取变换矩阵
        overlayGraphics_.Render(*view_, transform, (float)scale_);
    }
}
```

**关键流程：**
1. **先渲染图像** → 输出到 `view_` (CRGBByteImg)
2. **再渲染 overlay** → 直接绘制到 `view_` 上
3. **共享同一个缓冲区** → overlay 直接修改图像像素

### 步骤 3: Overlay 渲染实现

```cpp
// OverlayPrimitives.cpp
void OverlayGraphics::Render(vt::CRGBAByteImg& dst, 
                              const vt::CMtx3x3<float>& transform, 
                              float scale)
{
    // 1. 渲染所有线段 (在底层)
    for (const auto& line : lines)
    {
        if (!line.visible) continue;
        
        // 变换坐标：图像坐标 → 屏幕坐标
        vt::CVec2f p0 = TransformPoint(transform, line.x0, line.y0);
        vt::CVec2f p1 = TransformPoint(transform, line.x1, line.y1);
        
        DrawLine(dst, p0.x, p0.y, p1.x, p1.y, line.color);
    }
    
    // 2. 渲染所有圆弧 (中层)
    for (const auto& arc : arcs) { ... }
    
    // 3. 渲染所有点 (顶层)
    for (const auto& pt : points)
    {
        vt::CVec2f p = TransformPoint(transform, pt.x, pt.y);
        float scaledRadius = pt.radius * scale;  // 根据缩放调整大小
        
        DrawPoint(dst, p.x, p.y, scaledRadius, pt.color);
    }
}
```

## 坐标变换详解

### 变换矩阵的作用

```
图像坐标 (x, y)
      │
      │ 应用变换矩阵 transform
      ▼
屏幕坐标 (x', y')
```

**变换矩阵包含：**
- **缩放 (Scale)**: 用户缩放图像时，overlay 也同步缩放
- **平移 (Pan)**: 用户平移图像时，overlay 同步移动
- **原点偏移**: 图像原点与屏幕原点的偏移

### 示例

假设用户：
1. 在原始图像坐标 (100, 200) 处添加一个点
2. 缩放到 2x (scale = 2.0)
3. 平移到屏幕中心

**变换过程：**
```
原始坐标: (100, 200)
    ↓ 应用变换矩阵
屏幕坐标: (100 * 2 + offsetX, 200 * 2 + offsetY)
         = (200 + offsetX, 400 + offsetY)
```

**点的半径也会缩放：**
```
原始半径: 3.0 像素
缩放后:   3.0 * 2.0 = 6.0 像素
```

## 渲染层次

```
┌─────────────────────────────────┐
│   WPF Canvas (显示层)            │
│   ┌───────────────────────────┐ │
│   │  InteropBitmap (位图)      │ │
│   │  ┌─────────────────────┐  │ │
│   │  │  原始图像 (RGB)      │  │ │
│   │  │                     │  │ │
│   │  │  ┌───────────────┐  │  │ │
│   │  │  │ Overlay 线段  │  │  │ │
│   │  │  │   (底层)      │  │  │ │
│   │  │  ├───────────────┤  │  │ │
│   │  │  │ Overlay 圆弧  │  │  │ │
│   │  │  │   (中层)      │  │  │ │
│   │  │  ├───────────────┤  │  │ │
│   │  │  │ Overlay 点    │  │  │ │
│   │  │  │   (顶层)      │  │  │ │
│   │  │  └───────────────┘  │  │ │
│   │  └─────────────────────┘  │ │
│   └───────────────────────────┘ │
└─────────────────────────────────┘
```

**渲染顺序：**
1. 线段（底层）→ 先绘制，会被点覆盖
2. 圆弧（中层）
3. 点（顶层）→ 最后绘制，显示在最上面

## 关键技术点

### 1. 共享缓冲区

```cpp
vt::CRGBAByteImg& dst  // 这就是 view_，已渲染好的图像
```

- overlay **直接修改**图像像素
- 不需要额外的缓冲区
- 性能高效（无内存拷贝）

### 2. 反锯齿绘制

使用 **Xiaolin Wu 算法** 绘制平滑线条：

```cpp
// OverlayPrimitives.cpp
static void DrawLine(vt::CRGBAByteImg& dst, float x0, float y0, 
                     float x1, float y1, const vt::CRGBAPix& color)
{
    // 计算每个像素的 alpha 值
    // 边缘像素部分透明，实现反锯齿
    float alpha = 1.0f - (intery - floor(intery));
    SetPixel(dst, x, y, BlendColor(color, background, alpha));
}
```

### 3. 脏标记机制

```cpp
void NativeImageView::AddOverlayPoint(...)
{
    overlayGraphics_.points.push_back(pt);
    MarkAsDirty();  // ← 设置 isDirty_ = true
}
```

**工作流程：**
1. 添加图元 → 设置 `isDirty_ = true`
2. WPF 请求刷新 → 调用 `Render()`
3. 检查 `if (!isDirty_) return;` → 决定是否重新渲染

## 性能优化考虑

### 当前实现的优点：
- ✅ 直接绘制到图像缓冲区，无额外开销
- ✅ 支持缩放/平移变换
- ✅ 反锯齿渲染

### 潜在优化（未来）：
1. **图元剔除**: 只渲染可见区域内的图元
2. **LOD (细节层次)**: 低缩放时简化绘制
3. **GPU 加速**: 使用 DirectX/OpenGL 渲染
4. **缓存机制**: 不变的部分缓存起来

## 总结

**overlayGraphics_** 是一个容器对象，负责：
1. **存储图元**: 保存点、线、圆弧的几何信息和颜色
2. **渲染图元**: 将图元绘制到已渲染的图像上
3. **处理变换**: 根据用户的缩放/平移调整图元位置

**工作流程：**
```
添加图元 → overlayGraphics_.points.push_back()
         → MarkAsDirty()
         
渲染时   → 图像渲染完成
         → overlayGraphics_.Render(view_, transform, scale)
         → 直接修改 view_ 像素
         → 显示到 WPF
```

这种设计简单高效，overlay 和图像共享同一个缓冲区，无需额外的合成步骤。
