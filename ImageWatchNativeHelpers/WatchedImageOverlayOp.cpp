#include "WatchedImageOverlayOp.h"

using namespace System;

namespace Microsoft 
{
    namespace ImageWatch
    {
        WatchedImageOverlayOp::WatchedImageOverlayOp()
            : WatchedImageBinaryOp()
        {
        }
        
        void WatchedImageOverlayOp::DoReloadInfo(WatchedImageInfo^% info)
        {
            if (Operand1 != nullptr)
            {
                Operand1->ReloadInfo();
                info = Operand1->GetInfo();
            }
        }
        
        void WatchedImageOverlayOp::DoReloadPixels(bool% hasPixelsLoaded)
        {
            if (Operand1 != nullptr)
            {
                Operand1->ReloadPixels();
                hasPixelsLoaded = Operand1->HasPixelsLoaded();
            }
            
            // TODO: Parse overlay data from Operand2 and add primitives
            // This would require debugger expression evaluation
        }
    }
}
