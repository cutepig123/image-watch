#include "WatchedImageOverlayOp.h"
#include "WatchedImageHelpers.h"
#include "WatchedImageImage.h"
#include "NativeImageView.h"

#include <Microsoft.VisualStudio.Debugger.Evaluation.DkmInspectionContext.h>
#include <Microsoft.VisualStudio.Debugger.CallStack.DkmStackWalkFrame.h>

using namespace System;
using namespace Microsoft::Research::NativeImage;

namespace Microsoft 
{
    namespace ImageWatch
    {
        WatchedImageOverlayOp::WatchedImageOverlayOp()
            : WatchedImageImage(), overlayDataExpr_(nullptr), sourceImage_(nullptr)
        {
        }
        
        WatchedImageOverlayOp::~WatchedImageOverlayOp()
        {
            this->!WatchedImageOverlayOp();
        }
        
        WatchedImageOverlayOp::!WatchedImageOverlayOp()
        {
        }
        
        void WatchedImageOverlayOp::DoReloadInfo(WatchedImageInfo^% info)
        {
            info = nullptr;
            
            // Args[0] should be the source image
            if (ObjectPointerType == nullptr || ObjectPointerType->Contains("WatchedImageOverlayOp"))
            {
                // This is the operator itself, get the source image from process
                // For now, we need a different approach
                
                // Actually, we need to get args from somewhere
                // The expression parser should have set this up
                
                return;
            }
            
            // Use parent's info loading
            WatchedImageImage::DoReloadInfo(info);
        }
        
        void WatchedImageOverlayOp::DoReloadPixels(bool% hasPixelsLoaded)
        {
            hasPixelsLoaded = false;
            
            if (!HasValidInfo())
                return;
            
            // Load source image pixels first
            // The source image should be stored somewhere
            
            // Parse overlay data and apply to view
            ParseOverlayDataAndApply();
            
            hasPixelsLoaded = true;
        }
        
        void WatchedImageOverlayOp::ParseOverlayDataAndApply()
        {
            if (overlayDataExpr_ == nullptr || InspectionContext == nullptr || Frame == nullptr)
                return;
            
#ifdef _DEBUG
            System::Diagnostics::Debug::WriteLine(
                "ParseOverlayDataAndApply: Parsing '{0}'", overlayDataExpr_);
#endif
            
            // Try to read as points array (std::vector<cv::Point2f> or similar)
            if (ReadPointsArray(overlayDataExpr_))
                return;
            
            // Try to read as lines array
            if (ReadLinesArray(overlayDataExpr_))
                return;
        }
        
        bool WatchedImageOverlayOp::ReadPointsArray(String^ arrayExpr)
        {
            // Read size of the array
            String^ sizeExpr = String::Format("({0}).size()", arrayExpr);
            String^ sizeValue = nullptr;
            String^ sizeType = nullptr;
            
            if (!WatchedImageHelpers::EvaluateExpression(
                InspectionContext, Frame, sizeExpr, sizeValue, sizeType))
            {
#ifdef _DEBUG
                System::Diagnostics::Debug::WriteLine(
                    "Failed to evaluate size expression: {0}", sizeExpr);
#endif
                return false;
            }
            
            UInt32 size = 0;
            if (!WatchedImageHelpers::TryParseUInt32(sizeValue, size))
            {
#ifdef _DEBUG
                System::Diagnostics::Debug::WriteLine(
                    "Failed to parse size: {0}", sizeValue);
#endif
                return false;
            }
            
#ifdef _DEBUG
            System::Diagnostics::Debug::WriteLine(
                "Reading {0} points from array", size);
#endif
            
            // Read each point
            for (UInt32 i = 0; i < size && i < 1000; ++i)  // Limit to 1000 points
            {
                String^ xExpr = String::Format("({0})[{1}].x", arrayExpr, i);
                String^ yExpr = String::Format("({0})[{1}].y", arrayExpr, i);
                
                String^ xValue = nullptr, yValue = nullptr;
                String^ tempType = nullptr;
                
                if (WatchedImageHelpers::EvaluateExpression(
                    InspectionContext, Frame, xExpr, xValue, tempType) &&
                    WatchedImageHelpers::EvaluateExpression(
                    InspectionContext, Frame, yExpr, yValue, tempType))
                {
                    float x = 0, y = 0;
                    
                    // Try parsing as float
                    double xDouble, yDouble;
                    if (WatchedImageHelpers::TryParseDouble(xValue, xDouble) &&
                        WatchedImageHelpers::TryParseDouble(yValue, yDouble))
                    {
                        x = (float)xDouble;
                        y = (float)yDouble;
                        
                        // Add point to overlay
                        // Note: We need access to NativeImageView here
                        // For now, store the data
                        
#ifdef _DEBUG
                        System::Diagnostics::Debug::WriteLine(
                            "Point {0}: ({1}, {2})", i, x, y);
#endif
                    }
                }
            }
            
            return true;
        }
        
        bool WatchedImageOverlayOp::ReadLinesArray(String^ arrayExpr)
        {
            // Similar to ReadPointsArray, but for lines
            // Each line has 4 coordinates: x0, y0, x1, y1
            
            String^ sizeExpr = String::Format("({0}).size()", arrayExpr);
            String^ sizeValue = nullptr;
            String^ sizeType = nullptr;
            
            if (!WatchedImageHelpers::EvaluateExpression(
                InspectionContext, Frame, sizeExpr, sizeValue, sizeType))
                return false;
            
            UInt32 size = 0;
            if (!WatchedImageHelpers::TryParseUInt32(sizeValue, size))
                return false;
            
            for (UInt32 i = 0; i < size && i < 500; ++i)  // Limit to 500 lines
            {
                String^ x0Expr = String::Format("({0})[{1}].x0", arrayExpr, i);
                String^ y0Expr = String::Format("({0})[{1}].y0", arrayExpr, i);
                String^ x1Expr = String::Format("({0})[{1}].x1", arrayExpr, i);
                String^ y1Expr = String::Format("({0})[{1}].y1", arrayExpr, i);
                
                String^ x0Value = nullptr, *y0Value = nullptr, *x1Value = nullptr, *y1Value = nullptr;
                String^ tempType = nullptr;
                
                // Try alternative member names (start/end)
                if (!WatchedImageHelpers::EvaluateExpression(
                    InspectionContext, Frame, x0Expr, x0Value, tempType))
                {
                    x0Expr = String::Format("({0})[{1}].start.x", arrayExpr, i);
                    WatchedImageHelpers::EvaluateExpression(
                        InspectionContext, Frame, x0Expr, x0Value, tempType);
                }
                
                // Continue with other coordinates...
                // (Similar pattern for y0, x1, y1)
            }
            
            return true;
        }
        
        bool WatchedImageOverlayOp::EvaluateFloatMember(String^ objectExpr, 
            String^ memberName, float% value)
        {
            String^ expr = String::Format("({0}).{1}", objectExpr, memberName);
            String^ valueStr = nullptr;
            String^ typeStr = nullptr;
            
            if (!WatchedImageHelpers::EvaluateExpression(
                InspectionContext, Frame, expr, valueStr, typeStr))
                return false;
            
            double dValue;
            if (!WatchedImageHelpers::TryParseDouble(valueStr, dValue))
                return false;
            
            value = (float)dValue;
            return true;
        }
        
        bool WatchedImageOverlayOp::EvaluateIntMember(String^ objectExpr, 
            String^ memberName, int% value)
        {
            String^ expr = String::Format("({0}).{1}", objectExpr, memberName);
            String^ valueStr = nullptr;
            String^ typeStr = nullptr;
            
            if (!WatchedImageHelpers::EvaluateExpression(
                InspectionContext, Frame, expr, valueStr, typeStr))
                return false;
            
            Int32 iValue;
            if (!WatchedImageHelpers::TryParseInt32(valueStr, iValue))
                return false;
            
            value = iValue;
            return true;
        }
    }
}
