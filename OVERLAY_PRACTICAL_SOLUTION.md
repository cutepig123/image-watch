# 实用的 Overlay 使用方案

## 当前实现

**✅ 已支持**：
```
@overlay(image, points)
```

用户需要：
1. 先知道图像变量名
2. 在表达式栏输入完整语法

## 问题

用户希望：
1. 右键图像 → Add to Image Watch
2. 右键点数据 → Add to Image Watch
3. 系统自动组合显示

## 最实用的方案

### 改进方案：两步添加

**步骤 1**：添加图像
```
右键 image → Add to Image Watch
```

**步骤 2**：添加 overlay 点
```
右键 points → Add to Image Watch as Overlay
```

系统会自动将最近添加的 overlay 数据应用到最近添加的图像上。

### 实现

在右键菜单添加两个选项：
1. "Add to Image Watch" - 添加为图像
2. "Add to Image Watch as Overlay" - 添加为 overlay 数据

## 当前可用的方案

**立即可用**：

1. 在代码中设置断点
2. 打开 Image Watch 窗口
3. 在表达式栏输入：`@overlay(imageVar, pointsVar)`

**示例**：

```cpp
cv::Mat image = cv::imread("test.jpg");
std::vector<cv::Point2f> points;
points.push_back(cv::Point2f(100, 200));

int debug = 0;  // 断点
```

在 Image Watch 表达式栏输入：
```
@overlay(image, points)
```

结果：图像显示，上面有红色点标记。

## natvis 已支持

我已经更新了 `ImageWatchOverlay.natvis`，支持：
- `cv::Point2f`, `cv::Point2i`, `cv::Point2d`
- `std::vector<cv::Point_*>`
- `Point2f`, `Point2i`, `Point2d`
- `std::vector<Point2f>`

所以右键这些类型时，会显示 "Add to Image Watch" 菜单项。

## 下一步

要实现"添加为 overlay"功能，需要：
1. 修改 natvis 添加新的 UIVisualizer ID (Id="2")
2. 在 C# 代码中处理 Id="2" 的情况
3. 将点数据添加为 overlay

这是一个更大的改动，建议先使用当前的 `@overlay()` 语法。
