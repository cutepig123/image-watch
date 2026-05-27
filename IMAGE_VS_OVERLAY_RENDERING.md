# 原始图像渲染 vs Overlay 渲染对比

## 核心区别

**原始图像渲染** ≠ Overlay 渲染

- 原始图像使用 **Transform Graph（变换图）管道**
- Overlay 使用 **直接像素绘制**

---

## 原始图像渲染流程

### 1. 使用 Transform Graph 管道

```cpp
// NativeImageView.cpp (Line 295-409)
vt::CTransformGraphUnaryNode graph[7];

// 构建处理节点链
graph[0] → graph[1] → graph[2] → graph[3] → graph[4] → graph[5] → graph[6]
```

### 2. 完整的处理管道

```
原始图像数据 (例如: UINT16, 3通道)
    │
    ▼
┌─────────────────────────────────────┐
│ Step 0: Band Select (可选)           │
│ - 选择单个通道                        │
│ - 例如: 选择 R 通道                   │
└─────────────────────────────────────┘
    │
    ▼
┌─────────────────────────────────────┐
│ Step 1: Prescale/Offset              │
│ - 归一化像素值                        │
│ - 例如: UINT16 → FLOAT (除以65535)   │
└─────────────────────────────────────┘
    │
    ▼
┌─────────────────────────────────────┐
│ Step 2: ColorMap Transform           │
│ - 像素值 → 颜色映射                   │
│ - 例如: 伪彩色、灰度、Jet colormap   │
│ - 输出: RGBA Byte                    │
└─────────────────────────────────────┘
    │
    ▼
┌─────────────────────────────────────┐
│ Step 3: Convert to RGBA Float        │
│ - 转换为浮点格式（为了 warp）         │
│ - RGBA Byte → RGBA Float            │
└─────────────────────────────────────┘
    │
    ▼
┌─────────────────────────────────────┐
│ Step 4: Warp Transform               │
│ - 缩放 (Zoom in/out)                 │
│ - 平移 (Pan)                         │
│ - 使用 Lanczos3 或 Nearest 插值      │
└─────────────────────────────────────┘
    │
    ▼
┌─────────────────────────────────────┐
│ Step 5: Pixel Grid (可选)            │
│ - 高倍放大时显示像素网格              │
└─────────────────────────────────────┘
    │
    ▼
┌─────────────────────────────────────┐
│ Step 6: Crop/Pad                     │
│ - 裁剪到可见区域                      │
│ - 填充边界（背景色）                  │
└─────────────────────────────────────┘
    │
    ▼
┌─────────────────────────────────────┐
│ Step 7: Alpha Blend (可选)           │
│ - 混合透明通道                        │
└─────────────────────────────────────┘
    │
    ▼
输出到 view_ (CRGBByteImg)
```

### 3. 关键代码解析

```cpp
// Step 1: Prescale/Offset (归一化)
ScaleOffsetTransform scaleOffsetXform(
    (float)GetColorMapPreScale(cmap, reader.GetImgInfo().type),
    (float)GetColorMapPreOffset(cmap, reader.GetImgInfo().type),
    reader.GetImgInfo().type, 
    selectSingleBand ? 1 : reader.GetImgInfo().Bands()
);
graph[0].SetTransform(&scaleOffsetXform);

// Step 2: ColorMap (像素值 → 颜色)
ColorMapTransform colorMapXform(cmap, 
    selectSingleBand ? 1 : reader.GetImgInfo().Bands());
graph[1].SetTransform(&colorMapXform);

// Step 4: Warp (缩放/平移)
vt::CWarpTransform warpXform;
if (warp(0,0) > 1.0 / minDisplayPixelSizeForNN)
{
    // 高分辨率: 使用 Lanczos3 插值
    warpXform.InitializeResize(..., vt::eSamplerKernelLanczos3);
}
else
{
    // 低分辨率: 使用 Nearest 插值
    warpXform.Initialize(..., vt::eSamplerKernelNearest);
}
graph[3].SetTransform(&warpXform);

// Step 6: Crop/Pad (裁剪/填充)
vt::CCropPadTransform cropXform(cropSrcImgInfo, dst.Rect());
graph[5].SetTransform(&cropXform);

// 执行整个管道
graphHead->SetDest(&dst);
vt::PushTransformTaskAndWait(graphHead, ...);  // ← 在这里执行所有变换
```

### 4. VisionTools 库的作用

原始图像渲染使用 **VisionTools** 库的强大功能：

- **vt::CTransformGraphUnaryNode**: 变换图节点
- **vt::CWarpTransform**: 高质量图像变换（Lanczos3 插值）
- **vt::CColorMapTransform**: 颜色映射
- **vt::CCropPadTransform**: 裁剪和填充
- **vt::PushTransformTaskAndWait**: 多线程并行执行

**特点：**
- ✅ 高质量插值（Lanczos3, Bilinear, Nearest）
- ✅ 多线程并行处理
- ✅ 支持多种像素格式（UINT8, UINT16, FLOAT, HALF_FLOAT 等）
- ✅ 支持 Python Pyramid（多分辨率）

---

## Overlay 渲染流程

### 1. 直接像素绘制

```cpp
// NativeImageView.cpp (Line 422-428)
if (!overlayGraphics_.IsEmpty())
{
    vt::CMtx3x3<float> transform;
    ComputeWarpMatrix(transform);
    overlayGraphics_.Render(*view_, transform, (float)scale_);
}
```

### 2. 简单的逐像素操作

```cpp
// OverlayPrimitives.cpp
void OverlayGraphics::Render(vt::CRGBAByteImg& dst, ...)
{
    // 遍历所有线段，逐个绘制
    for (const auto& line : lines)
    {
        DrawLine(dst, x0, y0, x1, y1, color);  // ← 直接修改 dst 像素
    }
    
    // 遍历所有点，逐个绘制
    for (const auto& pt : points)
    {
        DrawPoint(dst, x, y, radius, color);  // ← 直接修改 dst 像素
    }
}

// 绘制单个像素
static void SetPixel(vt::CRGBAByteImg& dst, int x, int y, const vt::CRGBAPix& color)
{
    if (x >= 0 && x < dst.Width() && y >= 0 && y < dst.Height())
    {
        vt::RGBAPix* p = dst.Ptr(y);
        p[x] = color;  // ← 直接赋值
    }
}
```

**特点：**
- ✅ 简单直接
- ✅ 无需复杂的数据结构
- ⚠️ 逐像素操作，性能较低
- ⚠️ 使用简单的反锯齿算法（Xiaolin Wu）

---

## 对比总结

| 特性 | 原始图像渲染 | Overlay 渲染 |
|------|-------------|-------------|
| **机制** | Transform Graph 管道 | 直接像素绘制 |
| **数据处理** | 像素值变换（归一化、颜色映射） | 几何图元绘制（点、线、圆弧） |
| **缩放/平移** | 高质量插值（Lanczos3） | 坐标变换 + 简单绘制 |
| **性能** | 多线程并行处理 | 单线程逐像素 |
| **灵活性** | 支持多种像素格式 | 固定 RGBA 格式 |
| **用途** | 显示图像数据 | 绘制标注、特征点 |

---

## 为什么这样设计？

### 原始图像需要高质量渲染

**需求：**
- 支持 UINT8, UINT16, FLOAT, HALF_FLOAT 等多种格式
- 支持 1-4 通道
- 支持伪彩色映射
- 高质量缩放（Lanczos3 插值）
- 多线程并行处理大图像

**解决方案：** 使用 VisionTools 的 Transform Graph

### Overlay 只需要简单标注

**需求：**
- 绘制简单的几何图元（点、线、圆弧）
- 支持缩放/平移同步
- 不需要复杂的插值算法
- 图元数量通常较少（几十到几百个）

**解决方案：** 直接像素绘制就足够了

---

## 完整渲染流程图

```
┌──────────────────────────────────────────────────────┐
│                NativeImageView::Render()              │
└──────────────────────────────────────────────────────┘
                         │
         ┌───────────────┴───────────────┐
         ▼                               ▼
┌──────────────────────┐      ┌──────────────────────┐
│   原始图像渲染        │      │   Overlay 渲染       │
│                      │      │   (可选)             │
│  Transform Graph     │      │                      │
│  ┌────────────────┐  │      │  ┌────────────────┐  │
│  │ Band Select    │  │      │  │ Direct Pixel   │  │
│  │ Prescale       │  │      │  │ Drawing        │  │
│  │ ColorMap       │  │      │  │                │  │
│  │ Warp (Lanczos) │  │      │  │ - DrawLine()   │  │
│  │ Crop/Pad       │  │      │  │ - DrawPoint()  │  │
│  └────────────────┘  │      │  │ - DrawArc()    │  │
│         │            │      │  └────────────────┘  │
│         ▼            │      │         │            │
│   输出到 view_       │      │         │            │
└─────────┬────────────┘      └─────────┬────────────┘
          │                             │
          └──────────┬──────────────────┘
                     ▼
            ┌────────────────┐
            │   view_        │
            │ (CRGBByteImg)  │
            │                │
            │ 图像 + Overlay │
            └────────────────┘
                     │
                     ▼
            ┌────────────────┐
            │ InteropBitmap  │
            │    ↓           │
            │ WPF Canvas     │
            └────────────────┘
```

---

## 关键区别总结

### 原始图像渲染 = 数据处理管道

```
像素值 → 归一化 → 颜色映射 → 变换 → 输出
```

**重点：** 像素值的**数学变换**

### Overlay 渲染 = 几何绘制

```
图元坐标 → 变换 → 逐像素绘制 → 输出
```

**重点：** 几何图元的**光栅化**

---

## 代码位置对比

### 原始图像渲染

**主要代码：** `vtiview/NativeImage/NativeImageView.cpp` (Line 214-409)

**关键函数：**
- `NativeImageView::Render()` - 构建和执行 Transform Graph
- VisionTools 库的变换类（CWarpTransform, CColorMapTransform 等）

### Overlay 渲染

**主要代码：** `vtiview/NativeImage/OverlayPrimitives.cpp` (Line 147-226)

**关键函数：**
- `OverlayGraphics::Render()` - 遍历图元并绘制
- `DrawLine()`, `DrawPoint()`, `DrawArc()` - 逐像素绘制函数
- `SetPixel()` - 设置单个像素颜色

---

## 总结

**原始图像渲染**使用专业的图像处理管道（VisionTools），支持：
- 高质量插值
- 多种像素格式
- 颜色映射
- 多线程并行

**Overlay 渲染**使用简单的直接像素绘制，因为：
- 只需要绘制简单的几何图元
- 图元数量较少
- 不需要复杂的插值算法
- 性能足够

两者共享同一个输出缓冲区 `view_`，overlay 在图像渲染完成后直接绘制在图像上。
