# Overlay Data Binding Implementation

## 问题分析

当前架构：
```
用户输入: @overlay(imageVar, pointsVar)
    ↓
ExpressionHelper 解析
    ↓
创建 WatchedImageOverlayOp
    ↓
问题：如何将 overlay 数据传递给 NativeImageView？
```

**关键问题**：
1. WatchedImageOverlayOp 是 WatchedImage，负责加载像素数据
2. NativeImageView 负责**渲染**图像
3. Overlay 数据需要在**渲染时**添加到 NativeImageView
4. 但是 WatchedImage 和 NativeImageView 在不同层级

## 正确的架构

### 方案：在 WatchListItem 层处理 Overlay

```
WatchListItem (C#)
    ├── Image (WatchedImage)
    ├── View (WatchListItemView)
    │       └── NativeImageView
    └── OverlayData (String expression)
    
当渲染时：
1. WatchListItem 检查是否是 @overlay 表达式
2. 如果是，解析出图像和 overlay 变量
3. 加载图像
4. 读取 overlay 数据
5. 调用 View.AddOverlayPoint/Line
```

## 实现步骤

### 步骤 1: 在 WatchListItem 中检测 overlay 表达式

```csharp
// WatchListItem.cs
private bool IsOverlayExpression()
{
    return ExpressionHelper.IsOverlayExpression(Expression);
}

private Tuple<string, string> ParseOverlayExpression()
{
    return ExpressionHelper.ParseOverlayExpression(Expression);
}
```

### 步骤 2: 添加 overlay 数据加载方法

```csharp
// WatchListItem.cs
public void LoadOverlayData()
{
    if (!IsOverlayExpression())
        return;
    
    var parts = ParseOverlayExpression();
    if (parts == null)
        return;
    
    string imageExpr = parts.Item1;
    string overlayExpr = parts.Item2;
    
    // Read overlay data from debugger
    var overlayData = ReadOverlayFromDebugger(overlayExpr);
    
    // Apply to view
    if (View != null && overlayData != null)
    {
        View.ClearOverlay();
        foreach (var point in overlayData.Points)
        {
            View.AddOverlayPoint(point.X, point.Y, point.Color, point.Radius, 1.0f);
        }
    }
}
```

### 步骤 3: 在 WatchListItemView 中添加公共方法

```csharp
// WatchListItemView.cs (已完成)
public void AddOverlayPoint(float x, float y, Color color, float radius, float lineWidth)
{
    if (view_ != null)
    {
        view_.AddPoint(x, y, color, radius, lineWidth);
    }
}
```

### 步骤 4: 在更新流程中调用

```csharp
// WatchListItem.cs - UpdatePixels()
private void UpdatePixels()
{
    if (ArePixelsOutOfDate && detailLevel > 1)
    {
        Image.ReloadPixels();
        
        // ★ 如果是 overlay 表达式，加载 overlay 数据
        if (IsOverlayExpression())
        {
            LoadOverlayData();
        }
    }
}
```

## 代码修改

我现在按照这个架构实现。
