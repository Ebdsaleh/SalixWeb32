// =================================================================================
// Filename:    runtime/ApplicationRuntime.h
// Author:      Ebdsaleh
// Description: Declares the application runtime lifecycle coordinator.
// =================================================================================
#pragma once

#include "ServiceRegistry.h"
#include "RuntimeStatusService.h"

class ApplicationRuntime {
    public:
        ApplicationRuntime();

        bool initialize();
        void update();
        void shutdown();

        bool get_is_initialized() const;
        int get_service_count() const;
        unsigned long get_update_count() const;
        ServiceRegistry& get_services();

    private:
        bool register_core_services();

        ServiceRegistry service_registry;
        RuntimeStatusService runtime_status_service;
        bool core_services_registered;
        bool is_initialized;
};
