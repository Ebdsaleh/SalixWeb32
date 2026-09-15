// =================================================================================
// Filename:    engine/renderers/win32/Win32EmoticonPainter.h
// Author:      Ebdsaleh
// Description: Declares original classic-messenger-inspired GDI emoticon painting.
// =================================================================================
#pragma once

#include <windows.h>

#include "framework/EmoticonRegistry.h"

class Win32EmoticonPainter {
    public:
        static void draw(
            HDC device_context,
            EmoticonRegistry::EmoticonId emoticon_id,
            const RECT& bounds
        );
};
