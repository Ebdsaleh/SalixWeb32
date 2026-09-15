// =================================================================================
// Filename:    runtime/ApplicationRuntime.h
// Author:      Ebdsaleh
// Description: Declares the application runtime lifecycle coordinator.
// =================================================================================
#pragma once

#include "ServiceRegistry.h"

class ApplicationRuntime {
    public:
        ApplicationRuntime();

        bool initialize();
        void update();
        void shutdown();

        bool get_is_initialized() const;
        ServiceRegistry& get_services();

    private:
        ServiceRegistry service_registry;
        bool is_initialized;
};
