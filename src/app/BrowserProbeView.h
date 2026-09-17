// =================================================================================
// Filename:    app/BrowserProbeView.h
// Author:      Ebdsaleh
// Description: Declares the application-level Browser Probe workspace.
// =================================================================================
#pragma once

#include "framework/Panel.h"
#include "framework/Label.h"
#include "framework/TextInput.h"
#include "framework/Button.h"

class WebPlatformHost;
class WebSurfaceSnapshot;

class BrowserProbeView : public Panel {
    public:
        BrowserProbeView();

        void set_web_platform_host(WebPlatformHost* host);
        WebPlatformHost* get_web_platform_host();
        const WebPlatformHost* get_web_platform_host() const;

        bool navigate(const char* url);
        void update();
        void arrange(int x, int y, int width, int height);

    private:
        enum ProbeMode {
            probe_summary = 0,
            probe_headers,
            probe_raw,
            probe_extracted
        };

        static void on_go_clicked(Button* button, void* context);
        static void on_probe_mode_clicked(Button* button, void* context);

        void set_probe_mode(ProbeMode new_mode);
        void refresh_labels();
        void refresh_probe_content(const WebSurfaceSnapshot& snapshot);

        WebPlatformHost* web_platform_host;
        ProbeMode probe_mode;

        Label title_label;
        Label backend_label;
        Label capability_label;
        Label status_label;

        TextInput address_input;
        Button go_button;

        Button summary_button;
        Button headers_button;
        Button raw_button;
        Button extracted_button;

        Label content_label;
};
