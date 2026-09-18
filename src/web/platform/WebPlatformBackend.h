// =================================================================================
// Filename:    web/platform/WebPlatformBackend.h
// Author:      Ebdsaleh
// Description: Declares the interchangeable web-platform backend contract.
// =================================================================================
#pragma once

#include "WebBackendCapabilities.h"

class WebInputEvent;
class WebNavigationRequest;
class WebSurfaceSnapshot;

class WebPlatformBackend {
    public:
        virtual ~WebPlatformBackend() {}

        virtual const char* get_name() const = 0;
        virtual WebBackendFamily get_family() const = 0;

        virtual bool initialize() = 0;
        virtual void update() = 0;
        virtual void shutdown() = 0;
        virtual bool get_is_initialized() const = 0;

        virtual void get_capabilities(
            WebBackendCapabilities& capabilities
        ) const = 0;

        virtual bool navigate(
            const WebNavigationRequest& request
        ) = 0;

        virtual unsigned long get_surface_revision() const = 0;

        virtual bool get_surface_snapshot(
            WebSurfaceSnapshot& snapshot
        ) const = 0;

        virtual bool handle_input(
            const WebInputEvent& event
        ) = 0;
};
