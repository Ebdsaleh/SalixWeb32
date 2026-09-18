// =================================================================================
// Filename:    web/backends/RemoteBridgeWebBackend.cpp
// Author:      Ebdsaleh
// Description: Implements the remote bridge backend over NetworkRequestExecutor.
// =================================================================================

#include <stdio.h>
#include <stdlib.h>

#include "RemoteBridgeWebBackend.h"
#include "web/network/NetworkRequest.h"
#include "web/network/NetworkResponse.h"
#include "web/network/NetworkRequestExecutor.h"
#include "web/platform/WebInputEvent.h"
#include "web/platform/WebNavigationRequest.h"

namespace {
    std::string get_probe_value(
        const std::string& metadata,
        const char* key
    ) {
        if (key == 0 || key[0] == '\0') {
            return "";
        }

        std::string prefix(key);
        prefix += "=";

        std::string::size_type position = metadata.find(prefix);
        while (position != std::string::npos) {
            if (position == 0 || metadata[position - 1] == '\n') {
                std::string::size_type value_start = position + prefix.size();
                std::string::size_type value_end = metadata.find('\n', value_start);
                if (value_end == std::string::npos) {
                    value_end = metadata.size();
                }
                return metadata.substr(value_start, value_end - value_start);
            }
            position = metadata.find(prefix, position + 1);
        }

        return "";
    }

    unsigned long get_probe_unsigned(
        const std::string& metadata,
        const char* key
    ) {
        std::string value = get_probe_value(metadata, key);
        if (value.empty()) {
            return 0;
        }
        return strtoul(value.c_str(), 0, 10);
    }

    int get_probe_int(
        const std::string& metadata,
        const char* key
    ) {
        std::string value = get_probe_value(metadata, key);
        if (value.empty()) {
            return 0;
        }
        return atoi(value.c_str());
    }

    bool read_probe_section(
        const std::string& payload,
        std::string::size_type& cursor,
        unsigned long length,
        std::string& output
    ) {
        if (length > (unsigned long)(payload.size() - cursor)) {
            return false;
        }

        output.assign(payload, cursor, (std::string::size_type)length);
        cursor += (std::string::size_type)length;
        return true;
    }

    bool parse_probe_payload(
        const std::string& payload,
        WebSurfaceSnapshot& snapshot
    ) {
        const char* protocol = "SALIX-PROBE/1";
        std::string::size_type header_end = payload.find("\n\n");
        if (header_end == std::string::npos) {
            return false;
        }

        std::string metadata = payload.substr(0, header_end);
        if (metadata.compare(0, 13, protocol) != 0) {
            return false;
        }

        if (get_probe_value(metadata, "status") != "ok") {
            return false;
        }

        unsigned long requested_url_length =
            get_probe_unsigned(metadata, "requested_url_len");
        unsigned long final_url_length =
            get_probe_unsigned(metadata, "final_url_len");
        unsigned long reason_length =
            get_probe_unsigned(metadata, "http_reason_len");
        unsigned long mime_length =
            get_probe_unsigned(metadata, "mime_type_len");
        unsigned long headers_length =
            get_probe_unsigned(metadata, "headers_len");
        unsigned long raw_length =
            get_probe_unsigned(metadata, "raw_len");
        unsigned long extracted_length =
            get_probe_unsigned(metadata, "extracted_len");
        unsigned long title_length =
            get_probe_unsigned(metadata, "title_len");

        std::string requested_url;
        std::string final_url;
        std::string reason;
        std::string mime_type;
        std::string headers;
        std::string raw_content;
        std::string extracted_content;
        std::string title;

        std::string::size_type cursor = header_end + 2;
        if (!read_probe_section(payload, cursor, requested_url_length, requested_url) ||
            !read_probe_section(payload, cursor, final_url_length, final_url) ||
            !read_probe_section(payload, cursor, reason_length, reason) ||
            !read_probe_section(payload, cursor, mime_length, mime_type) ||
            !read_probe_section(payload, cursor, headers_length, headers) ||
            !read_probe_section(payload, cursor, raw_length, raw_content) ||
            !read_probe_section(payload, cursor, extracted_length, extracted_content) ||
            !read_probe_section(payload, cursor, title_length, title)) {
            return false;
        }

        snapshot.clear();
        snapshot.title = title.empty() ? "Salix Browser Probe" : title;
        snapshot.address = requested_url;
        snapshot.final_address = final_url;
        snapshot.http_status_code = get_probe_int(metadata, "http_status");
        snapshot.http_status_text = reason;
        snapshot.mime_type = mime_type;
        snapshot.response_size = get_probe_unsigned(metadata, "response_size");
        snapshot.response_truncated =
            get_probe_int(metadata, "truncated") != 0;
        snapshot.redirect_count = get_probe_int(metadata, "redirect_count");
        snapshot.script_count = get_probe_int(metadata, "script_count");
        snapshot.form_count = get_probe_int(metadata, "form_count");
        snapshot.link_count = get_probe_int(metadata, "link_count");
        snapshot.response_headers = headers;
        snapshot.raw_content = raw_content;
        snapshot.extracted_content = extracted_content;

        char number_text[64];
        std::string status("HTTP ");
        sprintf(number_text, "%d", snapshot.http_status_code);
        status += number_text;
        if (!reason.empty()) {
            status += " ";
            status += reason;
        }
        status += " | ";
        status += mime_type.empty() ? "unknown MIME" : mime_type;
        status += " | ";
        sprintf(number_text, "%lu", snapshot.response_size);
        status += number_text;
        status += " bytes";
        if (snapshot.response_truncated) {
            status += " captured (truncated)";
        }
        snapshot.status = status;

        std::string summary;
        summary += "Requested: ";
        summary += requested_url;
        summary += "\nFinal: ";
        summary += final_url.empty() ? requested_url : final_url;
        summary += "\nHTTP: ";
        sprintf(number_text, "%d", snapshot.http_status_code);
        summary += number_text;
        if (!reason.empty()) {
            summary += " ";
            summary += reason;
        }
        summary += "\nMIME: ";
        summary += mime_type.empty() ? "unknown" : mime_type;
        summary += "\nCaptured bytes: ";
        sprintf(number_text, "%lu", snapshot.response_size);
        summary += number_text;
        summary += snapshot.response_truncated ? " (truncated)" : "";
        summary += "\nRedirects: ";
        sprintf(number_text, "%d", snapshot.redirect_count);
        summary += number_text;
        summary += "\nHTML signals: scripts ";
        sprintf(number_text, "%d", snapshot.script_count);
        summary += number_text;
        summary += " | forms ";
        sprintf(number_text, "%d", snapshot.form_count);
        summary += number_text;
        summary += " | links ";
        sprintf(number_text, "%d", snapshot.link_count);
        summary += number_text;
        summary +=
            "\n\nProbe mode fetches and inspects the response only. It does not "
            "execute JavaScript or authenticate to the target service.";
        snapshot.content = summary;

        return true;
    }
}

RemoteBridgeWebBackend::RemoteBridgeWebBackend(
    NetworkRequestExecutor* new_request_executor,
    const char* new_host,
    unsigned short new_port
) : request_executor(new_request_executor),
    host(new_host == 0 ? "" : new_host),
    port(new_port),
    is_initialized(false),
    bridge_online(false),
    request_count(0),
    surface_revision(0),
    current_url("about:blank") {
    refresh_ready_surface();
}

const char* RemoteBridgeWebBackend::get_name() const {
    return "Remote Bridge Web Backend";
}

WebBackendFamily RemoteBridgeWebBackend::get_family() const {
    return web_backend_remote;
}

bool RemoteBridgeWebBackend::initialize() {
    if (is_initialized) {
        return true;
    }

    if (request_executor == 0 || host.empty() || port == 0) {
        surface_snapshot.clear();
        surface_snapshot.title = "Salix Browser Probe";
        surface_snapshot.address = current_url;
        surface_snapshot.status = "Remote bridge configuration is incomplete.";
        surface_snapshot.content =
            "Configure the bridge host and port before selecting the remote backend.";
        mark_surface_changed();
        return false;
    }

    if (!request_executor->initialize()) {
        set_transport_failure(
            "Network executor initialization",
            request_executor->get_last_error()
        );
        return false;
    }

    is_initialized = true;
    bridge_online = false;
    request_count = 0;
    refresh_ready_surface();
    return true;
}

void RemoteBridgeWebBackend::update() {
    if (!is_initialized || request_executor == 0) {
        return;
    }

    NetworkResponse response;
    std::string error_text;
    bool succeeded = false;

    if (!request_executor->take_result(
            response,
            error_text,
            succeeded
        )) {
        return;
    }

    if (!succeeded) {
        bridge_online = false;
        set_transport_failure(
            "Browser Probe request",
            error_text.c_str()
        );
        return;
    }

    bridge_online = true;
    apply_network_response(response);
}

void RemoteBridgeWebBackend::shutdown() {
    if (
        request_executor != 0 &&
        request_executor->get_is_initialized()
    ) {
        request_executor->shutdown();
    }

    is_initialized = false;
    bridge_online = false;
    refresh_ready_surface();
}

bool RemoteBridgeWebBackend::get_is_initialized() const {
    return is_initialized;
}

void RemoteBridgeWebBackend::get_capabilities(
    WebBackendCapabilities& capabilities
) const {
    capabilities = WebBackendCapabilities();
    capabilities.navigation = true;
    capabilities.surface_snapshot = true;
    capabilities.network = true;

    // Browser Probe can retrieve and inspect HTML, but it does not yet expose
    // a DOM/runtime to Salix, so HTML/JS capabilities remain conservative.
    capabilities.pointer_input = false;
    capabilities.keyboard_input = false;
    capabilities.html = false;
    capabilities.css = false;
    capabilities.javascript = false;
    capabilities.websocket = false;
    capabilities.file_upload = false;
}

bool RemoteBridgeWebBackend::navigate(
    const WebNavigationRequest& request
) {
    if (
        !is_initialized ||
        request_executor == 0 ||
        request.empty()
    ) {
        return false;
    }

    if (request_executor->get_is_busy()) {
        return false;
    }

    current_url = request.get_url();

    NetworkRequest network_request("POST", "/v1/fetch");
    network_request.set_content_type("text/plain; charset=utf-8");
    network_request.set_body(current_url);

    if (!request_executor->submit(
            host.c_str(),
            port,
            network_request
        )) {
        set_transport_failure(
            "Browser Probe request",
            request_executor->get_last_error()
        );
        return false;
    }

    ++request_count;

    // Navigation is now queued only.  The blocking transport work runs behind
    // NetworkRequestExecutor, while update() consumes completion on the main
    // application thread.
    return true;
}

unsigned long RemoteBridgeWebBackend::get_surface_revision() const {
    return surface_revision;
}

bool RemoteBridgeWebBackend::get_surface_snapshot(
    WebSurfaceSnapshot& snapshot
) const {
    snapshot = surface_snapshot;
    return true;
}

bool RemoteBridgeWebBackend::handle_input(
    const WebInputEvent& event
) {
    (void)event;
    return false;
}

void RemoteBridgeWebBackend::apply_network_response(
    const NetworkResponse& response
) {
    if (!response.get_is_success()) {
        char number_text[64];
        std::string status("Browser Probe bridge returned HTTP ");
        sprintf(number_text, "%d", response.get_status_code());
        status += number_text;
        status += ".";

        surface_snapshot.clear();
        surface_snapshot.title = "Salix Browser Probe";
        surface_snapshot.address = current_url;
        surface_snapshot.status = status;
        surface_snapshot.content = response.get_body();
        mark_surface_changed();
        return;
    }

    if (!parse_probe_payload(response.get_body(), surface_snapshot)) {
        surface_snapshot.clear();
        surface_snapshot.title = "Salix Browser Probe";
        surface_snapshot.address = current_url;
        surface_snapshot.status =
            "Browser Probe returned an invalid probe payload.";
        surface_snapshot.content = response.get_body();
        mark_surface_changed();
        return;
    }

    mark_surface_changed();
}

void RemoteBridgeWebBackend::set_transport_failure(
    const char* operation,
    const char* detail
) {
    std::string status(operation == 0 ? "Transport request" : operation);
    status += " failed: ";

    if (detail != 0 && detail[0] != '\0') {
        status += detail;
    } else {
        status += "unknown transport error";
    }

    surface_snapshot.clear();
    surface_snapshot.title = "Salix Browser Probe";
    surface_snapshot.address = current_url;
    surface_snapshot.status = status;
    surface_snapshot.content =
        "The Browser Probe could not reach the companion. The application shell "
        "remains isolated; fix the bridge endpoint and retry.";
    mark_surface_changed();
}

void RemoteBridgeWebBackend::refresh_ready_surface() {
    char port_text[16];
    sprintf(port_text, "%u", (unsigned int)port);

    std::string endpoint_text("Remote transport endpoint: ");
    endpoint_text += host.empty() ? "(not configured)" : host;
    endpoint_text += ":";
    endpoint_text += port_text;
    endpoint_text += "\n\nState: ";
    endpoint_text += bridge_online
        ? "online"
        : "ready / not yet contacted";
    endpoint_text +=
        "\n\nEnter an http:// or https:// URL in the Browser tab and press Go. "
        "The modern companion will perform an unauthenticated GET and return "
        "diagnostic response data. No cookies or credentials are forwarded.";

    surface_snapshot.clear();
    surface_snapshot.title = "Salix Browser Probe";
    surface_snapshot.address = current_url;
    surface_snapshot.status = is_initialized
        ? "Remote Browser Probe transport initialized."
        : "Remote Browser Probe transport stopped.";
    surface_snapshot.content = endpoint_text;
    mark_surface_changed();
}

void RemoteBridgeWebBackend::mark_surface_changed() {
    ++surface_revision;

    if (surface_revision == 0) {
        surface_revision = 1;
    }
}
