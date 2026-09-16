// =================================================================================
// Filename:    engine/platform/win32/Win32FileDialog.cpp
// Author:      Ebdsaleh
// Description: Implements native Win32 multi-file selection.
// =================================================================================

#include <windows.h>
#include <commdlg.h>
#include <string.h>

#include "Win32FileDialog.h"

Win32FileDialog::Win32FileDialog() {
}

Win32FileDialog::~Win32FileDialog() {
}

bool Win32FileDialog::open_files(
    std::vector<std::string>& selected_paths
) {
    selected_paths.clear();

    char file_buffer[32768];
    file_buffer[0] = '\0';

    OPENFILENAMEA open_file_name;
    ZeroMemory(&open_file_name, sizeof(open_file_name));

    open_file_name.lStructSize = sizeof(open_file_name);
    open_file_name.hwndOwner = GetActiveWindow();
    open_file_name.lpstrFile = file_buffer;
    open_file_name.nMaxFile = sizeof(file_buffer);
    open_file_name.lpstrFilter = "All files\0*.*\0\0";
    open_file_name.nFilterIndex = 1;

    // Explorer-style multi-selection deliberately delegates the familiar
    // Ctrl+Click and Shift+Click gestures to the operating system. One
    // GetOpenFileNameA invocation can therefore return the complete selected
    // set to the composer without SalixWeb32 reimplementing shell selection.
    open_file_name.Flags =
        OFN_EXPLORER |
        OFN_ALLOWMULTISELECT |
        OFN_FILEMUSTEXIST |
        OFN_PATHMUSTEXIST |
        OFN_HIDEREADONLY;

    if (!GetOpenFileNameA(&open_file_name)) {
        return false;
    }

    const char* first_entry = file_buffer;
    const char* next_entry = first_entry + strlen(first_entry) + 1;

    if (*next_entry == '\0') {
        selected_paths.push_back(std::string(first_entry));
        return true;
    }

    std::string directory(first_entry);

    while (*next_entry != '\0') {
        std::string full_path(directory);

        if (
            !full_path.empty() &&
            full_path[full_path.length() - 1] != '\\' &&
            full_path[full_path.length() - 1] != '/'
        ) {
            full_path += '\\';
        }

        full_path += next_entry;
        selected_paths.push_back(full_path);

        next_entry += strlen(next_entry) + 1;
    }

    return !selected_paths.empty();
}
