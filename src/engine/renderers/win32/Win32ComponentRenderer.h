// =================================================================================
// Filename:    engine/renderers/win32/Win32ComponentRenderer.h
// Author:      Ebdsaleh
// Description: Declares the Win32 renderer for framework UI components.
// =================================================================================
#pragma once

#include <windows.h>

#include "framework/rendering/ComponentRenderer.h"

class Win32ComponentRenderer : public ComponentRenderer {
    public:
        Win32ComponentRenderer(HDC device_context);
        virtual ~Win32ComponentRenderer();

        virtual void render_label(const Label& label);
        virtual void render_button(const Button& button);
        virtual void render_text_input(const TextInput& text_input);

    private:
        HDC device_context;
        HFONT previous_font;
        int previous_background_mode;
};
