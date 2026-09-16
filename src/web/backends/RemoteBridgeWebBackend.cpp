// =================================================================================
// Filename:    web/backends/RemoteBridgeWebBackend.cpp
// Author:      Ebdsaleh
// Description: Implements the first remote bridge backend over NetworkTransport.
// =================================================================================

#include <stdio.h>

#include "RemoteBridgeWebBackend.h"
#include "web/network/NetworkRequest.h"
#include "web/network/NetworkResponse.h"
#include "web/network/NetworkTransport.h"
#include "web/platform/WebInputEvent.h"
#include "web/platform/WebNavigationRequest.h"

RemoteBridgeWebBackend::RemoteBridgeWebBackend(
    NetworkTransport* new_transport,
    const char* new_host,
    unsigned short new_port
) : transport(new_transport),
    host(new_host == 0 ? "" : new_host),
    port(new_port),
    is_initialized(false),
    bridge_online(false),
    request_count(0),
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

    if (transport == 0 || host.empty() || port == 0) {
        surface_snapshot.title = "SalixWeb32 Remote Bridge";
        surface_snapshot.address = current_url;
        surface_snapshot.status = "Remote bridge configuration is incomplete.";
        surface_snapshot.content =
            "Set SALIX_BRIDGE_HOST and optionally SALIX_BRIDGE_PORT before "
            "selecting the remote backend.";
        return false;
    }

    if (!transport->initialize()) {
        set_transport_failure("Transport initialization");
        return false;
    }

    is_initialized = true;
    bridge_online = false;
    request_count = 0;
    refresh_ready_surface();
    return true;
}

void RemoteBridgeWebBackend::update() {
    // First transport tranche intentionally avoids polling the bridge every
    // frame. Requests are explicit so a missing companion cannot stall the UI.
}

void RemoteBridgeWebBackend::shutdown() {
    if (transport != 0 && transport->get_is_initialized()) {
        transport->shutdown();
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

    // These remain false until the companion protocol actually supplies them.
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
    if (!is_initialized || transport == 0 || request.empty()) {
        return false;
    }

    current_url = request.get_url();

    NetworkRequest network_request("POST", "/v1/navigate");
    network_request.set_content_type("text/plain; charset=utf-8");
    network_request.set_body(current_url);

    NetworkResponse response;
    ++request_count;

    if (!transport->send(
            host.c_str(),
            port,
            network_request,
            response
        )) {
        bridge_online = false;
        set_transport_failure("Navigation request");
        return false;
    }

    bridge_online = response.get_is_success();

    char number_text[64];
    std::string status_text("Bridge ");
    status_text += host;
    status_text += ":";
    sprintf(number_text, "%u", (unsigned int)port);
    status_text += number_text;
    status_text += " returned HTTP ";
    sprintf(number_text, "%d", response.get_status_code());
    status_text += number_text;
    status_text += " after request ";
    sprintf(number_text, "%lu", request_count);
    status_text += number_text;
    status_text += ".";

    surface_snapshot.title = "SalixWeb32 Remote Bridge";
    surface_snapshot.address = current_url;
    surface_snapshot.status = status_text;
    surface_snapshot.content = response.get_body();

    return response.get_is_success();
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

void RemoteBridgeWebBackend::set_transport_failure(
    const char* operation
) {
    std::string status(operation == 0 ? "Transport request" : operation);
    status += " failed: ";

    if (transport != 0 && transport->get_last_error() != 0) {
        status += transport->get_last_error();
    } else {
        status += "unknown transport error";
    }

    surface_snapshot.title = "SalixWeb32 Remote Bridge";
    surface_snapshot.address = current_url;
    surface_snapshot.status = status;
    surface_snapshot.content =
        "The remote backend remains isolated from the application shell. "
        "Fix the companion endpoint and retry without changing WebView code.";
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
        "\n\nThe first bridge protocol forwards navigation requests only. It does "
        "not yet fetch pages, authenticate to services, upload files, or expose "
        "modern TLS directly to the legacy machine.";

    surface_snapshot.title = "SalixWeb32 Remote Bridge";
    surface_snapshot.address = current_url;
    surface_snapshot.status = is_initialized
        ? "Remote bridge transport initialized."
        : "Remote bridge transport stopped.";
    surface_snapshot.content = endpoint_text;
}
