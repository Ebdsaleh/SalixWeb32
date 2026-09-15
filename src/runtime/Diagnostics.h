// =================================================================================
// Filename:    runtime/Diagnostics.h
// Author:      Ebdsaleh
// Description: Declares lightweight runtime diagnostic output helpers.
// =================================================================================
#pragma once

class Diagnostics {
    public:
        static void write(const char* message);
        static void write_line(const char* message);
};
