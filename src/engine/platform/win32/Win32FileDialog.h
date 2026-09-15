// =================================================================================
// Filename:    engine/platform/win32/Win32FileDialog.h
// Author:      Ebdsaleh
// Description: Declares the Win32 implementation of the file-dialog contract.
// =================================================================================
#pragma once

#include "framework/FileDialog.h"

class Win32FileDialog : public FileDialog {
    public:
        Win32FileDialog();
        virtual ~Win32FileDialog();

        virtual bool open_files(
            std::vector<std::string>& selected_paths
        );
};
