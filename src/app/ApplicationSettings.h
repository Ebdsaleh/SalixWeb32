// =================================================================================
// Filename:    app/ApplicationSettings.h
// Author:      Ebdsaleh
// Description: Declares startup configuration and persistent user settings/state.
// =================================================================================
#pragma once

#include <string>

class ApplicationSettings {
    public:
        ApplicationSettings();

        void set_path_context(
            const char* launch_directory,
            const char* executable_directory,
            const char* user_data_directory,
            const char* user_preferences_path,
            bool portable_mode
        );

        void load();
        bool save_user_preferences() const;

        bool get_use_remote_bridge() const;
        const char* get_bridge_host() const;
        unsigned short get_bridge_port() const;
        const char* get_source_path() const;

        const char* get_launch_directory() const;
        const char* get_executable_directory() const;
        const char* get_user_data_directory() const;
        const char* get_user_preferences_path() const;
        const char* get_diagnostics_directory() const;
        const char* get_attachment_directory() const;
        bool is_portable_mode() const;
        const char* get_application_mode_name() const;

        std::string get_default_diagnostics_directory() const;
        std::string get_default_attachment_directory() const;

        void set_diagnostics_directory(const char* path);
        void set_attachment_directory(const char* path);
        void reset_file_locations();

    private:
        bool load_file(
            const char* path,
            bool& backend_explicit,
            bool& bridge_host_configured,
            bool record_source_path,
            bool path_preferences_only
        );

        void apply_key_value(
            const std::string& key,
            const std::string& value,
            bool& backend_explicit,
            bool& bridge_host_configured,
            bool path_preferences_only
        );

        std::string make_launch_relative_path(
            const char* relative_path
        ) const;

        bool use_remote_bridge;
        std::string bridge_host;
        unsigned short bridge_port;
        std::string source_path;

        std::string launch_directory;
        std::string executable_directory;
        std::string user_data_directory;
        std::string user_preferences_path;
        std::string diagnostics_directory;
        std::string attachment_directory;
        bool portable_mode;
};
