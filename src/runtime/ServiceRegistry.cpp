// =================================================================================
// Filename:    runtime/ServiceRegistry.cpp
// Author:      Ebdsaleh
// Description: Implements runtime service registration and lifecycle dispatch.
// =================================================================================

#include "ServiceRegistry.h"

ServiceRegistry::ServiceRegistry()
    : service_count(0),
      is_started(false) {

    for (int index = 0; index < max_services; ++index) {
        services[index] = 0;
    }
}

bool ServiceRegistry::add_service(Service* service) {
    if (service == 0 || is_started || service_count >= max_services) {
        return false;
    }

    services[service_count] = service;
    ++service_count;
    return true;
}

bool ServiceRegistry::start_all() {
    if (is_started) {
        return true;
    }

    for (int index = 0; index < service_count; ++index) {
        if (!services[index]->start()) {
            for (int rollback_index = index - 1; rollback_index >= 0; --rollback_index) {
                services[rollback_index]->stop();
            }
            return false;
        }
    }

    is_started = true;
    return true;
}

void ServiceRegistry::update_all() {
    if (!is_started) {
        return;
    }

    for (int index = 0; index < service_count; ++index) {
        services[index]->update();
    }
}

void ServiceRegistry::stop_all() {
    if (!is_started) {
        return;
    }

    for (int index = service_count - 1; index >= 0; --index) {
        services[index]->stop();
    }

    is_started = false;
}

int ServiceRegistry::get_count() const {
    return service_count;
}
