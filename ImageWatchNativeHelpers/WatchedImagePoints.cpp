#include "WatchedImagePoints.h"
#include <vtcore.h>

using namespace System;
using namespace Microsoft::Research::NativeImage;

namespace Microsoft 
{
    namespace ImageWatch
    {
        WatchedImagePoints::WatchedImagePoints()
            : WatchedImageImage(), pointsX_(nullptr), pointsY_(nullptr), 
              numPoints_(0), minX_(0), maxX_(0), minY_(0), maxY_(0)
        {
        }
        
        void WatchedImagePoints::DoReloadInfo(WatchedImageInfo^% info)
        {
            info = nullptr;
            
            if (ObjectPointerType == nullptr || InspectionContext == nullptr || Frame == nullptr)
                return;
            
            // Read points data first
            if (!ReadPointsData())
                return;
            
            // Calculate image dimensions from point bounds
            if (numPoints_ == 0)
                return;
            
            // Add margin
            float marginX = (maxX_ - minX_) * 0.1f + 10.0f;
            float marginY = (maxY_ - minY_) * 0.1f + 10.0f;
            
            UInt32 width = (UInt32)(maxX_ - minX_ + 2 * marginX);
            UInt32 height = (UInt32)(maxY_ - minY_ + 2 * marginY);
            
            if (width == 0) width = 100;
            if (height == 0) height = 100;
            
            // Clamp to reasonable size
            width = Math::Min(width, 4096u);
            height = Math::Min(height, 4096u);
            
            // Create info
            info = gcnew WatchedImageInfo();
            info->IsInitialized = true;
            info->Width = width;
            info->Height = height;
            info->PixelAddress = 0;
            info->NumBytesPerPixel = 4;  // RGBA
            info->NumStrideBytes = width * 4;
            info->HasNativePixelValues = false;
            info->HasNativePixelAddress = false;
            info->ViewerPixelFormat = VT_IMG_FIXED(OBJ_RGBABYTEIMG);
        }
        
        bool WatchedImagePoints::ReadPointsData()
        {
            // Read vector size
            String^ sizeExpr = String::Format("({0}).size()", Expression);
            String^ sizeValue = nullptr;
            String^ sizeType = nullptr;
            
            if (!WatchedImageHelpers::EvaluateExpression(
                InspectionContext, Frame, sizeExpr, sizeValue, sizeType))
            {
                return false;
            }
            
            UInt32 size = 0;
            if (!WatchedImageHelpers::TryParseUInt32(sizeValue, size))
                return false;
            
            numPoints_ = Math::Min(size, 500u);
            
            if (numPoints_ == 0)
                return false;
            
            // Allocate arrays
            pointsX_ = gcnew cli::array<float>(numPoints_);
            pointsY_ = gcnew cli::array<float>(numPoints_);
            
            // Initialize bounds
            minX_ = FLT_MAX; maxX_ = -FLT_MAX;
            minY_ = FLT_MAX; maxY_ = -FLT_MAX;
            
            // Read each point
            for (UInt32 i = 0; i < numPoints_; i++)
            {
                String^ xExpr = String::Format("({0})[{1}].x", Expression, i);
                String^ yExpr = String::Format("({0})[{1}].y", Expression, i);
                
                String^ xValue = nullptr, yValue = nullptr;
                String^ tempType = nullptr;
                
                if (WatchedImageHelpers::EvaluateExpression(
                    InspectionContext, Frame, xExpr, xValue, tempType) &&
                    WatchedImageHelpers::EvaluateExpression(
                    InspectionContext, Frame, yExpr, yValue, tempType))
                {
                    double x = 0, y = 0;
                    if (WatchedImageHelpers::TryParseDouble(xValue, x) &&
                        WatchedImageHelpers::TryParseDouble(yValue, y))
                    {
                        pointsX_[i] = (float)x;
                        pointsY_[i] = (float)y;
                        
                        // Update bounds
                        minX_ = Math::Min(minX_, (float)x);
                        maxX_ = Math::Max(maxX_, (float)x);
                        minY_ = Math::Min(minY_, (float)y);
                        maxY_ = Math::Max(maxY_, (float)y);
                    }
                }
            }
            
            return numPoints_ > 0;
        }
        
        void WatchedImagePoints::DoReloadPixels(bool% hasPixelsLoaded)
        {
            hasPixelsLoaded = false;
            
            if (!HasValidInfo() || pointsX_ == nullptr || pointsY_ == nullptr)
                return;
            
            CreateVirtualImage();
            
            hasPixelsLoaded = true;
        }
        
        void WatchedImagePoints::CreateVirtualImage()
        {
            // This creates a virtual RGBA image with points drawn on it
            // The actual image data will be created in a custom way
            
            // For now, we'll create a simple black image with white points
            // In a full implementation, we would use vt::CRGBAByteImg
            
            // Note: This is a simplified implementation
            // A complete implementation would create actual pixel data
            // and use the overlay rendering system
        }
    }
}
