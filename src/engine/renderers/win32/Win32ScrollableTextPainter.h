// =================================================================================
// Filename:    engine/renderers/win32/Win32ScrollableTextPainter.h
// Author:      Ebdsaleh
// Description: Declares scroll-aware Win32 painting for custom TextInput widgets.
// =================================================================================
#pragma once

#include <windows.h>

class TextInput;

class Win32ScrollableTextPainter {
    public:
        static void render_text_input(
            HDC device_context,
            const TextInput& text_input
        );
};
