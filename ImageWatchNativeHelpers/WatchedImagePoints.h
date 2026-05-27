#pragma once

#include "WatchedImageImage.h"
#include "WatchedImageHelpers.h"

using namespace System;

namespace Microsoft 
{
    namespace ImageWatch
    {
        // WatchedImage for point/vector overlay types
        // Creates a virtual image with points drawn on it
        public ref class WatchedImagePoints : WatchedImageImage
        {
        public:
            WatchedImagePoints();
            
        protected:
            virtual void DoReloadInfo(WatchedImageInfo^% info) override;
            virtual void DoReloadPixels(bool% hasPixelsLoaded) override;
            
        private:
            bool ReadPointsData();
            void CreateVirtualImage();
            
        private:
            cli::array<float>^ pointsX_;
            cli::array<float>^ pointsY_;
            UInt32 numPoints_;
            float minX_, maxX_, minY_, maxY_;
        };
    }
}