// =================================================================================
// Filename:    app/BrowserProbeView.h
// Author:      Ebdsaleh
// Description: Declares the application-level Browser Probe workspace.
// =================================================================================
#pragma once

#include <string>

#include "framework/Panel.h"
#include "framework/Label.h"
#include "framework/TextInput.h"
#include "framework/Button.h"
#include "framework/ScrollBar.h"
#include "web/platform/WebSurfaceSnapshot.h"

class Clipboard;
class ComponentRenderer;
class NativeControlHost;
class TextMetrics;
class UIEvent;
class WebPlatformHost;

class BrowserProbeView : public Panel {
    public:
        enum ProbeMode {
            probe_summary = 0,
            probe_headers,
            probe_raw,
            probe_extracted
        };

        BrowserProbeView();
        virtual ~BrowserProbeView();

        void set_web_platform_host(WebPlatformHost* host);
        WebPlatformHost* get_web_platform_host();
        const WebPlatformHost* get_web_platform_host() const;

        bool navigate(const char* url);
        void update();

        const char* get_title_text() const {
            return title_label.get_text();
        }

        const char* get_backend_text() const {
            return backend_label.get_text();
        }

        const char* get_capability_text() const {
            return capability_label.get_text();
        }

        const char* get_status_text() const {
            return status_label.get_text();
        }

        const char* get_address_text() const {
            return address_input.get_text();
        }

        ProbeMode get_probe_mode() const {
            return probe_mode;
        }

        const char* get_probe_mode_name() const {
            switch (probe_mode) {
                case probe_headers:
                    return "Headers";
                case probe_raw:
                    return "Raw";
                case probe_extracted:
                    return "Extracted";
                case probe_summary:
                default:
                    return "Summary";
            }
        }

        const char* get_current_output_text() const;

        const std::string& get_summary_output_text() const {
            return summary_output_text;
        }

        const std::string& get_headers_output_text() const {
            return headers_output_text;
        }

        const std::string& get_raw_output_text() const {
            return raw_output_text;
        }

        const std::string& get_extracted_output_text() const {
            return extracted_output_text;
        }

        void attach_native_controls(NativeControlHost* control_host);
        void detach_native_controls();

        void arrange(
            int x,
            int y,
            int width,
            int height,
            TextMetrics* text_metrics = 0
        );

        virtual bool handle_event(const UIEvent& event);
        virtual void render(ComponentRenderer& renderer) const;

    private:
        static void on_go_clicked(Button* button, void* context);
        static void on_probe_mode_clicked(Button* button, void* context);
        static void on_copy_clicked(Button* button, void* context);
        static void on_scroll_changed(
            ScrollBar* scroll_bar,
            int value,
            void* context
        );

        void set_probe_mode(ProbeMode new_mode);
        void refresh_labels();
        void cache_probe_outputs(const WebSurfaceSnapshot& snapshot);
        void refresh_probe_content();
        const std::string& get_active_output_text() const;
        void set_output_text(const std::string& text);
        void copy_current_output();
        void scroll_pixels(int pixel_count);
        void update_content_metrics(TextMetrics* text_metrics);
        void layout_output();
        void sync_native_scrollbar();
        bool is_output_point(int x, int y) const;

        WebPlatformHost* web_platform_host;
        ProbeMode probe_mode;
        unsigned long observed_surface_revision;

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
        Button copy_button;

        Label content_label;
        ScrollBar vertical_scroll_bar;

        NativeControlHost* native_control_host;
        Clipboard* active_clipboard;

        std::string summary_output_text;
        std::string headers_output_text;
        std::string raw_output_text;
        std::string extracted_output_text;
        std::string displayed_output_text;

        int output_x;
        int output_y;
        int output_width;
        int output_height;
        int content_height;
        int scroll_offset_y;
        int scroll_bar_width;
        int line_step_pixels;
        bool layout_dirty;
};
