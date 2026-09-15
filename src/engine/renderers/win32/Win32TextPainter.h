// =================================================================================
// Filename:    engine/renderers/win32/Win32TextPainter.h
// Author:      Ebdsaleh
// Description: Declares Win32 formatted text and inline-emoticon painting helpers.
// =================================================================================
#pragma once

#include <windows.h>

class Label;
class TextInput;

class Win32TextPainter {
    public:
        static void render_label(HDC device_context, const Label& label);
        static void render_text_input(
            HDC device_context,
            const TextInput& text_input
        );
};
