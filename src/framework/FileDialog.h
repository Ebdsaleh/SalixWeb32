// =================================================================================
// Filename:    framework/FileDialog.h
// Author:      Ebdsaleh
// Description: Declares a backend-neutral file-selection dialog contract.
// =================================================================================
#pragma once

#include <string>
#include <vector>

class FileDialog {
    public:
        virtual ~FileDialog() {}

        virtual void set_initial_directory(
            const char* directory
        ) = 0;

        virtual const char* get_last_directory() const = 0;

        virtual bool open_files(
            std::vector<std::string>& selected_paths
        ) = 0;
};
