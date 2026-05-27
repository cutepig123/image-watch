#pragma once

#include "WatchedImageImage.h"

namespace Microsoft 
{
    namespace ImageWatch
    {
        // @overlay(image, overlayData) operator
        // Combines an image with overlay graphics data
        // The second argument is overlay data expression (points, lines)
        public ref class WatchedImageOverlayOp : public WatchedImageImage
        {
        public:
            WatchedImageOverlayOp();
            virtual ~WatchedImageOverlayOp();
            !WatchedImageOverlayOp();
            
        protected:
            virtual void DoReloadInfo(WatchedImageInfo^% info) override;
            virtual void DoReloadPixels(bool% hasPixelsLoaded) override;
            
        private:
            String^ overlayDataExpr_;
            WatchedImage^ sourceImage_;
            
            void ParseOverlayDataAndApply();
            
            // Helper methods for reading overlay data from debugger
            bool ReadPointsArray(String^ arrayExpr);
            bool ReadLinesArray(String^ arrayExpr);
            bool EvaluateFloatMember(String^ objectExpr, String^ memberName, float% value);
            bool EvaluateIntMember(String^ objectExpr, String^ memberName, int% value);
        };
    }
}
