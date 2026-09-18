// =================================================================================
// Filename:    app/ApplicationSettings.cpp
// Author:      Ebdsaleh
// Description: Loads environment and machine-local SalixWeb32 startup settings.
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
}

ApplicationSettings::ApplicationSettings()
    : use_remote_bridge(false),
      bridge_host("127.0.0.1"),
      bridge_port(8765) {
}

void ApplicationSettings::load() {
    use_remote_bridge = false;
    bridge_host = "127.0.0.1";
    bridge_port = 8765;
    source_path.clear();

    bool backend_explicit = false;
    bool bridge_host_configured = false;

    const char* config_path = getenv("SALIX_CONFIG");
    if (config_path != 0 && config_path[0] != '\0') {
        load_file(
            config_path,
            backend_explicit,
            bridge_host_configured
        );
    } else if (!load_file(
            "salixweb32.local.ini",
            backend_explicit,
            bridge_host_configured
        )) {
        load_file(
            "..\\..\\salixweb32.local.ini",
            backend_explicit,
            bridge_host_configured
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

bool ApplicationSettings::load_file(
    const char* path,
    bool& backend_explicit,
    bool& bridge_host_configured
) {
    if (path == 0 || path[0] == '\0') {
        return false;
    }

    FILE* file = fopen(path, "rt");
    if (file == 0) {
        return false;
    }

    source_path = path;

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
            bridge_host_configured
        );
    }

    fclose(file);
    return true;
}

void ApplicationSettings::apply_key_value(
    const std::string& key_text,
    const std::string& value_text,
    bool& backend_explicit,
    bool& bridge_host_configured
) {
    std::string key = lowercase_copy(trim_copy(key_text));
    std::string value = trim_copy(value_text);

    if (key == "web_backend") {
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

    if (key == "bridge_host") {
        if (!value.empty()) {
            bridge_host = value;
            bridge_host_configured = true;
        }

        return;
    }

    if (key == "bridge_port") {
        unsigned short parsed_port = bridge_port;

        if (parse_port(value, parsed_port)) {
            bridge_port = parsed_port;
        }
    }
}
