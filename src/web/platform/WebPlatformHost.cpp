// =================================================================================
// Filename:    web/platform/WebPlatformHost.cpp
// Author:      Ebdsaleh
// Description: Implements web backend selection and lifecycle coordination.
// =================================================================================

#include "WebPlatformHost.h"
#include "WebInputEvent.h"
#include "WebNavigationRequest.h"
#include "WebPlatformBackend.h"
#include "WebSurfaceSnapshot.h"

WebPlatformHost::WebPlatformHost()
    : backend(0),
      is_initialized(false) {
}

bool WebPlatformHost::set_backend(WebPlatformBackend* new_backend) {
    if (is_initialized) {
        return false;
    }

    backend = new_backend;
    return true;
}

WebPlatformBackend* WebPlatformHost::get_backend() {
    return backend;
}

const WebPlatformBackend* WebPlatformHost::get_backend() const {
    return backend;
}

bool WebPlatformHost::initialize() {
    if (is_initialized) {
        return true;
    }

    if (backend == 0) {
        return true;
    }

    if (!backend->initialize()) {
        return false;
    }

    is_initialized = true;
    return true;
}

void WebPlatformHost::update() {
    if (backend == 0 || !is_initialized) {
        return;
    }

    backend->update();
}

void WebPlatformHost::shutdown() {
    if (backend != 0 && is_initialized) {
        backend->shutdown();
    }

    is_initialized = false;
}

bool WebPlatformHost::get_is_initialized() const {
    return is_initialized;
}

bool WebPlatformHost::has_backend() const {
    return backend != 0;
}

const char* WebPlatformHost::get_backend_name() const {
    return backend == 0 ? "none" : backend->get_name();
}

WebBackendFamily WebPlatformHost::get_backend_family() const {
    return backend == 0
        ? web_backend_placeholder
        : backend->get_family();
}

void WebPlatformHost::get_capabilities(
    WebBackendCapabilities& capabilities
) const {
    capabilities = WebBackendCapabilities();

    if (backend != 0) {
        backend->get_capabilities(capabilities);
    }
}

bool WebPlatformHost::navigate(
    const WebNavigationRequest& request
) {
    if (
        backend == 0 ||
        !is_initialized ||
        request.empty()
    ) {
        return false;
    }

    return backend->navigate(request);
}

unsigned long WebPlatformHost::get_surface_revision() const {
    return backend == 0 ? 0UL : backend->get_surface_revision();
}

bool WebPlatformHost::get_surface_snapshot(
    WebSurfaceSnapshot& snapshot
) const {
    snapshot.clear();

    if (backend == 0) {
        return false;
    }

    return backend->get_surface_snapshot(snapshot);
}

bool WebPlatformHost::handle_input(
    const WebInputEvent& event
) {
    if (backend == 0 || !is_initialized) {
        return false;
    }

    return backend->handle_input(event);
}
