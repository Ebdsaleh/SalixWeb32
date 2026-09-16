// =================================================================================
// Filename:    engine/platform/win32/Win32GraphicsRuntime.cpp
// Author:      Ebdsaleh
// Description: Implements process-scoped GDI+ startup for Win32 image services.
// =================================================================================

#include "Win32GraphicsRuntime.h"

Win32GraphicsRuntime::Win32GraphicsRuntime()
    : token(0),
      is_initialized(false) {
}

Win32GraphicsRuntime::~Win32GraphicsRuntime() {
    shutdown();
}

bool Win32GraphicsRuntime::initialize() {
    if (is_initialized) {
        return true;
    }

    Gdiplus::GdiplusStartupInput startup_input;
    Gdiplus::Status status = Gdiplus::GdiplusStartup(
        &token,
        &startup_input,
        0
    );

    is_initialized = status == Gdiplus::Ok;
    return is_initialized;
}

void Win32GraphicsRuntime::shutdown() {
    if (!is_initialized) {
        return;
    }

    Gdiplus::GdiplusShutdown(token);
    token = 0;
    is_initialized = false;
}

bool Win32GraphicsRuntime::get_is_initialized() const {
    return is_initialized;
}
