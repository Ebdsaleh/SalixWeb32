// =================================================================================
// Filename:    engine/platform/win32/Win32TextMetrics.h
// Author:      Ebdsaleh
// Description: Declares Win32 text measurement for framework hit-testing.
// =================================================================================
#pragma once

#include <windows.h>

#include "framework/TextMetrics.h"

class Win32TextMetrics : public TextMetrics {
    public:
        Win32TextMetrics(HDC device_context);

        virtual int get_character_index_at_x(
            const char* text,
            int text_length,
            int pixel_x
        );

    private:
        HDC device_context;
};
