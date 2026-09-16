// =================================================================================
// Filename:    framework/WebView.h
// Author:      Ebdsaleh
// Description: Declares the backend-neutral semantic WebView component.
// =================================================================================
#pragma once

#include "Panel.h"
#include "Label.h"

class UIEvent;
class WebPlatformHost;

class WebView : public Panel {
    public:
        WebView();

        void set_web_platform_host(WebPlatformHost* host);
        WebPlatformHost* get_web_platform_host();
        const WebPlatformHost* get_web_platform_host() const;

        bool navigate(const char* url);
        void update();
        void arrange(int x, int y, int width, int height);

        virtual bool handle_event(const UIEvent& event);

    private:
        void refresh_labels();

        WebPlatformHost* web_platform_host;
        Label title_label;
        Label backend_label;
        Label address_label;
        Label capability_label;
        Label status_label;
        Label content_label;
};
