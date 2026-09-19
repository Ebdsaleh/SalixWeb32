// =================================================================================
// Filename:    app/ApplicationSettings.cpp
// Author:      Ebdsaleh
// Description: Loads startup configuration and persistent user settings/state.
// =================================================================================

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include "ApplicationSettings.h"

namespace {
    std::string trim_copy(const std::string& text) {
        std::string::size_type start = 0;
        while (
            start < text.size() &&
            isspace((unsigned char)text[start])
        ) {
            ++start;
        }

        std::string::size_type end = text.size();
        while (
            end > start &&
            isspace((unsigned char)text[end - 1])
        ) {
            --end;
        }

        return text.substr(start, end - start);
    }

    std::string lowercase_copy(const std::string& text) {
        std::string result(text);
        std::string::size_type index = 0;

        for (index = 0; index < result.size(); ++index) {
            result[index] = (char)tolower((unsigned char)result[index]);
        }

        return result;
    }

    bool parse_port(
        const std::string& text,
        unsigned short& port
    ) {
        if (text.empty()) {
            return false;
        }

        char* end = 0;
        long value = strtol(text.c_str(), &end, 10);

        if (
            end == text.c_str() ||
            end == 0 ||
            *end != '\0' ||
            value < 1 ||
            value > 65535
        ) {
            return false;
        }

        port = (unsigned short)value;
        return true;
    }

    std::string strip_trailing_separators(
        const std::string& path
    ) {
        std::string result = trim_copy(path);

        while (
            result.size() > 3 &&
            (result[result.size() - 1] == '\\' ||
             result[result.size() - 1] == '/')
        ) {
            result.erase(result.size() - 1);
        }

        return result;
    }

    bool write_setting(
        FILE* file,
        const char* key,
        const std::string& value
    ) {
        if (file == 0 || key == 0) {
            return false;
        }

        return fprintf(
            file,
            "%s=%s\n",
            key,
            value.c_str()
        ) >= 0;
    }
}

ApplicationSettings::ApplicationSettings()
    : use_remote_bridge(false),
      bridge_host("127.0.0.1"),
      bridge_port(8765),
      portable_mode(false) {
}

void ApplicationSettings::set_path_context(
    const char* new_launch_directory,
    const char* new_executable_directory,
    const char* new_user_data_directory,
    const char* new_user_preferences_path,
    bool new_portable_mode
) {
    launch_directory = strip_trailing_separators(
        new_launch_directory == 0
            ? ""
            : new_launch_directory
    );

    executable_directory = strip_trailing_separators(
        new_executable_directory == 0
            ? ""
            : new_executable_directory
    );

    user_data_directory = strip_trailing_separators(
        new_user_data_directory == 0
            ? ""
            : new_user_data_directory
    );

    user_preferences_path =
        new_user_preferences_path == 0
            ? ""
            : trim_copy(new_user_preferences_path);

    portable_mode = new_portable_mode;
}

void ApplicationSettings::load() {
    use_remote_bridge = false;
    bridge_host = "127.0.0.1";
    bridge_port = 8765;
    source_path.clear();
    reset_file_locations();

    bool backend_explicit = false;
    bool bridge_host_configured = false;

    const char* config_path = getenv("SALIX_CONFIG");
    if (config_path != 0 && config_path[0] != '\0') {
        load_file(
            config_path,
            backend_explicit,
            bridge_host_configured,
            true,
            false
        );
    } else {
        bool loaded = false;

        if (!launch_directory.empty()) {
            std::string portable_config =
                make_launch_relative_path(
                    "salixweb32.local.ini"
                );

            loaded = load_file(
                portable_config.c_str(),
                backend_explicit,
                bridge_host_configured,
                true,
                false
            );

            if (!loaded) {
                std::string development_config =
                    make_launch_relative_path(
                        "..\\..\\salixweb32.local.ini"
                    );

                loaded = load_file(
                    development_config.c_str(),
                    backend_explicit,
                    bridge_host_configured,
                    true,
                    false
                );
            }
        }

        if (!loaded) {
            if (!load_file(
                    "salixweb32.local.ini",
                    backend_explicit,
                    bridge_host_configured,
                    true,
                    false
                )) {
                load_file(
                    "..\\..\\salixweb32.local.ini",
                    backend_explicit,
                    bridge_host_configured,
                    true,
                    false
                );
            }
        }
    }

    // Persistent user settings/state live in the startup-selected data root:
    // %APPDATA%\SalixWeb32 in standard mode, or beside the executable in
    // portable mode. Bridge configuration remains in the explicit
    // local/development configuration layer.
    if (!user_preferences_path.empty()) {
        load_file(
            user_preferences_path.c_str(),
            backend_explicit,
            bridge_host_configured,
            false,
            true
        );
    }

    const char* environment_host = getenv("SALIX_BRIDGE_HOST");
    if (
        environment_host != 0 &&
        environment_host[0] != '\0'
    ) {
        bridge_host = environment_host;
        bridge_host_configured = true;
    }

    const char* environment_port = getenv("SALIX_BRIDGE_PORT");
    if (
        environment_port != 0 &&
        environment_port[0] != '\0'
    ) {
        unsigned short parsed_port = bridge_port;
        if (parse_port(environment_port, parsed_port)) {
            bridge_port = parsed_port;
        }
    }

    const char* environment_backend = getenv("SALIX_WEB_BACKEND");
    if (
        environment_backend != 0 &&
        environment_backend[0] != '\0'
    ) {
        std::string backend = lowercase_copy(
            trim_copy(environment_backend)
        );

        if (backend == "remote") {
            use_remote_bridge = true;
            backend_explicit = true;
        } else if (backend == "placeholder") {
            use_remote_bridge = false;
            backend_explicit = true;
        }
    }

    if (!backend_explicit && bridge_host_configured) {
        use_remote_bridge = true;
    }
}

bool ApplicationSettings::save_user_preferences() const {
    if (user_preferences_path.empty()) {
        return false;
    }

    FILE* file = fopen(user_preferences_path.c_str(), "wt");
    if (file == 0) {
        return false;
    }

    bool result = true;

    if (fprintf(
            file,
            "# SalixWeb32 persistent user state\n"
            "# Diagnostics is user-configurable; attachment_directory is recent picker history.\n"
            "\n"
        ) < 0) {
        result = false;
    }

    if (
        result &&
        !write_setting(
            file,
            "diagnostics_directory",
            diagnostics_directory
        )
    ) {
        result = false;
    }

    if (
        result &&
        !write_setting(
            file,
            "attachment_directory",
            attachment_directory
        )
    ) {
        result = false;
    }

    if (fclose(file) != 0) {
        result = false;
    }

    return result;
}

bool ApplicationSettings::get_use_remote_bridge() const {
    return use_remote_bridge;
}

const char* ApplicationSettings::get_bridge_host() const {
    return bridge_host.c_str();
}

unsigned short ApplicationSettings::get_bridge_port() const {
    return bridge_port;
}

const char* ApplicationSettings::get_source_path() const {
    return source_path.c_str();
}

const char* ApplicationSettings::get_launch_directory() const {
    return launch_directory.c_str();
}

const char* ApplicationSettings::get_executable_directory() const {
    return executable_directory.c_str();
}

const char* ApplicationSettings::get_user_data_directory() const {
    return user_data_directory.c_str();
}

const char* ApplicationSettings::get_user_preferences_path() const {
    return user_preferences_path.c_str();
}

const char* ApplicationSettings::get_diagnostics_directory() const {
    return diagnostics_directory.c_str();
}

const char* ApplicationSettings::get_attachment_directory() const {
    return attachment_directory.c_str();
}

std::string ApplicationSettings::get_received_files_directory() const {
    std::string result(user_data_directory);

    if (!result.empty() && result[result.size() - 1] != '\\') {
        result += "\\";
    }

    result += "Received";
    return result;
}

bool ApplicationSettings::is_portable_mode() const {
    return portable_mode;
}

const char* ApplicationSettings::get_application_mode_name() const {
    return portable_mode
        ? "Portable (--portable)"
        : "Standard";
}

std::string ApplicationSettings::get_default_diagnostics_directory() const {
    std::string result(user_data_directory);

    if (!result.empty() && result[result.size() - 1] != '\\') {
        result += "\\";
    }

    result += "Diagnostics";
    return result;
}

std::string ApplicationSettings::get_default_attachment_directory() const {
    const char* user_profile = getenv("USERPROFILE");

    if (
        user_profile != 0 &&
        user_profile[0] != '\0'
    ) {
        std::string profile_directory =
            strip_trailing_separators(user_profile);

        if (!profile_directory.empty()) {
            return profile_directory;
        }
    }

    if (!launch_directory.empty()) {
        return launch_directory;
    }

    return executable_directory;
}

void ApplicationSettings::set_diagnostics_directory(
    const char* path
) {
    if (path == 0 || path[0] == '\0') {
        return;
    }

    diagnostics_directory =
        strip_trailing_separators(path);
}

void ApplicationSettings::set_attachment_directory(
    const char* path
) {
    if (path == 0 || path[0] == '\0') {
        return;
    }

    attachment_directory =
        strip_trailing_separators(path);
}

void ApplicationSettings::reset_file_locations() {
    diagnostics_directory =
        get_default_diagnostics_directory();

    attachment_directory =
        get_default_attachment_directory();
}

bool ApplicationSettings::load_file(
    const char* path,
    bool& backend_explicit,
    bool& bridge_host_configured,
    bool record_source_path,
    bool path_preferences_only
) {
    if (path == 0 || path[0] == '\0') {
        return false;
    }

    FILE* file = fopen(path, "rt");
    if (file == 0) {
        return false;
    }

    if (record_source_path) {
        source_path = path;
    }

    char line_buffer[1024];

    while (fgets(line_buffer, sizeof(line_buffer), file) != 0) {
        std::string line = trim_copy(line_buffer);

        if (
            line.empty() ||
            line[0] == '#' ||
            line[0] == ';'
        ) {
            continue;
        }

        std::string::size_type separator = line.find('=');
        if (separator == std::string::npos) {
            continue;
        }

        apply_key_value(
            trim_copy(line.substr(0, separator)),
            trim_copy(line.substr(separator + 1)),
            backend_explicit,
            bridge_host_configured,
            path_preferences_only
        );
    }

    fclose(file);
    return true;
}

void ApplicationSettings::apply_key_value(
    const std::string& key_text,
    const std::string& value_text,
    bool& backend_explicit,
    bool& bridge_host_configured,
    bool path_preferences_only
) {
    std::string key = lowercase_copy(trim_copy(key_text));
    std::string value = trim_copy(value_text);

    if (!path_preferences_only && key == "web_backend") {
        std::string backend = lowercase_copy(value);

        if (backend == "remote") {
            use_remote_bridge = true;
            backend_explicit = true;
        } else if (backend == "placeholder") {
            use_remote_bridge = false;
            backend_explicit = true;
        }

        return;
    }

    if (!path_preferences_only && key == "bridge_host") {
        if (!value.empty()) {
            bridge_host = value;
            bridge_host_configured = true;
        }

        return;
    }

    if (!path_preferences_only && key == "bridge_port") {
        unsigned short parsed_port = bridge_port;

        if (parse_port(value, parsed_port)) {
            bridge_port = parsed_port;
        }

        return;
    }

    if (key == "diagnostics_directory") {
        if (!value.empty()) {
            diagnostics_directory =
                strip_trailing_separators(value);
        }

        return;
    }

    if (key == "attachment_directory") {
        if (!value.empty()) {
            attachment_directory =
                strip_trailing_separators(value);
        }
    }
}

std::string ApplicationSettings::make_launch_relative_path(
    const char* relative_path
) const {
    if (
        launch_directory.empty() ||
        relative_path == 0 ||
        relative_path[0] == '\0'
    ) {
        return relative_path == 0
            ? std::string()
            : std::string(relative_path);
    }

    std::string result(launch_directory);

    if (result[result.size() - 1] != '\\') {
        result += "\\";
    }

    result += relative_path;
    return result;
}
