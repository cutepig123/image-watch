#pragma once

#include "WatchedImage.h"
#include "WatchedImageOperator.h"

namespace Microsoft 
{
    namespace ImageWatch
    {
        // @overlay(image, overlayData) operator
        // This operator is special: it doesn't use Transform Graph
        // Instead, it passes overlay data to NativeImageView for rendering
        public ref class WatchedImageOverlayOp : public WatchedImageOperator
        {
        public:
            WatchedImageOverlayOp();
            ~WatchedImageOverlayOp();
            !WatchedImageOverlayOp();
            
        internal:
            virtual vt::CTransformGraphNode* GetTransformGraphHead() override;
            
        protected:
            virtual array<Type^>^ DoGetArgumentTypes() override;
            virtual void DoReloadInfo(WatchedImageInfo^% info) override;
            virtual void DoReloadPixels(bool% hasPixelsLoaded) override;
            
        private:
            WatchedImage^ sourceImage_;
            System::String^ overlayDataExpr_;
            
            void ParseOverlayData();
            void ApplyOverlayToView();
        };
    }
}