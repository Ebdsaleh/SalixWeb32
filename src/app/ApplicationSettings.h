// =================================================================================
// Filename:    app/ApplicationSettings.h
// Author:      Ebdsaleh
// Description: Declares local startup settings for SalixWeb32.
// =================================================================================
#pragma once

#include <string>

class ApplicationSettings {
    public:
        ApplicationSettings();

        void load();

        bool get_use_remote_bridge() const;
        const char* get_bridge_host() const;
        unsigned short get_bridge_port() const;
        const char* get_source_path() const;

    private:
        bool load_file(
            const char* path,
            bool& backend_explicit,
            bool& bridge_host_configured
        );

        void apply_key_value(
            const std::string& key,
            const std::string& value,
            bool& backend_explicit,
            bool& bridge_host_configured
        );

        bool use_remote_bridge;
        std::string bridge_host;
        unsigned short bridge_port;
        std::string source_path;
};
