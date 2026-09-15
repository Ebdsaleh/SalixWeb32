// =================================================================================
// Filename:    engine/platform/win32/Win32Clipboard.h
// Author:      Ebdsaleh
// Description: Declares the Win32 clipboard backend for MIME-tagged framework data.
// =================================================================================
#pragma once

#include <windows.h>

#include "framework/Clipboard.h"

class Win32Clipboard : public Clipboard {
    public:
        Win32Clipboard(HWND owner_window);

        virtual bool set_data(const MimeData& data);
        virtual bool get_data(MimeData& data);

    private:
        HWND owner_window;
};
