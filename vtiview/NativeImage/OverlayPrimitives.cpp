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
