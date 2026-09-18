// =================================================================================
// Filename:    web/backends/PlaceholderWebBackend.h
// Author:      Ebdsaleh
// Description: Declares a no-network backend used to validate the WebView contract.
// =================================================================================
#pragma once

#include <string>

#include "web/platform/WebPlatformBackend.h"
#include "web/platform/WebSurfaceSnapshot.h"

class PlaceholderWebBackend : public WebPlatformBackend {
    public:
        PlaceholderWebBackend();

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
        void refresh_surface_text();

        bool is_initialized;
        unsigned long update_count;
        unsigned long input_event_count;
        unsigned long surface_revision;
        std::string current_url;
        WebSurfaceSnapshot surface_snapshot;
};
