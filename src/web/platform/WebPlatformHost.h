// =================================================================================
// Filename:    web/platform/WebPlatformHost.h
// Author:      Ebdsaleh
// Description: Selects one web backend and owns its runtime lifecycle boundary.
// =================================================================================
#pragma once

#include "WebBackendCapabilities.h"

class WebInputEvent;
class WebNavigationRequest;
class WebPlatformBackend;
class WebSurfaceSnapshot;

class WebPlatformHost {
    public:
        WebPlatformHost();

        bool set_backend(WebPlatformBackend* backend);
        WebPlatformBackend* get_backend();
        const WebPlatformBackend* get_backend() const;

        bool initialize();
        void update();
        void shutdown();

        bool get_is_initialized() const;
        bool has_backend() const;

        const char* get_backend_name() const;
        WebBackendFamily get_backend_family() const;

        void get_capabilities(
            WebBackendCapabilities& capabilities
        ) const;

        bool navigate(const WebNavigationRequest& request);
        unsigned long get_surface_revision() const;
        bool get_surface_snapshot(WebSurfaceSnapshot& snapshot) const;
        bool handle_input(const WebInputEvent& event);

    private:
        WebPlatformBackend* backend;
        bool is_initialized;
};
