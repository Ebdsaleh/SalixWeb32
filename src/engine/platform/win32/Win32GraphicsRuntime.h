// =================================================================================
// Filename:    engine/platform/win32/Win32GraphicsRuntime.h
// Author:      Ebdsaleh
// Description: Owns process-scoped GDI+ startup for Win32 image services.
// =================================================================================
#pragma once

#include <windows.h>
#include <gdiplus.h>

class Win32GraphicsRuntime {
    public:
        Win32GraphicsRuntime();
        ~Win32GraphicsRuntime();

        bool initialize();
        void shutdown();
        bool get_is_initialized() const;

    private:
        ULONG_PTR token;
        bool is_initialized;
};
