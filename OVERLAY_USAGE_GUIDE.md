# Overlay Graphics Feature - Usage Guide

## Overview

The overlay graphics feature allows you to visualize geometric primitives (points, lines, arcs) on top of images during debugging in Visual Studio.

## Current Implementation Status

**✅ Phase 1: Core Rendering** - Complete
- Native overlay rendering in `NativeImageView`
- Anti-aliased point, line, and arc drawing
- Transform-aware rendering (zoom/pan support)

**🚧 Phase 2: Data Binding** - Partial Implementation
- `@overlay()` operator framework is in place
- Overlay data parsing from debugger requires additional work
- Currently, overlay must be added programmatically

## How It Works

### Architecture

```
User Code (C++)
    ↓
Debugger Expression (@overlay(img, data))
    ↓
WatchedImageOverlayOp
    ↓
NativeImageView::AddOverlayPoint/Line/Arc
    ↓
OverlayGraphics::Render()
    ↓
Display in Image Watch Window
```

## Programmatic Usage (MVP)

Since the debugger data binding is still in progress, you can currently add overlays programmatically through the C++ API:

### Step 1: Access NativeImageView

In your C++ code, after obtaining a `NativeImageView` object:

```cpp
using namespace Microsoft::Research::NativeImage;

// Assuming you have access to a NativeImageView object
NativeImageView^ view = ...;

// Add a red point at (100, 200) with radius 5
view->AddOverlayPoint(100.0f, 200.0f, 255, 0, 0, 255, 5.0f, 1.0f);

// Add a green line from (50, 50) to (150, 150)
view->AddOverlayLine(50.0f, 50.0f, 150.0f, 150.0f, 0, 255, 0, 255, 1.0f);

// Add a blue arc
view->AddOverlayArc(200.0f, 200.0f, 30.0f, 0.0f, 3.14159f, 0, 0, 255, 255, 1.0f);

// Clear all overlays
view->ClearOverlay();
```

### Step 2: Trigger Rendering

The overlay will be rendered automatically when the image is displayed or when `MarkAsDirty()` is called.

## Future Usage (Full Implementation)

Once Phase 2 is complete, you'll be able to use overlay directly in the debugger:

### Example 1: Feature Points

```cpp
// In your C++ code
#include <opencv2/opencv.hpp>
#include <vector>

std::vector<cv::Point2f> featurePoints;
featurePoints.push_back(cv::Point2f(100, 200));
featurePoints.push_back(cv::Point2f(150, 250));
featurePoints.push_back(cv::Point2f(200, 180));

cv::Mat image = cv::imread("test.jpg");
```

**In Image Watch window:**
```
@overlay(image, featurePoints)
```

### Example 2: Custom Overlay Structure

```cpp
// Define overlay data structure
struct DebugOverlay
{
    std::vector<cv::Point2f> points;
    std::vector<std::pair<cv::Point2f, cv::Point2f>> lines;
    unsigned char r, g, b, a;
};

DebugOverlay overlay;
overlay.r = 255; overlay.g = 0; overlay.b = 0; overlay.a = 255;
overlay.points = { {100, 100}, {200, 200} };
overlay.lines = { {{50, 50}, {250, 250}} };
```

**In Image Watch window:**
```
@overlay(myImage, overlay)
```

## Coordinate System

- **Image coordinates**: Pixels in the original image (0,0) = top-left corner
- **Transform-aware**: Overlay automatically adjusts when you zoom/pan the image
- **Scale-independent**: Point radius and line width scale with zoom level

## Supported Primitives

### 1. Point

```cpp
struct OverlayPoint {
    float x, y;          // Image coordinates (pixels)
    float radius;        // Point size (at scale=1)
    vt::CRGBAPix color;  // RGBA color (0-255)
    float lineWidth;     // Line width (for outline)
};
```

### 2. Line

```cpp
struct OverlayLine {
    float x0, y0;        // Start point (image coordinates)
    float x1, y1;        // End point (image coordinates)
    vt::CRGBAPix color;  // RGBA color
    float lineWidth;     // Line thickness
};
```

### 3. Arc

```cpp
struct OverlayArc {
    float cx, cy;            // Center (image coordinates)
    float radius;            // Arc radius (pixels)
    float startAngle;        // Start angle (radians)
    float endAngle;          // End angle (radians)
    vt::CRGBAPix color;      // RGBA color
    float lineWidth;         // Line thickness
};
```

## Rendering Order

Overlays are rendered in this order (back to front):

1. **Lines** (bottom layer)
2. **Arcs** (middle layer)
3. **Points** (top layer)

This ensures points are always visible on top of lines.

## Performance Considerations

- **Rendering**: Overlay is rendered directly to the image buffer (no extra copy)
- **Anti-aliasing**: Uses Xiaolin Wu's algorithm for smooth lines
- **Scalability**: Performance is O(n) where n = number of primitives
- **Typical use case**: 10-1000 primitives (works well)

For large overlays (>10,000 primitives), consider:
- Implementing primitive culling (only render visible primitives)
- Using LOD (Level of Detail) at low zoom levels
- GPU acceleration (future enhancement)

## Troubleshooting

### Overlay not visible?

1. Check that overlay coordinates are within image bounds
2. Verify color alpha is not 0 (transparent)
3. Ensure `view->HasOverlay()` returns true
4. Check that image is being rendered (not black/empty)

### Overlay disappears when zooming?

This shouldn't happen - overlay should scale with image. Check that:
- `ComputeWarpMatrix()` is being called
- `scale` parameter is correct in `Render()`

### Performance issues?

- Reduce number of primitives
- Check for infinite loops in your code
- Profile `OverlayGraphics::Render()` function

## Next Steps

To complete Phase 2 (Debugger Data Binding):

1. **Implement overlay data parsing** in `WatchedImageOverlayOp::ParseOverlayData()`
2. **Read from debugger** using VS Debugger API
3. **Parse natvis metadata** for custom overlay structures
4. **Test with OpenCV** `std::vector<cv::Point2f>`
5. **Add documentation** for natvis-based custom overlay types

## API Reference

### NativeImageView Methods

```cpp
// Add overlay primitives
void AddOverlayPoint(float x, float y, 
                     Byte r, Byte g, Byte b, Byte a,
                     float radius = 3.0f, float lineWidth = 1.0f);

void AddOverlayLine(float x0, float y0, float x1, float y1,
                    Byte r, Byte g, Byte b, Byte a,
                    float lineWidth = 1.0f);

void AddOverlayArc(float cx, float cy, float radius,
                   float startAngle, float endAngle,
                   Byte r, Byte g, Byte b, Byte a,
                   float lineWidth = 1.0f);

// Clear all overlays
void ClearOverlay();

// Check if overlay exists
bool HasOverlay() const;
```

## Examples

See test applications in `Test/OverlayTest/` (to be created) for complete working examples.

## Contributing

To contribute to overlay graphics feature:

1. **Rendering improvements**: Better anti-aliasing, new primitive types
2. **Performance**: GPU acceleration, primitive culling
3. **UI controls**: Color picker, line width slider
4. **Data binding**: Natvis-based overlay type support
5. **Testing**: Unit tests, integration tests

Contact: Open an issue on GitHub at https://github.com/cutepig123/image-watch
