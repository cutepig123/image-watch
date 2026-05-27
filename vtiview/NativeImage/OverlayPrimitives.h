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
