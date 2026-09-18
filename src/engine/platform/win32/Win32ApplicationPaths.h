// =================================================================================
// Filename:    engine/platform/win32/Win32ApplicationPaths.h
// Author:      Ebdsaleh
// Description: Resolves stable Win32 application and persistent storage locations.
// =================================================================================
#pragma once

#include <string>

class Win32ApplicationPaths {
    public:
        static bool get_launch_directory(
            std::string& directory
        );

        static bool get_executable_directory(
            std::string& directory
        );

        static bool get_user_data_directory(
            bool portable_mode,
            const char* executable_directory,
            std::string& directory
        );

        static bool get_user_preferences_file(
            const char* user_data_directory,
            std::string& path
        );

        static bool directory_exists(const char* path);
        static bool ensure_directory_exists(const char* path);
        static bool is_absolute_path(const char* path);
};
