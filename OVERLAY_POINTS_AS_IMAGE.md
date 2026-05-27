# Overlay Points as Image - Design

## 问题

用户希望直接右键 `std::vector<Point2f>` 变量，选择 "Add to Image Watch"。

但是 Image Watch 期望的是**图像类型**，需要 `[width]`, `[height]`, `[data]` 等元数据。

## 解决方案

### 方案 1: 创建虚拟图像（推荐）

当检测到点类型时，创建一个虚拟图像，在上面绘制点：

```
用户右键: points (std::vector<Point2f>)
    ↓
检测到点类型
    ↓
创建虚拟图像（根据点的范围自动计算大小）
    ↓
在虚拟图像上绘制点
    ↓
显示在 Image Watch
```

**实现**：
1. 创建 `WatchedImagePoints` 类
2. 在 `WatchedImageTypeMap` 中注册点类型
3. 读取点数据并创建图像

### 方案 2: 合并到已有图像

用户先添加图像，然后指定 overlay 点。这是当前的实现。

## 实现（方案 1）

### 步骤 1: 创建 WatchedImagePoints 类

这个类会：
1. 读取点的数量和坐标
2. 计算边界框
3. 创建一个空白图像
4. 在图像上绘制点

### 步骤 2: 注册点类型

在 `WatchedImageTypeMap.Initialize()` 中添加：

```csharp
// Point types (create virtual image with points)
AddImage(@"std::vector<cv::Point_<.+>", "PointsVector", typeof(WatchedImagePoints));
AddImage(@"std::vector<Point2f>", "PointsVector", typeof(WatchedImagePoints));
```

### 步骤 3: natvis 支持

natvis 文件已经添加了 `UIVisualizer`，所以右键菜单会出现。

## 当前实现状态

**已实现**：
- ✅ `@overlay(image, points)` 语法
- ✅ 右键菜单添加点类型（natvis）
- ❌ 点类型创建虚拟图像（待实现）

**立即可用**：
用户可以先添加图像，然后使用 overlay 语法：
```
1. 右键图像 → Add to Image Watch
2. 在表达式栏输入: @overlay(image, points)
```

**需要实现**：
点类型直接创建虚拟图像的功能。
