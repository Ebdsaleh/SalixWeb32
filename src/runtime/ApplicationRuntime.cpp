// =================================================================================
// Filename:    runtime/ApplicationRuntime.cpp
// Author:      Ebdsaleh
// Description: Implements the application runtime lifecycle coordinator.
// =================================================================================

#include "ApplicationRuntime.h"
#include "Diagnostics.h"

ApplicationRuntime::ApplicationRuntime()
    : core_services_registered(false),
      is_initialized(false) {
}

bool ApplicationRuntime::initialize() {
    if (is_initialized) {
        return true;
    }

    if (!register_core_services()) {
        Diagnostics::write_line("ApplicationRuntime: core service registration failed.");
        return false;
    }

    if (!service_registry.start_all()) {
        Diagnostics::write_line("ApplicationRuntime: service startup failed.");
        return false;
    }

    is_initialized = true;
    Diagnostics::write_line("ApplicationRuntime initialized.");
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
    Diagnostics::write_line("ApplicationRuntime shut down.");
}

bool ApplicationRuntime::get_is_initialized() const {
    return is_initialized;
}

int ApplicationRuntime::get_service_count() const {
    return service_registry.get_count();
}

unsigned long ApplicationRuntime::get_update_count() const {
    return runtime_status_service.get_update_count();
}

ServiceRegistry& ApplicationRuntime::get_services() {
    return service_registry;
}

bool ApplicationRuntime::register_core_services() {
    if (core_services_registered) {
        return true;
    }

    if (!service_registry.add_service(&runtime_status_service)) {
        return false;
    }

    core_services_registered = true;
    return true;
}
