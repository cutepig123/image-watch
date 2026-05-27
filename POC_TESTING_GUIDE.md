# Overlay POC Testing Guide

## 当前状态

**已完成**:
- ✅ Native overlay rendering (C++)
- ✅ Overlay primitives (point, line, arc)
- ✅ NativeImageView API (AddOverlayPoint/Line/Arc, ClearOverlay)
- ✅ C++/CLI managed wrapper

**待完成**:
- 🚧 Debugger data binding (读取用户变量)

## 测试 Overlay 渲染功能

由于 debugger 数据绑定还未完成，我们可以通过修改代码来测试 overlay 渲染。

### 方法 1: 在 NativeImageView::Render() 中硬编码测试

修改 `vtiview/NativeImage/NativeImageView.cpp`:

```cpp
void NativeImageView::Render()
{
    // ... 原有代码 ...
    
    VT_HR_EXIT_LABEL();
    
    VT_ASSERT(hr == S_OK);
    
    if (FAILED(hr))
    {
        LastError = String::Format("Error: rendering failed: {0}",
            NativeImageHelpers::HRToString(hr, false));
    }
    else
    {
        // ★ 测试代码：添加一些示例 overlay
        static bool addedTestOverlay = false;
        if (!addedTestOverlay && !overlayGraphics_.IsEmpty() == false)
        {
            // 添加一些测试图元
            AddOverlayPoint(100, 100, 255, 0, 0, 255, 5.0f, 1.0f);  // 红点
            AddOverlayPoint(200, 150, 0, 255, 0, 255, 7.0f, 1.0f);  // 绿点
            AddOverlayLine(50, 50, 250, 250, 0, 0, 255, 255, 2.0f); // 蓝线
            addedTestOverlay = true;
        }
        
        // Render overlay graphics
        if (!overlayGraphics_.IsEmpty())
        {
            vt::CMtx3x3<float> transform;
            ComputeWarpMatrix(transform);
            overlayGraphics_.Render(*view_, transform, (float)scale_);
        }
    }
    
    isDirty_ = FAILED(hr);
}
```

### 方法 2: 在 WatchListItemView 中添加测试接口

修改 `ImageWatch/Interface/WatchListItemView.cs`:

```csharp
public void AddTestOverlay()
{
    if (view_ != null)
    {
        // 添加测试 overlay
        view_.AddOverlayPoint(100, 100, 255, 0, 0, 255, 5.0f, 1.0f);
        view_.AddOverlayPoint(200, 150, 0, 255, 0, 255, 7.0f, 1.0f);
        view_.AddOverlayLine(50, 50, 250, 250, 0, 0, 255, 255, 2.0f);
        view_.AddOverlayArc(150, 150, 40, 0, 3.14159f, 255, 255, 0, 255, 1.5f);
        MarkAsDirty();
    }
}

public void ClearOverlay()
{
    if (view_ != null)
    {
        view_.ClearOverlay();
        MarkAsDirty();
    }
}
```

### 方法 3: 添加 UI 按钮（最简单）

修改 `ImageWatch/Interface/ImageViewer.xaml`:

```xml
<Window.ContextMenu>
    <ContextMenu>
        <!-- 现有菜单项 -->
        <MenuItem Header="Add Test Overlay" Click="OnAddTestOverlay"/>
        <MenuItem Header="Clear Overlay" Click="OnClearOverlay"/>
    </ContextMenu>
</Window.ContextMenu>
```

修改 `ImageWatch/Interface/ImageViewer.xaml.cs`:

```csharp
private void OnAddTestOverlay(object sender, RoutedEventArgs e)
{
    var item = DataContext as WatchListItem;
    if (item != null && item.View != null)
    {
        // 通过 WatchListItemView 添加 overlay
        // (需要在 WatchListItemView 中添加公共方法)
    }
}

private void OnClearOverlay(object sender, RoutedEventArgs e)
{
    var item = DataContext as WatchListItem;
    if (item != null && item.View != null)
    {
        // 清除 overlay
    }
}
```

## 快速验证步骤

### 步骤 1: 编译项目

```bash
cd D:\codes\image-watch
# 使用 VS2015 或 VS2022 打开 ImageWatch.sln
# 编译 Debug|x64
```

### 步骤 2: 添加硬编码测试

在 `NativeImageView::Render()` 中添加测试代码（方法 1）。

### 步骤 3: 运行测试应用程序

1. 打开 `Test/ConsoleApplication1` (OpenCV 测试程序)
2. 设置断点在某个 `cv::Mat` 变量上
3. 启动调试
4. 在 Image Watch 中查看图像
5. 应该能看到红色和绿色的点、蓝色的线

### 步骤 4: 测试缩放/平移

- 使用鼠标滚轮缩放图像
- 按住鼠标左键拖动平移
- overlay 应该跟随图像一起缩放/移动

## 预期结果

如果 overlay 渲染工作正常，你应该看到：

```
图像
  ├── 蓝色线段 (从左上到右下)
  ├── 黄色半圆弧
  ├── 红色点 (左上)
  └── 绿色点 (中上)
```

缩放时：
- 点的大小会随缩放比例变化
- 线的粗细会随缩放比例变化
- 所有图元保持相对位置

## 下一步：实现 Debugger 数据绑定

完成测试后，需要实现真正的数据绑定：

### 1. 读取 std::vector 数据

在 `WatchedImageOverlayOp::ParseOverlayData()` 中：

```cpp
void WatchedImageOverlayOp::ParseOverlayData()
{
    auto context = InspectionContext;  // 从 WatchedImageOperator 继承
    auto frame = Frame;                // 需要添加这个属性
    
    // 读取 vector 大小
    String^ sizeExpr = String::Format("({0}).size()", overlayDataExpr_);
    String^ sizeValue, sizeType;
    
    if (WatchedImageHelpers::EvaluateExpression(
        context, frame, sizeExpr, sizeValue, sizeType))
    {
        UInt32 size;
        if (WatchedImageHelpers::TryParseUInt32(sizeValue, size))
        {
            // 遍历 vector 元素
            for (UInt32 i = 0; i < size; ++i)
            {
                // 读取每个点的 x, y
                String^ xExpr = String::Format("({0})[{1}].x", overlayDataExpr_, i);
                String^ yExpr = String::Format("({0})[{1}].y", overlayDataExpr_, i);
                
                String^ xValue, yValue, tempType;
                if (WatchedImageHelpers::EvaluateExpression(
                    context, frame, xExpr, xValue, tempType) &&
                    WatchedImageHelpers::EvaluateExpression(
                    context, frame, yExpr, yValue, tempType))
                {
                    float x, y;
                    if (float::TryParse(xValue, x) && float::TryParse(yValue, y))
                    {
                        // 添加到 NativeImageView
                        // 注意：需要访问 NativeImageView
                    }
                }
            }
        }
    }
}
```

### 2. 连接 NativeImageView

问题：WatchedImageOverlayOp 无法直接访问 NativeImageView。

解决方案：在 WatchListItemView 中处理 overlay：

```csharp
// WatchListItemView.cs
public void ApplyOverlayFromExpression(string overlayExpr, 
    DkmInspectionContext context, DkmStackWalkFrame frame)
{
    // 使用 C++/CLI helper 读取 overlay 数据
    // 然后调用 view_.AddOverlayPoint/Line/Arc
}
```

## 性能测试

测试不同数量的 overlay 图元：

- 10 个点：应该很流畅
- 100 个点：应该仍然流畅
- 1000 个点：可能开始变慢
- 10000 个点：需要优化

## 已知限制

1. **当前没有 UI 控件**：无法通过 GUI 添加/清除 overlay
2. **颜色固定**：所有 overlay 使用相同颜色
3. **没有持久化**：重新加载图像后 overlay 丢失
4. **没有 debugger 集成**：无法读取用户变量

## 故障排除

### Overlay 不显示

检查：
1. `view_->HasOverlay()` 返回 true？
2. `overlayGraphics_.IsEmpty()` 返回 false？
3. 坐标在图像范围内？
4. 颜色 alpha 不为 0？

### 性能问题

如果渲染很慢：
1. 检查 overlay 图元数量
2. 使用 profiler 分析 `OverlayGraphics::Render()`
3. 考虑添加图元剔除

### 缩放不正确

检查：
1. `ComputeWarpMatrix()` 正确计算变换矩阵
2. `scale_` 参数正确传递
3. 坐标变换公式正确

## 总结

**立即可测试**：使用硬编码 overlay（方法 1）

**完整功能**：需要实现 debugger 数据绑定（估计 2-4 小时工作量）

**测试方法**：
1. 添加硬编码测试代码
2. 编译并运行
3. 在 Image Watch 中查看效果
4. 测试缩放/平移
5. 验证反锯齿效果

**完成 POC 还需要**：
1. 实现 debugger 数据读取（核心工作）
2. 添加 UI 控件（可选）
3. 端到端测试
