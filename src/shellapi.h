// =================================================================================
// Filename:    shellapi.h
// Author:      Ebdsaleh
// Description: Minimal VC7.1-safe ShellExecute declaration for the NT5 target.
//
// This project-local compatibility header intentionally provides only the shell API
// surface SalixWeb32 currently uses.  The Visual Studio .NET 2003 Platform SDK copy
// of ShellAPI.h assumes Win32 base types have already been declared; including it
// first causes a cascade of errors.  Keeping this shim small avoids changing global
// target-version macros or depending on newer SDK headers.
// =================================================================================
#pragma once

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

HINSTANCE WINAPI ShellExecuteA(
    HWND window_handle,
    LPCSTR operation,
    LPCSTR file,
    LPCSTR parameters,
    LPCSTR directory,
    INT show_command
);

#ifdef __cplusplus
}
#endif
