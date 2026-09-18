// =================================================================================
// Filename:    web/backends/PlaceholderWebBackend.cpp
// Author:      Ebdsaleh
// Description: Implements a no-network backend used to validate the WebView contract.
// =================================================================================

#include <stdio.h>
#include <string>

#include "PlaceholderWebBackend.h"
#include "web/platform/WebInputEvent.h"
#include "web/platform/WebNavigationRequest.h"

PlaceholderWebBackend::PlaceholderWebBackend()
    : is_initialized(false),
      update_count(0),
      input_event_count(0),
      surface_revision(0),
      current_url("about:blank") {
    refresh_surface_text();
}

const char* PlaceholderWebBackend::get_name() const {
    return "Placeholder Web Backend";
}

WebBackendFamily PlaceholderWebBackend::get_family() const {
    return web_backend_placeholder;
}

bool PlaceholderWebBackend::initialize() {
    if (is_initialized) {
        return true;
    }

    is_initialized = true;
    update_count = 0;
    input_event_count = 0;
    refresh_surface_text();
    return true;
}

void PlaceholderWebBackend::update() {
    if (!is_initialized) {
        return;
    }

    ++update_count;
    refresh_surface_text();
}

void PlaceholderWebBackend::shutdown() {
    is_initialized = false;
    refresh_surface_text();
}

bool PlaceholderWebBackend::get_is_initialized() const {
    return is_initialized;
}

void PlaceholderWebBackend::get_capabilities(
    WebBackendCapabilities& capabilities
) const {
    capabilities = WebBackendCapabilities();
    capabilities.navigation = true;
    capabilities.surface_snapshot = true;
    capabilities.pointer_input = true;
    capabilities.keyboard_input = true;
}

bool PlaceholderWebBackend::navigate(
    const WebNavigationRequest& request
) {
    if (!is_initialized || request.empty()) {
        return false;
    }

    current_url = request.get_url();
    refresh_surface_text();
    return true;
}

unsigned long PlaceholderWebBackend::get_surface_revision() const {
    return surface_revision;
}

bool PlaceholderWebBackend::get_surface_snapshot(
    WebSurfaceSnapshot& snapshot
) const {
    snapshot = surface_snapshot;
    return true;
}

bool PlaceholderWebBackend::handle_input(
    const WebInputEvent& event
) {
    if (!is_initialized || event.type == WebInputEvent::input_none) {
        return false;
    }

    ++input_event_count;
    refresh_surface_text();
    return true;
}

void PlaceholderWebBackend::refresh_surface_text() {
    char status_text[192];
    char counter_text[160];

    surface_snapshot.title = "SalixWeb32 WebView contract";
    surface_snapshot.address = current_url;

    sprintf(
        status_text,
        "Backend: %s | state: %s",
        get_name(),
        is_initialized ? "initialized" : "stopped"
    );
    surface_snapshot.status = status_text;

    sprintf(
        counter_text,
        "Runtime ticks: %lu | forwarded input events: %lu",
        update_count,
        input_event_count
    );

    std::string content(
        "The generic WebView/backend boundary is active.\n\n"
        "Navigation target: "
    );
    content += current_url;
    content +=
        "\n\nThis placeholder intentionally performs no network request, HTML parsing, "
        "JavaScript execution, authentication, or remote service access.\n\n";
    content += counter_text;

    surface_snapshot.content = content;
    ++surface_revision;
}
