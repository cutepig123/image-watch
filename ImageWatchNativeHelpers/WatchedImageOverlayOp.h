#pragma once

#include "WatchedImageBinaryOp.h"

namespace Microsoft 
{
    namespace ImageWatch
    {
        public ref class WatchedImageOverlayOp : public WatchedImageBinaryOp
        {
        public:
            WatchedImageOverlayOp();
            
        protected:
            virtual void DoReloadInfo(WatchedImageInfo^% info) override;
            virtual void DoReloadPixels(bool% hasPixelsLoaded) override;
        };
    }
}
