# Design Document: Overlay Graphics Feature (Points, Lines, Arcs)

## Overview

This document describes the design for adding overlay graphics support to Image Watch, enabling users to visualize geometric primitives (points, line segments, and arcs) on top of images during debugging.

## Current Architecture Analysis

### Image Type System

Image Watch uses a **natvis-based type discovery system** to support various image types:

1. **Built-in Types** - Hardcoded in `WatchedImageTypeMap.cs`:
   - `vt::CImgInCache`, `vt::CTypedImgInCache` (VisionTools images)
   - `ID3D11Resource`, `ID3D11Texture2D` (DirectX textures)

2. **Natvis-Based User Types** - Loaded from `.natvis` files:
   - OpenCV: `cv::Mat`, `CvMat`, `IplImage` (`ImageWatchOpenCV.natvis`)
   - Eigen: Eigen matrix types (`ImageWatchEigen.natvis`)
   - Unreal Engine: Various texture types (`ImageWatchUnrealEngine.natvis`)
   - Custom user types from `%USERPROFILE%\Documents\Visual Studio 2022\Visualizers\`

3. **How Natvis Works**:
   - Natvis files define `<Type>` elements with `<UIVisualizer>` tags
   - The `<Expand>` section specifies how to extract image metadata:
     - `[type]` - Pixel type (UINT8, FLOAT32, etc.)
     - `[channels]` - Number of color channels
     - `[width]`, `[height]` - Image dimensions
     - `[data]` - Pointer to pixel data
     - `[stride]` - Bytes per row
   - `UserTypeLoader.cs` parses natvis files and registers types
   - When user adds a variable to watch, the type is matched and metadata extracted

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
- **Natvis is the primary mechanism for supporting new data types**

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

### Option 3: Natvis-Based Overlay Definition (RECOMMENDED for Data Binding)

Define overlay data structures via natvis files, following the same pattern as image types. This is **the preferred approach** for Phase 2 because it follows the existing architecture.

#### How It Works

1. **User defines overlay data structure in C++ code:**

```cpp
// User's debugged code
struct OverlayPoint2D
{
    float x, y;
};

struct OverlayLine2D
{
    float x0, y0, x1, y1;
};

struct DebugOverlay
{
    std::vector<OverlayPoint2D> points;
    std::vector<OverlayLine2D> lines;
    // Color and style can be included
    unsigned char r, g, b, a;
};
```

2. **Create natvis file to describe the overlay:**

**New file: `ImageWatchOverlay.natvis`**

```xml
<?xml version="1.0" encoding="utf-8"?>
<AutoVisualizer xmlns="http://schemas.microsoft.com/vstudio/debugger/natvis/2010">
  
  <!-- Overlay graphics type -->
  <Type Name="DebugOverlay">
    <UIVisualizer ServiceId="{A452AFEA-3DF6-46BB-9177-C0B08F318025}" Id="2" />
  </Type>
  
  <Type Name="DebugOverlay">
    <Expand>
      <!-- Points array -->
      <Item Name="[points]">points</Item>
      <!-- Lines array -->
      <Item Name="[lines]">lines</Item>
      <!-- Color -->
      <Item Name="[color_r]">r</Item>
      <Item Name="[color_g]">g</Item>
      <Item Name="[color_b]">b</Item>
      <Item Name="[color_a]">a</Item>
    </Expand>
  </Type>
  
  <!-- Point type -->
  <Type Name="OverlayPoint2D">
    <Expand>
      <Item Name="[x]">x</Item>
      <Item Name="[y]">y</Item>
    </Expand>
  </Type>
  
  <!-- Line type -->
  <Type Name="OverlayLine2D">
    <Expand>
      <Item Name="[x0]">x0</Item>
      <Item Name="[y0]">y0</Item>
      <Item Name="[x1]">x1</Item>
      <Item Name="[y1]">y1</Item>
    </Expand>
  </Type>
  
</AutoVisualizer>
```

3. **Extend UserTypeLoader to support overlay types:**

Modify `UserTypeLoader.cs` to:
- Recognize overlay types (ServiceId Id="2")
- Extract overlay-specific metadata ([points], [lines], [color_*])
- Register overlay types separately from image types

4. **Create WatchedImageOverlayOp operator:**

**New files: `ImageWatchNativeHelpers/WatchedImageOverlayOp.h/.cpp`**

This allows combining an image with overlay:

```
@overlay(myImage, myOverlay)
```

The operator:
- Takes an image and overlay data structure as arguments
- Reads overlay data from debugger using the natvis metadata
- Populates the `OverlayGraphics` object in `NativeImageView`
- Renders image + overlay together

**Advantages:**
- **Follows existing architecture** - Uses the same natvis-based type discovery
- **No code changes in debugged program** - Just add natvis file
- **User customizable** - Users can define their own overlay structures
- **Consistent with image type system** - Same parsing and registration mechanism

**Disadvantages:**
- Requires natvis file authoring (but users are already familiar with this for custom image types)
- Limited to what can be expressed in natvis (but this is sufficient for most use cases)

## Implementation Priorities

### Phase 1: Core Rendering (Native C++ overlay primitives)
**Goal:** Add rendering capability for overlay graphics

**Tasks:**
1. Implement `OverlayPrimitives.h/.cpp` - Primitive types and rendering
2. Extend `NativeImageView` - Add overlay storage and rendering
3. Add managed wrapper (C++/CLI) - Expose overlay methods to C#
4. Unit tests for rendering functions

**Deliverable:** Can programmatically add and render overlay graphics

**Effort:** 40-60 hours

### Phase 2: Data Binding (Natvis-based overlay definition)
**Goal:** Allow users to define overlay data in their code and have it automatically recognized

**Tasks:**
1. Create `ImageWatchOverlay.natvis` - Example overlay natvis file
2. Extend `UserTypeLoader.cs` - Parse overlay types from natvis
3. Create `WatchedImageOverlayOp` - Operator to combine image + overlay
4. Integrate with expression parser - Support `@overlay(img, overlay)` syntax
5. Test with standard types (cv::Point, std::vector)
6. Documentation and examples

**Deliverable:** Users can define overlay structures in C++, add natvis file, and use `@overlay()` to display

**Effort:** 30-40 hours

### Phase 3: UI Controls (Optional enhancement)
**Goal:** Provide UI for overlay customization without code changes

**Tasks:**
1. Add overlay context menu in `ImageViewer.xaml`
   - "Show Overlay" toggle
   - "Overlay Color" picker
   - "Line Width" slider
   - "Point Size" slider
2. Overlay settings persistence (save preferences)
3. Per-image overlay settings

**Deliverable:** Users can customize overlay appearance through UI

**Effort:** 20-30 hours

**Note:** This phase is optional because:
- Phase 2 already allows full customization through C++ code and natvis
- UI controls add convenience but aren't essential
- Can be added later based on user feedback

### Phase 4: Advanced Features (Future)
**Goal:** Enhanced overlay functionality

**Tasks:**
1. Interactive overlay editing (click to add/move points)
2. Overlay export/save (save annotations to file)
3. Multiple overlay layers (group overlays)
4. Overlay templates (predefined styles)

**Effort:** TBD

## Testing Strategy

1. **Unit tests** for overlay rendering (`OverlayPrimitives.cpp`)
   - Test point, line, arc rendering
   - Test coordinate transformation
   - Test clipping and bounds

2. **Integration tests** with natvis overlay types
   - Test with `std::vector<cv::Point2f>`
   - Test with custom overlay structures
   - Test `@overlay()` operator

3. **Manual testing** with test applications
   - `Test/VTApp1` - Add overlay test cases
   - Test with OpenCV types
   - Test with large overlay sets

4. **Performance benchmarks**
   - Measure rendering time with 100, 1000, 10000 primitives
   - Compare with/without overlay

## Dependencies

- VisionTools drawing primitives (if available)
- WPF InteropBitmap rendering
- Debugger expression evaluation API
- Natvis parsing infrastructure (already exists)

## Risk Analysis

| Risk | Impact | Mitigation |
|------|--------|------------|
| Performance degradation with many primitives | Medium | Limit primitive count, use efficient algorithms, add culling |
| Transform sync issues | Medium | Use same transform matrix as image |
| Memory overhead | Low | Clear overlay after rendering |
| Natvis parsing complexity | Low | Reuse existing UserTypeLoader patterns |
| User adoption | Low | Provide good examples and documentation |

## Estimated Effort

| Phase | Effort (hours) | Priority |
| |-------|----------------|----------|
| Phase 1: Core Rendering | 40-60 | **Required** |
| Phase 2: Data Binding (Natvis) | 30-40 | **Required** |
| Phase 3: UI Controls | 20-30 | Optional |
| Phase 4: Advanced | TBD | Future |

**Total Phase 1-2 (MVP):** ~70-100 hours  
**Total Phase 1-3 (Full feature):** ~90-130 hours

## Example Usage

After implementation, users would:

1. **Define overlay in C++ code:**

```cpp
std::vector<cv::Point2f> featurePoints;
std::vector<std::pair<cv::Point2f, cv::Point2f>> edges;

// ... compute features ...
featurePoints.push_back(cv::Point2f(100, 200));
edges.push_back({cv::Point2f(50, 50), cv::Point2f(150, 150)});
```

2. **Add natvis file** (if using custom types):

Copy `ImageWatchOverlay.natvis` to `%USERPROFILE%\Documents\Visual Studio 2022\Visualizers\`

3. **Use in Image Watch:**

```
@overlay(myImage, featurePoints)
```

Or with custom overlay structure:

```
@overlay(myImage, myDebugOverlay)
```

4. **Result:** Image displayed with overlay graphics rendered on top, zooming/panning works for both image and overlay.
