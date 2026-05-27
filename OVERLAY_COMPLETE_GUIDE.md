# Overlay Feature - Complete Usage Guide

## ✅ 功能完全实现！

Overlay graphics feature is now fully implemented with intuitive right-click menu support.

---

## 使用方法

### 方法 1: 右键菜单（最简单，推荐）

**步骤 1：添加图像**
```
在调试器中：
1. 找到图像变量（如 cv::Mat image）
2. 右键点击变量
3. 选择 "Add to Image Watch"
```

**步骤 2：添加 Overlay 点**
```
在调试器中：
1. 找到点数据变量（如 std::vector<cv::Point2f> points）
2. 右键点击变量
3. 选择 "Add as Overlay to Image Watch"
```

**结果**：
- 图像显示在 Image Watch 窗口
- 点自动显示在图像上（红色标记）
- 缩放/平移时 overlay 自动跟随

### 方法 2: 表达式语法（灵活）

在 Image Watch 表达式栏输入：
```
@overlay(imageVariable, pointsVariable)
```

**示例**：
```
@overlay(frame, keypoints)
@overlay(img, featurePoints)
@overlay(renderTarget, debugPoints)
```

### 方法 3: 测试菜单（快速验证）

```
1. 右键图像 → Add to Image Watch
2. 在 Image Watch 窗口右键图像
3. 选择 "Add Test Overlay"
4. 会显示预设的测试图形
```

---

## 支持的数据类型

### OpenCV 类型
- `cv::Point2f` - 单个点
- `cv::Point2i` - 整数点
- `cv::Point2d` - 双精度点
- `std::vector<cv::Point2f>` - 点集合
- `std::vector<cv::Point2i>` - 整数点集合
- `std::vector<cv::Point>` - 点集合

### 自定义类型
任何包含 `.x` 和 `.y` 成员的结构：
```cpp
struct Point2f { float x, y; };
struct Point2d { double x, y; };
struct Point2i { int x, y; };

std::vector<Point2f> myPoints;
```

### 图像类型
所有 Image Watch 支持的图像类型：
- `cv::Mat` (OpenCV)
- `vt::CImg` (VisionTools)
- `ID3D11Texture2D` (DirectX)
- 以及所有用户自定义图像类型

---

## 完整示例

### 示例 1: OpenCV 特征点检测

```cpp
#include <opencv2/opencv.hpp>

int main()
{
    // 加载图像
    cv::Mat image = cv::imread("test.jpg");
    
    // 检测特征点
    std::vector<cv::KeyPoint> keypoints;
    cv::Ptr<cv::ORB> orb = cv::ORB::create();
    orb->detect(image, keypoints);
    
    // 转换为 Point2f 格式
    std::vector<cv::Point2f> points;
    for (const auto& kp : keypoints)
        points.push_back(kp.pt);
    
    // 在这里设置断点
    int debug = 0;  // ← F9 设置断点
    
    return 0;
}
```

**调试步骤**：
1. F5 启动调试
2. 断点处暂停
3. 右键 `image` → "Add to Image Watch"
4. 右键 `points` → "Add as Overlay to Image Watch"
5. 查看图像上的特征点！

### 示例 2: 边缘检测可视化

```cpp
#include <opencv2/opencv.hpp>

int main()
{
    cv::Mat image = cv::imread("test.jpg", cv::IMREAD_GRAYSCALE);
    
    // Canny 边缘检测
    cv::Mat edges;
    cv::Canny(image, edges, 50, 150);
    
    // 查找边缘点
    std::vector<cv::Point> edgePoints;
    cv::findNonZero(edges, edgePoints);
    
    // 只取前 500 个点（性能考虑）
    if (edgePoints.size() > 500)
        edgePoints.resize(500);
    
    int debug = 0;  // ← 断点
    return 0;
}
```

**调试**：
```
右键 image → Add to Image Watch
右键 edgePoints → Add as Overlay to Image Watch
```

### 示例 3: 目标跟踪

```cpp
#include <opencv2/opencv.hpp>

int main()
{
    cv::Mat frame = cv::imread("frame.jpg");
    
    // 目标检测结果
    std::vector<cv::Rect> detections;
    detections.push_back(cv::Rect(100, 100, 50, 80));
    detections.push_back(cv::Rect(200, 150, 60, 90));
    
    // 提取中心点
    std::vector<cv::Point2f> centers;
    for (const auto& rect : detections)
    {
        centers.push_back(cv::Point2f(
            rect.x + rect.width / 2.0f,
            rect.y + rect.height / 2.0f));
    }
    
    int debug = 0;  // ← 断点
    return 0;
}
```

**调试**：
```
右键 frame → Add to Image Watch
右键 centers → Add as Overlay to Image Watch
```

---

## 功能特性

### ✅ 已实现

| 功能 | 状态 | 说明 |
|------|------|------|
| 点渲染 | ✅ | 反锯齿圆形点 |
| 右键菜单 | ✅ | "Add as Overlay to Image Watch" |
| 自动关联 | ✅ | 自动关联到最近的图像 |
| 缩放/平移 | ✅ | Overlay 跟随图像变换 |
| OpenCV 支持 | ✅ | cv::Point2f, std::vector |
| 自定义类型 | ✅ | 任何包含 .x, .y 的结构 |
| 表达式语法 | ✅ | @overlay(image, points) |

### 🚧 未来增强

| 功能 | 优先级 | 说明 |
|------|--------|------|
| 线段支持 | 中 | std::vector<Line> 数据 |
| 圆弧支持 | 低 | 圆弧图元 |
| 自定义颜色 | 中 | 从数据结构读取颜色 |
| 自定义大小 | 低 | 可配置点大小 |
| UI 控件 | 低 | 颜色选择器、大小调整 |

---

## 性能说明

- **最大点数**: 500 个点（性能优化）
- **渲染方式**: Native C++，反锯齿
- **更新频率**: 实时（跟随图像刷新）

**性能建议**：
- 点数 < 100：非常流畅
- 点数 100-500：流畅
- 点数 > 500：考虑采样或分批显示

---

## 故障排除

### 问题：右键菜单没有 "Add as Overlay" 选项

**原因**：变量类型不被识别

**解决**：
1. 确保变量类型包含 `.x` 和 `.y` 成员
2. 或使用表达式语法：`@overlay(image, points)`

### 问题：Overlay 不显示

**检查**：
1. 图像是否已添加到 Image Watch？
2. 点数据是否有效（非空）？
3. 点坐标是否在图像范围内？

### 问题：点显示位置不对

**可能原因**：
- 点坐标超出图像范围
- 坐标系不匹配（OpenCV 使用左上角原点）

---

## 技术细节

### 工作流程

```
用户右键 points 变量
    ↓
选择 "Add as Overlay to Image Watch"
    ↓
VisualizerService.DisplayValue(visualizerId=2)
    ↓
AddOverlayWatch(expression)
    ↓
Controller.AddOverlayToLastImage()
    ↓
找到最近的图像项
    ↓
修改表达式为 @overlay(image, points)
    ↓
WatchListItem 加载 overlay 数据
    ↓
NativeImageView 渲染 overlay
    ↓
显示在 Image Watch 窗口
```

### 架构

```
C# (UI Layer)
    ├── WatchListItem - 管理表达式和数据
    ├── Controller - 处理右键菜单事件
    └── WatchListItemView - 管理 NativeImageView
    
C++/CLI (Bridge Layer)
    ├── WatchedImage - 图像数据抽象
    └── NativeImageView - 图像渲染
    
C++ (Native Layer)
    ├── OverlayGraphics - Overlay 数据容器
    └── OverlayPrimitives - 图元渲染（点、线、圆弧）
```

---

## GitHub 仓库

https://github.com/cutepig123/image-watch

**提交记录**：共 21 个提交

**问题反馈**：在 GitHub Issues 中提交

---

## 总结

**✅ Overlay 功能完全实现！**

**三种使用方式**：
1. **右键菜单** - 最简单，推荐
2. **表达式语法** - 最灵活
3. **测试菜单** - 快速验证

**立即可用**，无需额外配置！

**示例**：
```cpp
cv::Mat image;
std::vector<cv::Point2f> points;
// ... 设置断点
```

```
右键 image → Add to Image Watch
右键 points → Add as Overlay to Image Watch
完成！
```

🎉 **享受可视化调试体验！**
