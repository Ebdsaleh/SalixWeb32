// =================================================================================
// Filename:    web/backends/RemoteBridgeWebBackend.h
// Author:      Ebdsaleh
// Description: Declares the first remote bridge backend over NetworkRequestExecutor.
// =================================================================================
#pragma once

#include <string>

#include "web/platform/WebPlatformBackend.h"
#include "web/platform/WebSurfaceSnapshot.h"

class NetworkRequestExecutor;
class NetworkResponse;
class WebInputEvent;
class WebNavigationRequest;

class RemoteBridgeWebBackend : public WebPlatformBackend {
    public:
        RemoteBridgeWebBackend(
            NetworkRequestExecutor* request_executor,
            const char* host,
            unsigned short port
        );

        virtual const char* get_name() const;
        virtual WebBackendFamily get_family() const;

        virtual bool initialize();
        virtual void update();
        virtual void shutdown();
        virtual bool get_is_initialized() const;

        virtual void get_capabilities(
            WebBackendCapabilities& capabilities
        ) const;

        virtual bool navigate(
            const WebNavigationRequest& request
        );

        virtual unsigned long get_surface_revision() const;

        virtual bool get_surface_snapshot(
            WebSurfaceSnapshot& snapshot
        ) const;

        virtual bool handle_input(
            const WebInputEvent& event
        );

    private:
        void apply_network_response(const NetworkResponse& response);
        void set_transport_failure(
            const char* operation,
            const char* detail
        );
        void refresh_ready_surface();
        void mark_surface_changed();

        NetworkRequestExecutor* request_executor;
        std::string host;
        unsigned short port;
        bool is_initialized;
        bool bridge_online;
        unsigned long request_count;
        unsigned long surface_revision;
        std::string current_url;
        WebSurfaceSnapshot surface_snapshot;
};
