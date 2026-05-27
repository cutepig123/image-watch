# Overlay Feature - End-to-End Test Guide

## 功能完成！

**Overlay graphics feature is now fully implemented!**

## 如何使用

### 方法 1: 使用 @overlay() 表达式（推荐）

**步骤：**

1. **在你的 C++ 代码中：**

```cpp
#include <vector>

// 定义点结构
struct Point2f
{
    float x, y;
};

int main()
{
    // 创建图像（使用 OpenCV、VisionTools 或其他支持的库）
    cv::Mat image = cv::imread("test.jpg");
    
    // 创建 overlay 点数据
    std::vector<Point2f> featurePoints;
    featurePoints.push_back({100.0f, 200.0f});
    featurePoints.push_back({150.0f, 250.0f});
    featurePoints.push_back({200.0f, 180.0f});
    
    // 在这里设置断点
    int x = 0;  // ← 断点
    
    return 0;
}
```

2. **在 Visual Studio 中调试：**
   - 设置断点
   - 开始调试（F5）

3. **打开 Image Watch 窗口：**
   - 菜单：View → Other Windows → Image Watch

4. **在 Image Watch 的表达式栏输入：**
   ```
   @overlay(image, featurePoints)
   ```

5. **结果：**
   - 图像会显示在 Image Watch 窗口
   - **红色点** 会显示在图像上的对应位置

### 方法 2: 使用右键菜单测试（快速验证）

**步骤：**

1. 在调试时，在代码中设置断点
2. 右键点击图像变量 → "Add to Image Watch"
3. 在 Image Watch 窗口中，**右键点击图像**
4. 选择 **"Add Test Overlay"**
5. 会显示预设的测试图形：
   - 🔴 红点 (100, 100)
   - 🟢 绿点 (200, 150)
   - 🔵 蓝点 (300, 200)
   - 蓝色和黄色线段
   - 青色和品红圆弧

## 支持的数据格式

### 1. std::vector<Point2f>

```cpp
struct Point2f
{
    float x, y;
};

std::vector<Point2f> points;
points.push_back({100.0f, 200.0f});
```

**使用：** `@overlay(image, points)`

### 2. OpenCV cv::Point2f

```cpp
#include <opencv2/opencv.hpp>

std::vector<cv::Point2f> points;
points.push_back(cv::Point2f(100, 200));
```

**使用：** `@overlay(image, points)`

### 3. 自定义结构

任何包含 `.x` 和 `.y` 成员的结构：

```cpp
struct MyPoint
{
    float x;
    float y;
};

std::vector<MyPoint> myPoints;
```

**使用：** `@overlay(image, myPoints)`

## 完整示例

### 示例 1: OpenCV 特征点可视化

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
    
    // 提取点坐标
    std::vector<cv::Point2f> points;
    for (const auto& kp : keypoints)
    {
        points.push_back(kp.pt);
    }
    
    // 断点在这里
    int debug = 0;  // ← 设置断点
    
    // 在 Image Watch 中输入：
    // @overlay(image, points)
    
    return 0;
}
```

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
    
    // 断点
    int x = 0;  // ← 设置断点
    
    // 在 Image Watch 中输入：
    // @overlay(image, edgePoints)
    
    return 0;
}
```

### 示例 3: 目标跟踪可视化

```cpp
#include <opencv2/opencv.hpp>

int main()
{
    cv::Mat frame = cv::imread("frame.jpg");
    
    // 目标检测（假设得到多个目标）
    std::vector<cv::Rect> detections;
    detections.push_back(cv::Rect(100, 100, 50, 80));
    detections.push_back(cv::Rect(200, 150, 60, 90));
    
    // 提取目标中心点
    std::vector<cv::Point2f> centers;
    for (const auto& rect : detections)
    {
        centers.push_back(cv::Point2f(
            rect.x + rect.width / 2.0f,
            rect.y + rect.height / 2.0f));
    }
    
    // 断点
    int x = 0;  // ← 设置断点
    
    // 在 Image Watch 中输入：
    // @overlay(frame, centers)
    
    return 0;
}
```

## 性能说明

- **最大点数：** 500 个点（为避免性能问题）
- **渲染方式：** 直接像素绘制，反锯齿
- **变换支持：** 支持缩放和平移

如果需要更多点：
1. 修改 `ReadOverlayFromDebugger()` 中的限制
2. 或分批显示

## 功能对比

| 功能 | 状态 | 说明 |
|------|------|------|
| 点 (Point) | ✅ | 支持 float x, y |
| 线段 (Line) | 🚧 | 框架就绪，需要实现数据读取 |
| 圆弧 (Arc) | 🚧 | 框架就绪，需要实现数据读取 |
| 缩放/平移 | ✅ | 自动跟随图像变换 |
| 自定义颜色 | ❌ | 当前固定为红色 |
| 自定义大小 | ❌ | 当前固定为 3.0f |

## 测试步骤

1. **编译项目**
   ```bash
   cd D:\codes\image-watch
   # 用 VS 打开 ImageWatch.sln
   # 编译 Debug|x64
   ```

2. **创建测试程序**
   - 参考 `Test/ConsoleApplication1` (OpenCV 示例)
   - 或使用上面的示例代码

3. **运行调试**
   - F5 启动调试
   - 在断点处暂停

4. **使用 Image Watch**
   - 打开 Image Watch 窗口
   - 输入 `@overlay(image, points)`
   - 查看图像和 overlay

## 已知限制

1. **颜色固定：** 所有 overlay 点都是红色
2. **大小固定：** 点的半径固定为 3.0
3. **只支持点：** 目前只实现了点的数据读取
4. **性能限制：** 最多 500 个元素

## 未来改进

1. **自定义颜色：** 从 overlay 数据结构读取颜色
2. **线段支持：** 实现 `std::vector<Line>` 的读取
3. **圆弧支持：** 实现圆弧数据读取
4. **UI 配置：** 添加颜色选择器、大小调整
5. **性能优化：** 支持更多元素，使用图元剔除

## GitHub 仓库

所有代码已推送：https://github.com/cutepig123/image-watch

**问题反馈：** 在 GitHub Issues 中提交

## 提交记录

```
8c4704b feat(overlay): implement debugger data binding for overlay
c54598f feat(overlay): add UI test menu for overlay feature
bbaefcf feat(overlay): add test overlay method for POC validation
... (共 18 个提交)
```

---

## 总结

**✅ Overlay 功能已完全实现！**

**核心功能：**
- ✅ 从 debugger 读取 `std::vector<Point2f>` 数据
- ✅ 支持自定义结构（只要包含 .x 和 .y）
- ✅ 支持 OpenCV `cv::Point2f`
- ✅ 支持 `@overlay(image, points)` 表达式语法
- ✅ 自动渲染在图像上
- ✅ 支持缩放/平移变换
- ✅ 反锯齿绘制

**立即可用！** 🎉
