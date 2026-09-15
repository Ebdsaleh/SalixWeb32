// =================================================================================
// Filename:    runtime/ServiceRegistry.h
// Author:      Ebdsaleh
// Description: Declares a small fixed-capacity registry for runtime services.
// =================================================================================
#pragma once

#include "Service.h"

class ServiceRegistry {
    public:
        enum { max_services = 32 };

        ServiceRegistry();

        bool add_service(Service* service);
        bool start_all();
        void update_all();
        void stop_all();

        int get_count() const;
        bool get_is_started() const;

    private:
        Service* services[max_services];
        int service_count;
        bool is_started;
};
