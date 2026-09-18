// =================================================================================
// Filename:    engine/platform/win32/Win32FileDialog.cpp
// Author:      Ebdsaleh
// Description: Implements native Win32 multi-file selection.
// =================================================================================

#include <windows.h>
#include <commdlg.h>
#include <string.h>

#include "Win32FileDialog.h"

namespace {
    std::string get_parent_directory(
        const std::string& path
    ) {
        std::string::size_type separator =
            path.find_last_of("\\/");

        if (separator == std::string::npos) {
            return std::string();
        }

        return path.substr(0, separator);
    }
}

Win32FileDialog::Win32FileDialog() {
}

Win32FileDialog::~Win32FileDialog() {
}

void Win32FileDialog::set_initial_directory(
    const char* directory
) {
    initial_directory =
        directory == 0
            ? ""
            : directory;
}

const char* Win32FileDialog::get_last_directory() const {
    return last_directory.c_str();
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
    open_file_name.lpstrInitialDir =
        initial_directory.empty()
            ? NULL
            : initial_directory.c_str();

    // OFN_NOCHANGEDIR is intentional.  A file picker may remember its own
    // navigation location, but it must never mutate process-wide path state.
    // Diagnostics, exports, caches, and future storage categories each own an
    // explicit directory policy instead of inheriting the last picker folder.
    open_file_name.Flags =
        OFN_EXPLORER |
        OFN_ALLOWMULTISELECT |
        OFN_FILEMUSTEXIST |
        OFN_PATHMUSTEXIST |
        OFN_HIDEREADONLY |
        OFN_NOCHANGEDIR;

    if (!GetOpenFileNameA(&open_file_name)) {
        return false;
    }

    const char* first_entry = file_buffer;
    const char* next_entry =
        first_entry + strlen(first_entry) + 1;

    if (*next_entry == '\0') {
        std::string full_path(first_entry);
        selected_paths.push_back(full_path);
        last_directory = get_parent_directory(full_path);

        if (!last_directory.empty()) {
            initial_directory = last_directory;
        }

        return true;
    }

    std::string directory(first_entry);
    last_directory = directory;
    initial_directory = directory;

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
