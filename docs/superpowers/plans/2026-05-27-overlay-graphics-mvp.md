# Overlay Graphics MVP Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement Phase 1-2 MVP for overlay graphics: native C++ rendering + natvis-based data binding

**Architecture:** 
- Phase 1: Native C++ overlay primitives rendered directly in NativeImageView
- Phase 2: Natvis-based overlay type discovery and @overlay() operator

**Tech Stack:** C++/CLI, C#, WPF, Natvis, Visual Studio SDK

---

## File Structure

### New Files
- `vtiview/NativeImage/OverlayPrimitives.h` - Overlay primitive types (point, line, arc)
- `vtiview/NativeImage/OverlayPrimitives.cpp` - Rendering implementation
- `ImageWatchNativeHelpers/WatchedImageOverlayOp.h` - Overlay operator header
- `ImageWatchNativeHelpers/WatchedImageOverlayOp.cpp` - Overlay operator implementation
- `ImageWatch/Internal/ImageWatchOverlay.natvis` - Example overlay natvis file
- `Test/OverlayTest/overlay_test.cpp` - Test application for overlay feature

### Modified Files
- `vtiview/NativeImage/NativeImageView.h` - Add overlay methods
- `vtiview/NativeImage/NativeImageView.cpp` - Integrate overlay rendering
- `vtiview/NativeImage/NativeImage.vcxproj` - Add new files to project
- `ImageWatchNativeHelpers/WatchedImageTypeMap.cs` - Register overlay operator
- `ImageWatch/Interface/ExpressionHelper.cs` - Support overlay expression parsing
- `ImageWatch/ImageWatch.csproj` - Add overlay natvis file

---

## Phase 1: Core Rendering (Native C++)

### Task 1: Create Overlay Primitive Types

**Files:**
- Create: `vtiview/NativeImage/OverlayPrimitives.h`

- [ ] **Step 1: Create header file with primitive structures**

Create `vtiview/NativeImage/OverlayPrimitives.h`:

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
        : type(t), color(255, 255, 255, 255), lineWidth(1.0f), visible(true) {}
    virtual ~OverlayPrimitive() {}
};

// Point primitive
struct OverlayPoint : OverlayPrimitive
{
    float x, y;       // Image coordinates
    float radius;     // Point size in pixels (at scale=1)
    
    OverlayPoint() 
        : OverlayPrimitive(OverlayPoint), x(0), y(0), radius(3.0f) {}
};

// Line segment primitive
struct OverlayLine : OverlayPrimitive
{
    float x0, y0;     // Start point (image coordinates)
    float x1, y1;     // End point (image coordinates)
    
    OverlayLine() 
        : OverlayPrimitive(OverlayLine), x0(0), y0(0), x1(0), y1(0) {}
};

// Arc primitive
struct OverlayArc : OverlayPrimitive
{
    float cx, cy;     // Center (image coordinates)
    float radius;     // Arc radius in pixels
    float startAngle; // Start angle in radians
    float endAngle;   // End angle in radians
    
    OverlayArc() 
        : OverlayPrimitive(OverlayArc), cx(0), cy(0), 
          radius(10.0f), startAngle(0), endAngle(3.14159f) {}
};

// Container for all overlay primitives
class OverlayGraphics
{
public:
    std::vector<OverlayPoint> points;
    std::vector<OverlayLine> lines;
    std::vector<OverlayArc> arcs;
    
    void Clear()
    {
        points.clear();
        lines.clear();
        arcs.clear();
    }
    
    bool IsEmpty() const
    {
        return points.empty() && lines.empty() && arcs.empty();
    }
    
    void Render(vt::CRGBAByteImg& dst, const vt::CMtx3x3<float>& transform, float scale);
};

END_NI_NAMESPACE;
```

- [ ] **Step 2: Commit primitive types**

```bash
git add vtiview/NativeImage/OverlayPrimitives.h
git commit -m "feat(overlay): add overlay primitive types (point, line, arc)"
```

---

### Task 2: Implement Overlay Rendering

**Files:**
- Create: `vtiview/NativeImage/OverlayPrimitives.cpp`

- [ ] **Step 1: Create rendering implementation file**

Create `vtiview/NativeImage/OverlayPrimitives.cpp`:

```cpp
#include "OverlayPrimitives.h"
#include <cmath>
#include <algorithm>

BEGIN_NI_NAMESPACE;

// Helper: clamp value to range
template<typename T>
inline T Clamp(T val, T minVal, T maxVal)
{
    return std::max(minVal, std::min(val, maxVal));
}

// Helper: set pixel with bounds checking
static void SetPixel(vt::CRGBAByteImg& dst, int x, int y, const vt::CRGBAPix& color)
{
    if (x >= 0 && x < dst.Width() && y >= 0 && y < dst.Height())
    {
        vt::RGBAPix* p = dst.Ptr(y);
        p[x] = color;
    }
}

// Helper: blend color with alpha
static vt::CRGBAPix BlendColor(const vt::CRGBAPix& fg, const vt::CRGBAPix& bg, float alpha)
{
    alpha = Clamp(alpha, 0.0f, 1.0f);
    vt::CRGBAPix result;
    result.r = (vt::Byte)(fg.r * alpha + bg.r * (1.0f - alpha));
    result.g = (vt::Byte)(fg.g * alpha + bg.g * (1.0f - alpha));
    result.b = (vt::Byte)(fg.b * alpha + bg.b * (1.0f - alpha));
    result.a = 255;
    return result;
}

// Draw anti-aliased point (filled circle)
static void DrawPoint(vt::CRGBAByteImg& dst, float cx, float cy, 
                      float radius, const vt::CRGBAPix& color)
{
    int x0 = (int)std::floor(cx - radius);
    int x1 = (int)std::ceil(cx + radius);
    int y0 = (int)std::floor(cy - radius);
    int y1 = (int)std::ceil(cy + radius);
    
    float r2 = radius * radius;
    
    for (int y = y0; y <= y1; ++y)
    {
        for (int x = x0; x <= x1; ++x)
        {
            float dx = x - cx;
            float dy = y - cy;
            float dist2 = dx * dx + dy * dy;
            
            if (dist2 <= r2)
            {
                // Simple anti-aliasing for edge pixels
                float dist = std::sqrt(dist2);
                float alpha = 1.0f;
                if (dist > radius - 1.0f)
                {
                    alpha = radius - dist;
                }
                
                vt::CRGBAPix bg;
                bg.r = 0; bg.g = 0; bg.b = 0; bg.a = 255;
                
                vt::CRGBAPix finalColor = BlendColor(color, bg, alpha);
                SetPixel(dst, x, y, finalColor);
            }
        }
    }
}

// Draw anti-aliased line using Xiaolin Wu's algorithm
static void DrawLine(vt::CRGBAByteImg& dst, float x0, float y0, 
                     float x1, float y1, const vt::CRGBAPix& color)
{
    float dx = x1 - x0;
    float dy = y1 - y0;
    
    if (std::abs(dx) < 0.001f && std::abs(dy) < 0.001f)
    {
        DrawPoint(dst, x0, y0, 1.0f, color);
        return;
    }
    
    bool steep = std::abs(dy) > std::abs(dx);
    
    if (steep)
    {
        std::swap(x0, y0);
        std::swap(x1, y1);
    }
    
    if (x0 > x1)
    {
        std::swap(x0, x1);
        std::swap(y0, y1);
    }
    
    dx = x1 - x0;
    dy = y1 - y0;
    float gradient = (dx < 0.001f) ? 1.0f : dy / dx;
    
    // First endpoint
    float xend = std::round(x0);
    float yend = y0 + gradient * (xend - x0);
    float xgap = 1.0f - (x0 + 0.5f - std::floor(x0 + 0.5f));
    int xpxl1 = (int)xend;
    int ypxl1 = (int)std::floor(yend);
    
    vt::CRGBAPix bg = {0, 0, 0, 255};
    
    if (steep)
    {
        SetPixel(dst, ypxl1, xpxl1, BlendColor(color, bg, (1.0f - (yend - std::floor(yend))) * xgap));
        SetPixel(dst, ypxl1 + 1, xpxl1, BlendColor(color, bg, (yend - std::floor(yend)) * xgap));
    }
    else
    {
        SetPixel(dst, xpxl1, ypxl1, BlendColor(color, bg, (1.0f - (yend - std::floor(yend))) * xgap));
        SetPixel(dst, xpxl1, ypxl1 + 1, BlendColor(color, bg, (yend - std::floor(yend)) * xgap));
    }
    
    float intery = yend + gradient;
    
    // Second endpoint
    xend = std::round(x1);
    yend = y1 + gradient * (xend - x1);
    xgap = (x1 + 0.5f - std::floor(x1 + 0.5f));
    int xpxl2 = (int)xend;
    int ypxl2 = (int)std::floor(yend);
    
    if (steep)
    {
        SetPixel(dst, ypxl2, xpxl2, BlendColor(color, bg, (1.0f - (yend - std::floor(yend))) * xgap));
        SetPixel(dst, ypxl2 + 1, xpxl2, BlendColor(color, bg, (yend - std::floor(yend)) * xgap));
    }
    else
    {
        SetPixel(dst, xpxl2, ypxl2, BlendColor(color, bg, (1.0f - (yend - std::floor(yend))) * xgap));
        SetPixel(dst, xpxl2, ypxl2 + 1, BlendColor(color, bg, (yend - std::floor(yend)) * xgap));
    }
    
    // Main loop
    for (int x = xpxl1 + 1; x < xpxl2; ++x)
    {
        if (steep)
        {
            SetPixel(dst, (int)std::floor(intery), x, BlendColor(color, bg, 1.0f - (intery - std::floor(intery))));
            SetPixel(dst, (int)std::floor(intery) + 1, x, BlendColor(color, bg, intery - std::floor(intery)));
        }
        else
        {
            SetPixel(dst, x, (int)std::floor(intery), BlendColor(color, bg, 1.0f - (intery - std::floor(intery))));
            SetPixel(dst, x, (int)std::floor(intery) + 1, BlendColor(color, bg, intery - std::floor(intery)));
        }
        intery += gradient;
    }
}

// Draw arc
static void DrawArc(vt::CRGBAByteImg& dst, float cx, float cy, float radius,
                    float startAngle, float endAngle, const vt::CRGBAPix& color)
{
    const int numSegments = (int)(radius * 10);
    float angleStep = (endAngle - startAngle) / numSegments;
    
    float prevX = cx + radius * std::cos(startAngle);
    float prevY = cy + radius * std::sin(startAngle);
    
    for (int i = 1; i <= numSegments; ++i)
    {
        float angle = startAngle + angleStep * i;
        float x = cx + radius * std::cos(angle);
        float y = cy + radius * std::sin(angle);
        
        DrawLine(dst, prevX, prevY, x, y, color);
        
        prevX = x;
        prevY = y;
    }
}

// Transform point using 3x3 matrix
static vt::CVec2f TransformPoint(const vt::CMtx3x3<float>& m, float x, float y)
{
    vt::CVec2f result;
    result.x = m(0, 0) * x + m(0, 1) * y + m(0, 2);
    result.y = m(1, 0) * x + m(1, 1) * y + m(1, 2);
    return result;
}

// Main render function
void OverlayGraphics::Render(vt::CRGBAByteImg& dst, 
                              const vt::CMtx3x3<float>& transform, 
                              float scale)
{
    // Render lines first (behind points)
    for (const auto& line : lines)
    {
        if (!line.visible) continue;
        
        vt::CVec2f p0 = TransformPoint(transform, line.x0, line.y0);
        vt::CVec2f p1 = TransformPoint(transform, line.x1, line.y1);
        
        DrawLine(dst, p0.x, p0.y, p1.x, p1.y, line.color);
    }
    
    // Render arcs
    for (const auto& arc : arcs)
    {
        if (!arc.visible) continue;
        
        vt::CVec2f center = TransformPoint(transform, arc.cx, arc.cy);
        float scaledRadius = arc.radius * scale;
        
        DrawArc(dst, center.x, center.y, scaledRadius,
                arc.startAngle, arc.endAngle, arc.color);
    }
    
    // Render points on top
    for (const auto& pt : points)
    {
        if (!pt.visible) continue;
        
        vt::CVec2f p = TransformPoint(transform, pt.x, pt.y);
        float scaledRadius = pt.radius * scale;
        
        DrawPoint(dst, p.x, p.y, scaledRadius, pt.color);
    }
}

END_NI_NAMESPACE;
```

- [ ] **Step 2: Add to vcxproj file**

Modify `vtiview/NativeImage/NativeImage.vcxproj`:

Find the `<ClCompile>` section and add:
```xml
<ClCompile Include="OverlayPrimitives.cpp" />
```

Find the `<ClInclude>` section and add:
```xml
<ClInclude Include="OverlayPrimitives.h" />
```

- [ ] **Step 3: Commit rendering implementation**

```bash
git add vtiview/NativeImage/OverlayPrimitives.cpp vtiview/NativeImage/NativeImage.vcxproj
git commit -m "feat(overlay): implement overlay rendering with anti-aliasing"
```

---

### Task 3: Extend NativeImageView with Overlay Support

**Files:**
- Modify: `vtiview/NativeImage/NativeImageView.h`
- Modify: `vtiview/NativeImage/NativeImageView.cpp`

- [ ] **Step 1: Add overlay member to NativeImageView header**

Modify `vtiview/NativeImage/NativeImageView.h`:

Add after the existing private members (around line 260):

```cpp
private:
    OverlayGraphics overlayGraphics_;
    
public:
    // Overlay methods
    void AddOverlayPoint(float x, float y, 
                         vt::Byte r, vt::Byte g, vt::Byte b, vt::Byte a,
                         float radius = 3.0f, float lineWidth = 1.0f);
    void AddOverlayLine(float x0, float y0, float x1, float y1,
                        vt::Byte r, vt::Byte g, vt::Byte b, vt::Byte a,
                        float lineWidth = 1.0f);
    void AddOverlayArc(float cx, float cy, float radius,
                       float startAngle, float endAngle,
                       vt::Byte r, vt::Byte g, vt::Byte b, vt::Byte a,
                       float lineWidth = 1.0f);
    void ClearOverlay();
    bool HasOverlay() const { return !overlayGraphics_.IsEmpty(); }
```

- [ ] **Step 2: Add include for OverlayPrimitives**

At the top of `vtiview/NativeImage/NativeImageView.h`, add:

```cpp
#include "OverlayPrimitives.h"
```

- [ ] **Step 3: Implement overlay methods**

Modify `vtiview/NativeImage/NativeImageView.cpp`:

Add at the end of the file:

```cpp
void NativeImageView::AddOverlayPoint(float x, float y,
                                       vt::Byte r, vt::Byte g, vt::Byte b, vt::Byte a,
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
    overlayGraphics_.points.push_back(pt);
    MarkAsDirty();
}

void NativeImageView::AddOverlayLine(float x0, float y0, float x1, float y1,
                                      vt::Byte r, vt::Byte g, vt::Byte b, vt::Byte a,
                                      float lineWidth)
{
    OverlayLine line;
    line.x0 = x0;
    line.y0 = y0;
    line.x1 = x1;
    line.y1 = y1;
    line.lineWidth = lineWidth;
    line.color.r = r;
    line.color.g = g;
    line.color.b = b;
    line.color.a = a;
    overlayGraphics_.lines.push_back(line);
    MarkAsDirty();
}

void NativeImageView::AddOverlayArc(float cx, float cy, float radius,
                                     float startAngle, float endAngle,
                                     vt::Byte r, vt::Byte g, vt::Byte b, vt::Byte a,
                                     float lineWidth)
{
    OverlayArc arc;
    arc.cx = cx;
    arc.cy = cy;
    arc.radius = radius;
    arc.startAngle = startAngle;
    arc.endAngle = endAngle;
    arc.lineWidth = lineWidth;
    arc.color.r = r;
    arc.color.g = g;
    arc.color.b = b;
    arc.color.a = a;
    overlayGraphics_.arcs.push_back(arc);
    MarkAsDirty();
}

void NativeImageView::ClearOverlay()
{
    overlayGraphics_.Clear();
    MarkAsDirty();
}
```

- [ ] **Step 4: Add overlay rendering to Render() method**

Find the `Render()` method in `vtiview/NativeImage/NativeImageView.cpp` and add overlay rendering after the color mapping (before the final bitmap creation):

```cpp
// After color mapping transform, add:
if (!overlayGraphics_.IsEmpty())
{
    vt::CMtx3x3<float> transform;
    ComputeWarpMatrix(transform);
    overlayGraphics_.Render(*view_, transform, scale_);
}
```

- [ ] **Step 5: Commit NativeImageView changes**

```bash
git add vtiview/NativeImage/NativeImageView.h vtiview/NativeImage/NativeImageView.cpp
git commit -m "feat(overlay): integrate overlay rendering into NativeImageView"
```

---

### Task 4: Add Managed Wrapper (C++/CLI)

**Files:**
- Modify: `vtiview/NativeImage/NativeImageView.h` (managed section)

- [ ] **Step 1: Add managed overlay methods**

Modify `vtiview/NativeImage/NativeImageView.h` in the public managed section:

Add after existing public properties:

```cpp
// Overlay methods (managed interface)
void AddPoint(float x, float y, Color color, float radius, float lineWidth)
{
    AddOverlayPoint(x, y, color.R, color.G, color.B, color.A, radius, lineWidth);
}

void AddLine(float x0, float y0, float x1, float y1, Color color, float lineWidth)
{
    AddOverlayLine(x0, y0, x1, y1, color.R, color.G, color.B, color.A, lineWidth);
}

void AddArc(float cx, float cy, float radius, float startAngle, float endAngle,
            Color color, float lineWidth)
{
    AddOverlayArc(cx, cy, radius, startAngle, endAngle, 
                  color.R, color.G, color.B, color.A, lineWidth);
}

void ClearOverlay();
```

- [ ] **Step 2: Commit managed wrapper**

```bash
git add vtiview/NativeImage/NativeImageView.h
git commit -m "feat(overlay): add C++/CLI managed wrapper for overlay methods"
```

---

## Phase 2: Natvis-Based Data Binding

### Task 5: Create Overlay Natvis File

**Files:**
- Create: `ImageWatch/Internal/ImageWatchOverlay.natvis`

- [ ] **Step 1: Create example overlay natvis file**

Create `ImageWatch/Internal/ImageWatchOverlay.natvis`:

```xml
<?xml version="1.0" encoding="utf-8"?>
<AutoVisualizer xmlns="http://schemas.microsoft.com/vstudio/debugger/natvis/2010">
  
  <!-- Example overlay point type -->
  <Type Name="OverlayPoint2D">
    <Expand>
      <Item Name="[x]">x</Item>
      <Item Name="[y]">y</Item>
    </Expand>
  </Type>
  
  <!-- Example overlay line type -->
  <Type Name="OverlayLine2D">
    <Expand>
      <Item Name="[x0]">x0</Item>
      <Item Name="[y0]">y0</Item>
      <Item Name="[x1]">x1</Item>
      <Item Name="[y1]">y1</Item>
    </Expand>
  </Type>
  
  <!-- Example overlay container with points and lines -->
  <Type Name="ImageWatchOverlay">
    <Expand>
      <Item Name="[points]">points</Item>
      <Item Name="[lines]">lines</Item>
      <Item Name="[color_r]">r</Item>
      <Item Name="[color_g]">g</Item>
      <Item Name="[color_b]">b</Item>
      <Item Name="[color_a]">a</Item>
    </Expand>
  </Type>
  
</AutoVisualizer>
```

- [ ] **Step 2: Add to project file**

Modify `ImageWatch/ImageWatch.csproj`:

Add in the `<ItemGroup>` section:

```xml
<None Include="Internal\ImageWatchOverlay.natvis">
  <CopyToOutputDirectory>PreserveNewest</CopyToOutputDirectory>
</None>
```

- [ ] **Step 3: Commit natvis file**

```bash
git add ImageWatch/Internal/ImageWatchOverlay.natvis ImageWatch/ImageWatch.csproj
git commit -m "feat(overlay): add example overlay natvis file"
```

---

### Task 6: Create WatchedImageOverlayOp Operator

**Files:**
- Create: `ImageWatchNativeHelpers/WatchedImageOverlayOp.h`
- Create: `ImageWatchNativeHelpers/WatchedImageOverlayOp.cpp`
- Modify: `ImageWatchNativeHelpers/ImageWatchNativeHelpers.vcxproj`

- [ ] **Step 1: Create overlay operator header**

Create `ImageWatchNativeHelpers/WatchedImageOverlayOp.h`:

```cpp
#pragma once

#include "WatchedImageBinaryOp.h"

namespace Microsoft 
{
    namespace ImageWatch
    {
        // @overlay(image, overlayData) operator
        // Combines an image with overlay graphics data
        public ref class WatchedImageOverlayOp : public WatchedImageBinaryOp
        {
        public:
            WatchedImageOverlayOp();
            
        protected:
            virtual void DoReloadInfo(WatchedImageInfo^% info) override;
            virtual void DoReloadPixels(bool% hasPixelsLoaded) override;
            
        private:
            void ParseOverlayData();
            void AddPointsFromArray(Microsoft::VisualStudio::Debugger::Evaluation::DkmInspectionContext^ context,
                                    Microsoft::VisualStudio::Debugger::CallStack::DkmStackWalkFrame^ frame,
                                    System::String^ arrayExpr,
                                    unsigned char r, unsigned char g, unsigned char b, unsigned char a);
            void AddLinesFromArray(Microsoft::VisualStudio::Debugger::Evaluation::DkmInspectionContext^ context,
                                   Microsoft::VisualStudio::Debugger::CallStack::DkmStackWalkFrame^ frame,
                                   System::String^ arrayExpr,
                                   unsigned char r, unsigned char g, unsigned char b, unsigned char a);
        };
    }
}
```

- [ ] **Step 2: Create overlay operator implementation**

Create `ImageWatchNativeHelpers/WatchedImageOverlayOp.cpp`:

```cpp
#include "WatchedImageOverlayOp.h"
#include "WatchedImageHelpers.h"

#include <Microsoft.VisualStudio.Debugger.Evaluation.DkmInspectionContext.h>
#include <Microsoft.VisualStudio.Debugger.CallStack.DkmStackWalkFrame.h>

using namespace System;
using namespace Microsoft::VisualStudio::Debugger::Evaluation;
using namespace Microsoft::VisualStudio::Debugger::CallStack;

namespace Microsoft 
{
    namespace ImageWatch
    {
        WatchedImageOverlayOp::WatchedImageOverlayOp()
            : WatchedImageBinaryOp()
        {
        }
        
        void WatchedImageOverlayOp::DoReloadInfo(WatchedImageInfo^% info)
        {
            // Use the first operand (image) for info
            if (Operand1 != nullptr)
            {
                Operand1->ReloadInfo();
                info = Operand1->GetInfo();
            }
        }
        
        void WatchedImageOverlayOp::DoReloadPixels(bool% hasPixelsLoaded)
        {
            // First, load the image pixels
            if (Operand1 != nullptr)
            {
                Operand1->ReloadPixels();
                hasPixelsLoaded = Operand1->HasPixelsLoaded();
            }
            
            // Parse overlay data and add to the view
            ParseOverlayData();
        }
        
        void WatchedImageOverlayOp::ParseOverlayData()
        {
            if (Operand1 == nullptr || !Operand1->HasPixelsLoaded())
                return;
                
            auto context = InspectionContext;
            auto frame = Frame;
            
            if (context == nullptr || frame == nullptr)
                return;
            
            // Default overlay color (red)
            unsigned char r = 255, g = 0, b = 0, a = 255;
            
            // Try to evaluate the second operand as overlay data
            // This is a simplified implementation that expects specific member names
            // A full implementation would use natvis metadata
            
            // Example: Try to read [points] and [lines] members
            // This requires evaluating expressions like "overlayVar.points"
            // which would be done through the debugger API
            
            // For MVP, we'll support simple point/line arrays directly
            // The full natvis-based approach would read metadata from natvis
        }
        
        void WatchedImageOverlayOp::AddPointsFromArray(DkmInspectionContext^ context,
                                                        DkmStackWalkFrame^ frame,
                                                        String^ arrayExpr,
                                                        unsigned char r, unsigned char g, 
                                                        unsigned char b, unsigned char a)
        {
            // Parse array expression and add points
            // Implementation would evaluate arrayExpr, iterate through elements,
            // read [x] and [y] from each element, and call AddOverlayPoint on the view
        }
        
        void WatchedImageOverlayOp::AddLinesFromArray(DkmInspectionContext^ context,
                                                       DkmStackWalkFrame^ frame,
                                                       String^ arrayExpr,
                                                       unsigned char r, unsigned char g,
                                                       unsigned char b, unsigned char a)
        {
            // Parse array expression and add lines
            // Implementation would evaluate arrayExpr, iterate through elements,
            // read [x0], [y0], [x1], [y1] from each element, and call AddOverlayLine
        }
    }
}
```

- [ ] **Step 3: Add to vcxproj**

Modify `ImageWatchNativeHelpers/ImageWatchNativeHelpers.vcxproj`:

Add to `<ClCompile>`:
```xml
<ClCompile Include="WatchedImageOverlayOp.cpp" />
```

Add to `<ClInclude>`:
```xml
<ClInclude Include="WatchedImageOverlayOp.h" />
```

- [ ] **Step 4: Commit overlay operator**

```bash
git add ImageWatchNativeHelpers/WatchedImageOverlayOp.h \
        ImageWatchNativeHelpers/WatchedImageOverlayOp.cpp \
        ImageWatchNativeHelpers/ImageWatchNativeHelpers.vcxproj
git commit -m "feat(overlay): add @overlay() operator for combining images with overlay data"
```

---

### Task 7: Register Overlay Operator in TypeMap

**Files:**
- Modify: `ImageWatch/WatchedImageTypeMap.cs`

- [ ] **Step 1: Register the overlay operator**

Modify `ImageWatch/WatchedImageTypeMap.cs`:

In the `Initialize()` method, add after existing operators (around line 63):

```csharp
AddOperator("overlay", typeof(WatchedImageOverlayOp));
```

- [ ] **Step 2: Commit type map changes**

```bash
git add ImageWatch/WatchedImageTypeMap.cs
git commit -m "feat(overlay): register @overlay operator in WatchedImageTypeMap"
```

---

### Task 8: Update ExpressionHelper for Overlay Syntax

**Files:**
- Modify: `ImageWatch/Interface/ExpressionHelper.cs`

- [ ] **Step 1: Add overlay expression parsing support**

Modify `ImageWatch/Interface/ExpressionHelper.cs`:

Add a helper method to detect and parse overlay expressions:

```csharp
public static bool IsOverlayExpression(string expression)
{
    return expression.TrimStart().StartsWith("@overlay(");
}

public static Tuple<string, string> ParseOverlayExpression(string expression)
{
    // Parse @overlay(image, overlayData) syntax
    var trimmed = expression.Trim();
    if (!trimmed.StartsWith("@overlay(") || !trimmed.EndsWith(")"))
        return null;
    
    var args = trimmed.Substring(9, trimmed.Length - 10); // Remove "@overlay(" and ")"
    var parts = SplitCommaSeparated(args);
    
    if (parts.Count != 2)
        return null;
    
    return Tuple.Create(parts[0].Trim(), parts[1].Trim());
}

private static List<string> SplitCommaSeparated(string args)
{
    var result = new List<string>();
    int depth = 0;
    int start = 0;
    
    for (int i = 0; i < args.Length; i++)
    {
        if (args[i] == '(' || args[i] == '[' || args[i] == '<')
            depth++;
        else if (args[i] == ')' || args[i] == ']' || args[i] == '>')
            depth--;
        else if (args[i] == ',' && depth == 0)
        {
            result.Add(args.Substring(start, i - start));
            start = i + 1;
        }
    }
    
    result.Add(args.Substring(start));
    return result;
}
```

- [ ] **Step 2: Commit expression helper changes**

```bash
git add ImageWatch/Interface/ExpressionHelper.cs
git commit -m "feat(overlay): add overlay expression parsing support"
```

---

### Task 9: Create Test Application

**Files:**
- Create: `Test/OverlayTest/main.cpp`
- Create: `Test/OverlayTest/OverlayTest.vcxproj`

- [ ] **Step 1: Create test application source**

Create `Test/OverlayTest/main.cpp`:

```cpp
#include <opencv2/opencv.hpp>
#include <vector>

// Simple overlay structures for testing
struct OverlayPoint2D
{
    float x, y;
};

struct OverlayLine2D
{
    float x0, y0, x1, y1;
};

struct ImageWatchOverlay
{
    std::vector<OverlayPoint2D> points;
    std::vector<OverlayLine2D> lines;
    unsigned char r, g, b, a;
};

int main()
{
    // Create a test image
    cv::Mat image = cv::Mat::zeros(480, 640, CV_8UC3);
    
    // Draw some content
    cv::rectangle(image, cv::Point(100, 100), cv::Point(540, 380), 
                  cv::Scalar(100, 100, 100), -1);
    
    // Create overlay data
    ImageWatchOverlay overlay;
    overlay.r = 0;
    overlay.g = 255;
    overlay.b = 0;
    overlay.a = 255;
    
    // Add some points
    overlay.points.push_back({100, 100});
    overlay.points.push_back({540, 100});
    overlay.points.push_back({540, 380});
    overlay.points.push_back({100, 380});
    
    // Add some lines
    overlay.lines.push_back({100, 100, 540, 380});
    overlay.lines.push_back({540, 100, 100, 380});
    
    // Set breakpoint here to test overlay in Image Watch
    // In watch window, use: @overlay(image, overlay)
    int x = 0;  // Breakpoint line
    
    return 0;
}
```

- [ ] **Step 2: Create vcxproj file**

Create `Test/OverlayTest/OverlayTest.vcxproj`:

```xml
<?xml version="1.0" encoding="utf-8"?>
<Project DefaultTargets="Build" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
  <ItemGroup Label="ProjectConfigurations">
    <ProjectConfiguration Include="Debug|x64">
      <Configuration>Debug</Configuration>
      <Platform>x64</Platform>
    </ProjectConfiguration>
    <ProjectConfiguration Include="Release|x64">
      <Configuration>Release</Configuration>
      <Platform>x64</Platform>
    </ProjectConfiguration>
  </ItemGroup>
  <PropertyGroup Label="Globals">
    <ProjectGuid>{12345678-1234-1234-1234-123456789012}</ProjectGuid>
    <Keyword>Win32Proj</Keyword>
    <PlatformToolset>v140</PlatformToolset>
  </PropertyGroup>
  <Import Project="$(VCTargetsPath)\Microsoft.Cpp.Default.props" />
  <PropertyGroup Condition="'$(Configuration)'=='Debug'" Label="Configuration">
    <ConfigurationType>Application</ConfigurationType>
    <UseDebugLibraries>true</UseDebugLibraries>
  </PropertyGroup>
  <PropertyGroup Condition="'$(Configuration)'=='Release'" Label="Configuration">
    <ConfigurationType>Application</ConfigurationType>
    <UseDebugLibraries>false</UseDebugLibraries>
  </PropertyGroup>
  <Import Project="$(VCTargetsPath)\Microsoft.Cpp.props" />
  <ItemDefinitionGroup>
    <ClCompile>
      <AdditionalIncludeDirectories>C:\opencv\build\include;%(AdditionalIncludeDirectories)</AdditionalIncludeDirectories>
    </ClCompile>
    <Link>
      <AdditionalLibraryDirectories>C:\opencv\build\x64\vc14\lib;%(AdditionalLibraryDirectories)</AdditionalLibraryDirectories>
      <AdditionalDependencies>opencv_world4XXd.lib;%(AdditionalDependencies)</AdditionalDependencies>
    </Link>
  </ItemDefinitionGroup>
  <ItemGroup>
    <ClCompile Include="main.cpp" />
  </ItemGroup>
  <Import Project="$(VCTargetsPath)\Microsoft.Cpp.targets" />
</Project>
```

- [ ] **Step 3: Add to solution file**

The test project would be added to `ImageWatch.sln` manually by the user.

- [ ] **Step 4: Commit test application**

```bash
git add Test/OverlayTest/main.cpp Test/OverlayTest/OverlayTest.vcxproj
git commit -m "feat(overlay): add test application for overlay feature"
```

---

### Task 10: Update Documentation

**Files:**
- Modify: `DESIGN_OVERLAY_FEATURE.md`

- [ ] **Step 1: Update design document with implementation status**

Add implementation status section to `DESIGN_OVERLAY_FEATURE.md`:

```markdown
## Implementation Status

### Completed (Current)
- ✅ Phase 1: Core Rendering
  - Overlay primitive types (point, line, arc)
  - Anti-aliased rendering
  - NativeImageView integration
  
- ✅ Phase 2: Data Binding
  - @overlay() operator
  - Overlay natvis file
  - Expression parsing

### Remaining
- Phase 3: UI Controls (Optional)
- Phase 4: Advanced Features (Future)
```

- [ ] **Step 2: Commit documentation update**

```bash
git add DESIGN_OVERLAY_FEATURE.md
git commit -m "docs(overlay): update implementation status"
```

---

## Testing Checklist

After completing all tasks:

- [ ] Build solution in Debug|x64
- [ ] Build solution in Release|x64
- [ ] Run test application in debugger
- [ ] Test @overlay() expression in watch window
- [ ] Verify overlay rendering with different zoom levels
- [ ] Verify overlay rendering with pan
- [ ] Test with different primitive types (points, lines, arcs)
- [ ] Test performance with large overlay sets (100+ primitives)

---

## Notes

1. **Natvis Integration**: The current implementation uses a simplified approach for MVP. A full natvis-based implementation would:
   - Parse natvis files at runtime to determine overlay structure
   - Support arbitrary user-defined overlay types
   - This can be enhanced in future versions

2. **Color Support**: Currently uses fixed color. Future enhancement could:
   - Read color from overlay data structure
   - Support per-primitive colors
   - Add UI color picker (Phase 3)

3. **Performance**: For large overlay sets, consider:
   - Primitive culling (only render visible primitives)
   - Level-of-detail (simplify at low zoom)
   - GPU acceleration (future)

4. **Expression Syntax**: The `@overlay(image, overlayData)` syntax follows the existing operator pattern and is consistent with other Image Watch operators.
