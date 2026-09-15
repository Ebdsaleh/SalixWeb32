// =================================================================================
// Filename:    runtime/Diagnostics.cpp
// Author:      Ebdsaleh
// Description: Implements lightweight runtime diagnostic output helpers.
// =================================================================================

#include <windows.h>

#include "Diagnostics.h"

void Diagnostics::write(const char* message) {
    if (message == 0) {
        return;
    }

    OutputDebugStringA(message);
}

void Diagnostics::write_line(const char* message) {
    write(message);
    OutputDebugStringA("\r\n");
}
