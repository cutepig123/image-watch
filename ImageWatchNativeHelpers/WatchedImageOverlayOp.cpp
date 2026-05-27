#include "WatchedImageOverlayOp.h"
#include "WatchedImageHelpers.h"
#include "WatchedImageImage.h"

#include <Microsoft.VisualStudio.Debugger.Evaluation.DkmInspectionContext.h>
#include <Microsoft.VisualStudio.Debugger.CallStack.DkmStackWalkFrame.h>

using namespace System;
using namespace Microsoft::Research::NativeImage;

namespace Microsoft 
{
    namespace ImageWatch
    {
        WatchedImageOverlayOp::WatchedImageOverlayOp()
            : WatchedImageOperator(), sourceImage_(nullptr), overlayDataExpr_(nullptr)
        {
        }
        
        WatchedImageOverlayOp::~WatchedImageOverlayOp()
        {
            this->!WatchedImageOverlayOp();
        }
        
        WatchedImageOverlayOp::!WatchedImageOverlayOp()
        {
        }
        
        array<Type^>^ WatchedImageOverlayOp::DoGetArgumentTypes()
        {
            // Two arguments: image and overlay data expression
            auto res = gcnew array<Type^>(2);
            res[0] = WatchedImage::typeid;  // Image
            res[1] = String::typeid;         // Overlay data expression
            return res;
        }
        
        vt::CTransformGraphNode* WatchedImageOverlayOp::GetTransformGraphHead()
        {
            // Overlay doesn't use Transform Graph
            // We return the source image's transform graph
            if (!CheckArguments() || sourceImage_ == nullptr)
                return nullptr;
            
            auto op = dynamic_cast<WatchedImageOperator^>(sourceImage_);
            if (op != nullptr)
            {
                return op->GetTransformGraphHead();
            }
            
            return nullptr;
        }
        
        void WatchedImageOverlayOp::DoReloadInfo(WatchedImageInfo^% info)
        {
            info = nullptr;
            
            if (!CheckArguments())
                return;
            
            // First argument is the image
            sourceImage_ = dynamic_cast<WatchedImage^>(Args[0]);
            if (sourceImage_ == nullptr)
                return;
            
            // Second argument is overlay data expression
            overlayDataExpr_ = dynamic_cast<String^>(Args[1]);
            
            // Reload source image info
            sourceImage_->ReloadInfo();
            
            if (!sourceImage_->HasValidInfo())
                return;
            
            // Use source image's info
            info = sourceImage_->GetInfo();
        }
        
        void WatchedImageOverlayOp::DoReloadPixels(bool% hasPixelsLoaded)
        {
            hasPixelsLoaded = false;
            
            if (sourceImage_ == nullptr || !sourceImage_->HasValidInfo())
                return;
            
            // Reload source image pixels
            sourceImage_->ReloadPixels();
            hasPixelsLoaded = sourceImage_->HasPixelsLoaded();
            
            if (!hasPixelsLoaded)
                return;
            
            // Parse and apply overlay
            ParseOverlayData();
        }
        
        void WatchedImageOverlayOp::ParseOverlayData()
        {
            // This is a simplified implementation
            // In a full implementation, we would:
            // 1. Evaluate overlayDataExpr_ in the debugger context
            // 2. Parse the structure (points, lines, arcs)
            // 3. Extract coordinates and colors
            // 4. Call NativeImageView::AddOverlayPoint/Line/Arc
            
            // For now, this is a placeholder that demonstrates the concept
            // Full implementation requires deep integration with VS Debugger API
            
#ifdef _DEBUG
            System::Diagnostics::Debug::WriteLine(
                "WatchedImageOverlayOp::ParseOverlayData: Overlay expression: {0}", 
                overlayDataExpr_);
#endif
        }
        
        void WatchedImageOverlayOp::ApplyOverlayToView()
        {
            // This would be called after rendering to add overlay
            // The actual implementation needs access to NativeImageView
            // which is managed at a higher level in the UI layer
        }
    }
}
