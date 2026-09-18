// =================================================================================
// Filename:    engine/platform/win32/Win32ApplicationPaths.cpp
// Author:      Ebdsaleh
// Description: Implements stable Win32 application and persistent storage locations.
// =================================================================================

#include <windows.h>
#include <string.h>
#include <string>

#include "Win32ApplicationPaths.h"

namespace {
    std::string trim_trailing_separator(
        const std::string& path
    ) {
        std::string result(path);

        while (
            result.size() > 3 &&
            (result[result.size() - 1] == '\\' ||
             result[result.size() - 1] == '/')
        ) {
            result.erase(result.size() - 1);
        }

        return result;
    }

    bool create_directory_level(const std::string& path) {
        if (path.empty()) {
            return false;
        }

        DWORD attributes = GetFileAttributesA(path.c_str());
        if (
            attributes != INVALID_FILE_ATTRIBUTES &&
            (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0
        ) {
            return true;
        }

        if (CreateDirectoryA(path.c_str(), NULL)) {
            return true;
        }

        return GetLastError() == ERROR_ALREADY_EXISTS;
    }
}

bool Win32ApplicationPaths::get_launch_directory(
    std::string& directory
) {
    directory.clear();

    char current_directory[MAX_PATH + 1];
    DWORD length = GetCurrentDirectoryA(
        MAX_PATH,
        current_directory
    );

    if (length == 0 || length >= MAX_PATH) {
        return false;
    }

    current_directory[length] = '\0';
    directory = trim_trailing_separator(
        current_directory
    );

    return
        !directory.empty() &&
        is_absolute_path(directory.c_str());
}

bool Win32ApplicationPaths::get_executable_directory(
    std::string& directory
) {
    directory.clear();

    char module_path[MAX_PATH + 1];
    DWORD length = GetModuleFileNameA(
        NULL,
        module_path,
        MAX_PATH
    );

    if (length == 0 || length >= MAX_PATH) {
        return false;
    }

    module_path[length] = '\0';

    std::string path(module_path);
    std::string::size_type separator =
        path.find_last_of("\\/");

    if (separator == std::string::npos) {
        return false;
    }

    directory = trim_trailing_separator(
        path.substr(0, separator)
    );
    return !directory.empty();
}

bool Win32ApplicationPaths::get_user_data_directory(
    bool portable_mode,
    const char* executable_directory,
    std::string& directory
) {
    directory.clear();

    if (portable_mode) {
        if (
            executable_directory == 0 ||
            executable_directory[0] == '\0'
        ) {
            return false;
        }

        directory = trim_trailing_separator(
            executable_directory
        );
    } else {
        char app_data[MAX_PATH + 1];
        DWORD length = GetEnvironmentVariableA(
            "APPDATA",
            app_data,
            MAX_PATH
        );

        if (length == 0 || length >= MAX_PATH) {
            return false;
        }

        app_data[length] = '\0';
        directory = trim_trailing_separator(app_data);
        directory += "\\SalixWeb32";
    }

    if (
        directory.empty() ||
        directory.size() >= MAX_PATH ||
        !is_absolute_path(directory.c_str())
    ) {
        directory.clear();
        return false;
    }

    if (!ensure_directory_exists(directory.c_str())) {
        directory.clear();
        return false;
    }

    return true;
}

bool Win32ApplicationPaths::get_user_preferences_file(
    const char* user_data_directory,
    std::string& path
) {
    path.clear();

    if (
        user_data_directory == 0 ||
        user_data_directory[0] == '\0'
    ) {
        return false;
    }

    std::string root = trim_trailing_separator(
        user_data_directory
    );

    if (
        root.empty() ||
        root.size() >= MAX_PATH ||
        !is_absolute_path(root.c_str())
    ) {
        return false;
    }

    path = root;
    path += "\\settings.ini";

    return path.size() < MAX_PATH;
}

bool Win32ApplicationPaths::directory_exists(
    const char* path
) {
    if (path == 0 || path[0] == '\0') {
        return false;
    }

    DWORD attributes = GetFileAttributesA(path);

    return
        attributes != INVALID_FILE_ATTRIBUTES &&
        (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

bool Win32ApplicationPaths::ensure_directory_exists(
    const char* path
) {
    if (path == 0 || path[0] == '\0') {
        return false;
    }

    std::string target =
        trim_trailing_separator(path);

    std::string::size_type slash_index = 0;
    for (
        slash_index = 0;
        slash_index < target.size();
        ++slash_index
    ) {
        if (target[slash_index] == '/') {
            target[slash_index] = '\\';
        }
    }

    if (
        target.empty() ||
        target.size() >= MAX_PATH ||
        !is_absolute_path(target.c_str())
    ) {
        return false;
    }

    if (directory_exists(target.c_str())) {
        return true;
    }

    std::string current;
    std::string::size_type index = 0;

    if (
        target.size() >= 2 &&
        target[0] == '\\' &&
        target[1] == '\\'
    ) {
        std::string::size_type server_end =
            target.find('\\', 2);
        if (server_end == std::string::npos) {
            return false;
        }

        std::string::size_type share_end =
            target.find('\\', server_end + 1);
        if (share_end == std::string::npos) {
            return directory_exists(target.c_str());
        }

        current = target.substr(0, share_end);
        index = share_end + 1;
    } else {
        current = target.substr(0, 3);
        index = 3;
    }

    while (index <= target.size()) {
        std::string::size_type separator =
            target.find('\\', index);

        std::string part =
            separator == std::string::npos
                ? target.substr(index)
                : target.substr(
                    index,
                    separator - index
                );

        if (!part.empty()) {
            if (
                !current.empty() &&
                current[current.size() - 1] != '\\'
            ) {
                current += "\\";
            }

            current += part;

            if (!create_directory_level(current)) {
                return false;
            }
        }

        if (separator == std::string::npos) {
            break;
        }

        index = separator + 1;
    }

    return directory_exists(target.c_str());
}

bool Win32ApplicationPaths::is_absolute_path(
    const char* path
) {
    if (path == 0) {
        return false;
    }

    size_t length = strlen(path);
    if (length < 2) {
        return false;
    }

    if (
        length >= 2 &&
        path[0] == '\\' &&
        path[1] == '\\'
    ) {
        return true;
    }

    return
        length >= 3 &&
        path[1] == ':' &&
        (path[2] == '\\' || path[2] == '/');
}
