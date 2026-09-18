// =================================================================================
// Filename:    engine/application_hosts/Win32SettingsDialog.h
// Author:      Ebdsaleh
// Description: Declares the native user-facing SalixWeb32 settings dialog.
// =================================================================================
#pragma once

#include <windows.h>

class ApplicationSettings;

class Win32SettingsDialog {
    public:
        static bool show(
            HWND owner_handle,
            ApplicationSettings* settings
        );
};
