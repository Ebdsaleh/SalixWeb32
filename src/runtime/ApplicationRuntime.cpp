// =================================================================================
// Filename:    runtime/ApplicationRuntime.cpp
// Author:      Ebdsaleh
// Description: Implements the application runtime lifecycle coordinator.
// =================================================================================

#include "ApplicationRuntime.h"

ApplicationRuntime::ApplicationRuntime()
    : is_initialized(false) {
}

bool ApplicationRuntime::initialize() {
    if (is_initialized) {
        return true;
    }

    if (!service_registry.start_all()) {
        return false;
    }

    is_initialized = true;
    return true;
}

void ApplicationRuntime::update() {
    if (!is_initialized) {
        return;
    }

    service_registry.update_all();
}

void ApplicationRuntime::shutdown() {
    if (!is_initialized) {
        return;
    }

    service_registry.stop_all();
    is_initialized = false;
}

bool ApplicationRuntime::get_is_initialized() const {
    return is_initialized;
}

ServiceRegistry& ApplicationRuntime::get_services() {
    return service_registry;
}
