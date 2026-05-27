# Overlay POC - Practical Implementation Plan

## 当前理解

Image Watch 的工作流程：
```
用户在调试器中右键点击变量
    ↓
选择 "Add to Image Watch"
    ↓
表达式添加到 WatchList
    ↓
WatchListItem 管理该表达式
    ↓
WatchListItemView 渲染图像
```

## Overlay 的正确用法

### 方案 1: 使用操作符表达式（推荐）

用户在 Image Watch 窗口的表达式栏中输入：

```
@overlay(imageVariable, overlayDataVariable)
```

**优点**：
- 符合现有的操作符模式（如 @scale, @band 等）
- 不需要修改 UI
- 用户可以自由组合图像和 overlay

**实现步骤**：
1. WatchListItem 解析 `@overlay()` 表达式
2. 创建 WatchedImageOverlayOp
3. 读取图像和 overlay 数据
4. 渲染时调用 NativeImageView 的 overlay API

### 方案 2: 自动检测 overlay 数据（智能）

当用户添加图像变量时，自动查找同名的 overlay 变量：

```
用户添加: imageVariable
自动查找: imageVariable_overlay
```

**优点**：
- 用户体验更简单
- 自动关联

**缺点**：
- 需要命名约定
- 不够灵活

### 方案 3: 多选添加（UI 增强）

允许用户同时选择图像和 overlay 变量，右键菜单添加：

```
选择: imageVariable, pointData
右键: "Add to Image Watch with Overlay"
```

**优点**：
- 直观
- 不需要记住操作符语法

**缺点**：
- 需要修改 VS 扩展 UI
- 实现复杂

## 当前 POC 状态

**已完成**：
- ✅ Native overlay rendering
- ✅ `@overlay()` operator framework
- ✅ Test method `AddTestOverlay()`

**缺失**：
- ❌ Debugger 数据读取
- ❌ 与 WatchListItemView 的集成

## 最小可行实现

### 步骤 1: 硬编码测试（立即可用）

在代码中直接调用：

```csharp
// WatchListItemView.cs
public void TestOverlay()
{
    if (view_ != null)
    {
        view_.AddTestOverlay();  // 添加测试 overlay
    }
}
```

### 步骤 2: 实现 `@overlay(image, data)` 语法

修改 `WatchedImageOverlayOp.cpp`:

```cpp
void WatchedImageOverlayOp::ParseOverlayData()
{
    // 假设 overlayDataExpr_ 是 "myPoints"
    // 我们需要读取这个变量的数据
    
    // 方法 1: 简单实现 - 读取 std::vector<float> 格式
    // 方法 2: 完整实现 - 通过 natvis 读取任意结构
}
```

### 步骤 3: 端到端集成

1. 用户在 Image Watch 输入框输入：`@overlay(img, points)`
2. ExpressionHelper 解析为：图像= img, overlay = points
3. WatchedImageOverlayOp 被创建
4. 在 DoReloadPixels() 中：
   - 加载图像数据
   - 读取 points 变量数据
   - 调用 NativeImageView::AddOverlayPoint()

## 简化实现方案

由于 Debugger API 复杂，我建议采用以下简化方案：

### 方案 A: 支持 OpenCV 格式（最实用）

假设用户使用 OpenCV：

```cpp
std::vector<cv::Point2f> points;
cv::Mat image;
```

在 Image Watch 中：
```
@overlay(image, points)
```

实现：
1. 读取 `points.size()`
2. 遍历 `points[i].x`, `points[i].y`
3. 添加到 overlay

### 方案 B: 约定格式

定义标准 overlay 格式：

```cpp
struct OverlayData
{
    float* points;  // x0,y0,x1,y1,... 
    int numPoints;
    float* lines;   // x0,y0,x1,y1,x2,y2,...
    int numLines;
    unsigned char r, g, b, a;
};
```

在 Image Watch 中：
```
@overlay(image, overlayData)
```

## 立即可测试的方案

**最简单的测试方法**（无需 debugger 集成）：

### 修改 WatchListItemView

```csharp
// WatchListItemView.cs
public void AddTestOverlay()
{
    if (view_ != null)
    {
        view_.AddTestOverlay();
    }
}
```

### 修改 ImageViewer.xaml.cs

添加测试按钮或菜单项：

```csharp
private void OnAddTestOverlay(object sender, RoutedEventArgs e)
{
    var item = DataContext as WatchListItem;
    if (item?.View != null)
    {
        item.View.AddTestOverlay();
    }
}
```

### 测试步骤

1. 编译 Image Watch 扩展
2. 在 VS 中调试一个包含图像的程序
3. 在 Image Watch 窗口查看图像
4. 点击测试按钮/菜单
5. 应该看到 overlay 图形显示在图像上

## 下一步行动

**立即可做**：
1. 添加 UI 测试按钮到 ImageViewer
2. 测试 overlay 渲染是否工作
3. 验证缩放/平移功能

**后续改进**：
1. 实现 debugger 数据读取
2. 支持 `@overlay()` 表达式
3. 支持 OpenCV 格式
4. 添加配置 UI（颜色、大小等）

## 结论

**POC 当前状态**：核心渲染功能已完成，缺少的是：
1. UI 入口（测试按钮）
2. Debugger 数据读取（表达式解析）

**建议**：先添加 UI 测试入口，验证渲染功能工作正常，然后再实现完整的 debugger 集成。

需要我添加 UI 测试入口吗？
