// =================================================================================
// Filename:    conversation/ConversationServiceHost.cpp
// Author:      Ebdsaleh
// Description: Implements semantic conversation backend lifecycle coordination.
// =================================================================================

#include "ConversationServiceHost.h"
#include "ConversationEvent.h"
#include "ConversationRequest.h"
#include "ConversationServiceBackend.h"

ConversationServiceHost::ConversationServiceHost()
    : backend(0),
      is_initialized(false),
      next_request_id(1) {
}

bool ConversationServiceHost::set_backend(
    ConversationServiceBackend* new_backend
) {
    if (is_initialized) {
        return false;
    }

    backend = new_backend;
    return true;
}

ConversationServiceBackend* ConversationServiceHost::get_backend() {
    return backend;
}

const ConversationServiceBackend* ConversationServiceHost::get_backend() const {
    return backend;
}

bool ConversationServiceHost::initialize() {
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

void ConversationServiceHost::update() {
    if (backend == 0 || !is_initialized) {
        return;
    }

    backend->update();
}

void ConversationServiceHost::shutdown() {
    if (backend != 0 && is_initialized) {
        backend->shutdown();
    }

    is_initialized = false;
}

bool ConversationServiceHost::get_is_initialized() const {
    return is_initialized;
}

bool ConversationServiceHost::has_backend() const {
    return backend != 0;
}

const char* ConversationServiceHost::get_backend_name() const {
    return backend == 0 ? "none" : backend->get_name();
}

const char* ConversationServiceHost::get_backend_status_text() const {
    return backend == 0
        ? "unavailable"
        : backend->get_status_text();
}

unsigned long ConversationServiceHost::submit_request(
    const ConversationRequest& request
) {
    if (
        backend == 0 ||
        !is_initialized ||
        request.empty()
    ) {
        return 0;
    }

    unsigned long request_id = next_request_id;
    ++next_request_id;

    if (next_request_id == 0) {
        next_request_id = 1;
    }

    if (!backend->submit_request(request, request_id)) {
        return 0;
    }

    return request_id;
}

bool ConversationServiceHost::take_event(ConversationEvent& event) {
    event.clear();

    if (backend == 0 || !is_initialized) {
        return false;
    }

    return backend->take_event(event);
}
