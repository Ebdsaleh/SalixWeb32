// =================================================================================
// Filename:    web/backends/RemoteBridgeWebBackend.h
// Author:      Ebdsaleh
// Description: Declares the first remote bridge backend over NetworkTransport.
// =================================================================================
#pragma once

#include <string>

#include "web/platform/WebPlatformBackend.h"
#include "web/platform/WebSurfaceSnapshot.h"

class NetworkTransport;
class WebInputEvent;
class WebNavigationRequest;

class RemoteBridgeWebBackend : public WebPlatformBackend {
    public:
        RemoteBridgeWebBackend(
            NetworkTransport* transport,
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

        virtual bool get_surface_snapshot(
            WebSurfaceSnapshot& snapshot
        ) const;

        virtual bool handle_input(
            const WebInputEvent& event
        );

    private:
        void set_transport_failure(const char* operation);
        void refresh_ready_surface();

        NetworkTransport* transport;
        std::string host;
        unsigned short port;
        bool is_initialized;
        bool bridge_online;
        unsigned long request_count;
        std::string current_url;
        WebSurfaceSnapshot surface_snapshot;
};
