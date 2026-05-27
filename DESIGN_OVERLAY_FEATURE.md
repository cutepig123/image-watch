# Design Document: Overlay Graphics Feature (Points, Lines, Arcs)

## Overview

This document describes the design for adding overlay graphics support to Image Watch, enabling users to visualize geometric primitives (points, line segments, and arcs) on top of images during debugging.

## Current Architecture Analysis

### Rendering Pipeline

The current rendering pipeline consists of:

1. **WatchedImage** (C++/CLI) - Abstract base class for image data sources
   - Location: `ImageWatchNativeHelpers/WatchedImage.h`
   - Provides `WatchedImageInfo` containing width, height, pixel format
   - Manages pixel loading and info retrieval

2. **NativeImageBase** (C++/CLI) - Base for native image containers
   - Location: `vtiview/NativeImage/NativeImageBase.h`
   - Provides pixel reading, min/max computation
   - Wraps `vt::IImageReaderWriter`

3. **NativeImageView** (C++/CLI) - Renders images with transforms
   - Location: `vtiview/NativeImage/NativeImageView.h`
   - Handles zoom/pan via `BitmapRef`, `SourceRef`, `Scale` properties
   - Uses `ColorMap` to convert pixels to RGBA
   - Outputs `InteropBitmap` for WPF display

4. **WatchListItemView** (C#) - C# wrapper for NativeImageView
   - Location: `ImageWatch/Interface/WatchListItemView.cs`
   - Manages transform state (zoom, pan)
   - Applies colormap settings

5. **ImageViewer** (WPF UserControl) - Final UI display
   - Location: `ImageWatch/Interface/ImageViewer.xaml.cs`
   - Displays `InteropBitmap` in canvas
   - Handles mouse interactions (pan, zoom, pixel info)

### Key Observations

- The rendering happens in native C++ (`NativeImageView::Render()`)
- The output is a single `InteropBitmap` displayed in WPF
- There's no current mechanism for overlay graphics
- Transform state (zoom/pan) is managed at multiple levels

## Proposed Architecture

### Design Option 1: Native C++ Overlay Rendering (Recommended)

Add overlay rendering capability directly in `NativeImageView` for best performance and seamless integration.

#### Advantages
- Minimal performance overhead (renders directly to output bitmap)
- Consistent transform handling (same zoom/pan for overlay and image)
- No additional WPF complexity
- Follows existing architecture patterns

#### Implementation Steps

### 1. Create Overlay Primitive Types (C++)

**New file: `vtiview/NativeImage/OverlayPrimitives.h`**

```cpp
#pragma once

#include <vector>
#include <vtcore.h>

BEGIN_NI_NAMESPACE;

// Overlay primitive types
enum OverlayPrimitiveType
{
    OverlayPoint,
    OverlayLine,
    OverlayArc
};

// Base class for overlay primitives
struct OverlayPrimitive
{
    OverlayPrimitiveType type;
    vt::CRGBAPix color;
    float lineWidth;
    bool visible;
    
    OverlayPrimitive(OverlayPrimitiveType t) 
        : type(t), color(255, 0, 0, 255), lineWidth(1.0f), visible(true) {}
};

// Point primitive
struct OverlayPoint : OverlayPrimitive
{
    float x, y;       // Image coordinates
    float radius;     // Point size in pixels (at scale=1)
    
    OverlayPoint() : OverlayPrimitive(OverlayPoint), x(0), y(0), radius(3.0f) {}
};

// Line segment primitive
struct OverlayLine : OverlayPrimitive
{
    float x0, y0;     // Start point (image coordinates)
    float x1, y1;     // End point (image coordinates)
    
    OverlayLine() : OverlayPrimitive(OverlayLine), x0(0), y0(0), x1(0), y1(0) {}
};

// Arc primitive
struct OverlayArc : OverlayPrimitive
{
    float cx, cy;     // Center (image coordinates)
    float radius;     // Arc radius in pixels
    float startAngle; // Start angle in radians
    float endAngle;   // End angle in radians
    
    OverlayArc() : OverlayPrimitive(OverlayArc), cx(0), cy(0), 
                   radius(10.0f), startAngle(0), endAngle(VT_PI) {}
};

// Container for all overlay primitives
class OverlayGraphics
{
public:
    std::vector<OverlayPoint> points;
    std::vector<OverlayLine> lines;
    std::vector<OverlayArc> arcs;
    
    void Clear();
    void Render(vt::CRGBAByteImg& dst, const vt::CMtx3x3<float>& transform, float scale);
};

END_NI_NAMESPACE;
```

### 2. Implement Overlay Rendering (C++)

**New file: `vtiview/NativeImage/OverlayPrimitives.cpp`**

```cpp
#include "OverlayPrimitives.h"
#include <cmath>

BEGIN_NI_NAMESPACE;

void OverlayGraphics::Clear()
{
    points.clear();
    lines.clear();
    arcs.clear();
}

void OverlayGraphics::Render(vt::CRGBAByteImg& dst, 
                              const vt::CMtx3x3<float>& transform, 
                              float scale)
{
    // Render lines
    for (const auto& line : lines)
    {
        if (!line.visible) continue;
        
        // Transform coordinates
        vt::CVec2f p0(line.x0, line.y0);
        vt::CVec2f p1(line.x1, line.y1);
        
        // Apply transform matrix
        vt::CVec2f tp0 = transform * p0;
        vt::CVec2f tp1 = transform * p1;
        
        // Draw line on dst bitmap
        // Use Bresenham or anti-aliased line drawing
        DrawLine(dst, tp0.x, tp0.y, tp1.x, tp1.y, 
                 line.color, line.lineWidth * scale);
    }
    
    // Render points
    for (const auto& pt : points)
    {
        if (!pt.visible) continue;
        
        vt::CVec2f p(pt.x, pt.y);
        vt::CVec2f tp = transform * p;
        
        DrawCircle(dst, tp.x, tp.y, pt.radius * scale, 
                   pt.color, pt.lineWidth * scale);
    }
    
    // Render arcs
    for (const auto& arc : arcs)
    {
        if (!arc.visible) continue;
        
        vt::CVec2f c(arc.cx, arc.cy);
        vt::CVec2f tc = transform * c;
        
        DrawArc(dst, tc.x, tc.y, arc.radius * scale,
                arc.startAngle, arc.endAngle,
                arc.color, arc.lineWidth * scale);
    }
}

// Helper drawing functions
static void DrawLine(vt::CRGBAByteImg& dst, float x0, float y0, 
                     float x1, float y1, const vt::CRGBAPix& color, float width)
{
    // Anti-aliased line drawing implementation
    // Use Xiaolin Wu's algorithm or similar
    // Clamp coordinates to image bounds
}

static void DrawCircle(vt::CRGBAByteImg& dst, float cx, float cy, 
                       float radius, const vt::CRGBAPix& color, float width)
{
    // Draw filled circle or ring based on width
}

static void DrawArc(vt::CRGBAByteImg& dst, float cx, float cy, 
                    float radius, float startAngle, float endAngle,
                    const vt::CRGBAPix& color, float width)
{
    // Draw arc segment
}

END_NI_NAMESPACE;
```

### 3. Extend NativeImageView

**Modify: `vtiview/NativeImage/NativeImageView.h`**

Add overlay member:

```cpp
private:
    OverlayGraphics overlayGraphics_;
    
public:
    void AddOverlayPoint(float x, float y, vt::Byte r, vt::Byte g, 
                         vt::Byte b, vt::Byte a, float radius, float lineWidth);
    void AddOverlayLine(float x0, float y0, float x1, float y1, 
                        vt::Byte r, vt::Byte g, vt::Byte b, vt::Byte a, 
                        float lineWidth);
    void AddOverlayArc(float cx, float cy, float radius, 
                       float startAngle, float endAngle,
                       vt::Byte r, vt::Byte g, vt::Byte b, vt::Byte a, 
                       float lineWidth);
    void ClearOverlay();
```

**Modify: `vtiview/NativeImage/NativeImageView.cpp`**

In `Render()` method, after color mapping:

```cpp
void NativeImageView::Render()
{
    // ... existing rendering code ...
    
    // Render overlay graphics
    if (!overlayGraphics_.points.empty() || 
        !overlayGraphics_.lines.empty() ||
        !overlayGraphics_.arcs.empty())
    {
        vt::CMtx3x3<float> transform;
        ComputeWarpMatrix(transform);
        overlayGraphics_.Render(*view_, transform, scale_);
    }
}
```

### 4. Create Managed Wrapper (C++/CLI)

**Modify: `vtiview/NativeImage/NativeImageView.h`** (managed section)

```cpp
public:
    void AddPoint(float x, float y, Color color, float radius, float lineWidth);
    void AddLine(float x0, float y0, float x1, float y1, Color color, float lineWidth);
    void AddArc(float cx, float cy, float radius, float startAngle, float endAngle, 
                Color color, float lineWidth);
    void ClearOverlay();
```

### 5. Create WatchedImage Overlay Operator

**New files: `ImageWatchNativeHelpers/WatchedImageOverlayOp.h/.cpp`**

This allows users to add overlay via expression syntax:

```
@overlay(img, points, lines, arcs)
```

Where points, lines, arcs are arrays in the debugged program.

### 6. Create Overlay Data Provider Pattern

Allow users to define overlay data in their C++ code:

```cpp
// User's debugged code
struct DebugOverlayData
{
    std::vector<Point2f> points;
    std::vector<std::pair<Point2f, Point2f>> lines;
    std::vector<ArcData> arcs;
};

DebugOverlayData overlay_;
```

The extension would read this structure and render overlay on the associated image.

### 7. UI Integration (Optional)

Add overlay controls to ImageViewer context menu:

- "Show Overlay" checkbox
- "Overlay Color" picker
- "Overlay Line Width" slider

**Modify: `ImageWatch/Interface/ImageViewer.xaml`**

Add context menu items for overlay settings.

## Alternative Design Options

### Option 2: WPF Overlay Canvas

Add a separate WPF canvas layer on top of the image bitmap.

**Advantages:**
- Pure C# implementation
- Easy to interact with overlay elements (selection, hover)
- Can use WPF built-in shapes (Ellipse, Line, Path)

**Disadvantages:**
- Transform synchronization complexity
- Performance overhead for many primitives
- Less consistent with existing architecture

**Implementation:** Add `Canvas` in `ImageViewer.xaml` above the image, render WPF shapes with transform binding.

### Option 3: Natvis-Based Overlay Definition

Define overlay data structures via natvis files, similar to how user-defined image types work.

**Advantages:**
- No code changes required in debugged program
- Consistent with existing user type system
- User customizable

**Disadvantages:**
- Limited to static overlay definitions
- More complex natvis authoring

## Implementation Priorities

1. **Phase 1: Core Rendering** (Native C++ overlay primitives)
   - Implement `OverlayPrimitives.h/.cpp`
   - Extend `NativeImageView`
   - Add managed wrapper

2. **Phase 2: Data Binding** (Read overlay from debugged program)
   - Create `WatchedImageOverlayOp`
   - Parse overlay data structures from debugger
   - Support standard types (cv::Point, std::vector)

3. **Phase 3: UI Controls** (Optional)
   - Add overlay context menu
   - Overlay settings persistence
   - Color/style customization

4. **Phase 4: Advanced Features** (Future)
   - Interactive overlay editing
   - Overlay export/save
   - Multiple overlay layers

## Testing Strategy

1. Unit tests for overlay rendering (`OverlayPrimitives.cpp`)
2. Manual testing with test applications (`Test/VTApp1`)
3. Integration tests with various image types
4. Performance benchmarks for large overlay sets

## Dependencies

- VisionTools drawing primitives (if available)
- WPF InteropBitmap rendering
- Debugger expression evaluation API

## Risk Analysis

| Risk | Impact | Mitigation |
|------|--------|------------|
| Performance degradation with many primitives | Medium | Limit primitive count, use efficient algorithms |
| Transform sync issues | Medium | Use same transform matrix as image |
| Memory overhead | Low | Clear overlay after rendering |
| User data parsing complexity | Medium | Support standard types first, then custom |

## Estimated Effort

| Phase | Effort (hours) |
|-------|----------------|
| Phase 1: Core Rendering | 40-60 |
| Phase 2: Data Binding | 30-40 |
| Phase 3: UI Controls | 20-30 |
| Phase 4: Advanced | TBD |

Total Phase 1-3: ~90-130 hours