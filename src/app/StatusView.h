// =================================================================================
// Filename:    app/StatusView.h
// Author:      Ebdsaleh
// Description: Declares the backend-neutral SalixWeb32 messenger-style shell view.
// =================================================================================
#pragma once

#include <stdio.h>
#include <string>

#include "framework/View.h"
#include "framework/Panel.h"
#include "framework/Label.h"
#include "framework/StackPanel.h"
#include "framework/TabView.h"
#include "MessageComposer.h"
#include "ConversationView.h"
#include "BrowserProbeView.h"

class ApplicationRuntime;
class DesktopServices;
class FileDialog;
class NativeControlHost;
class TextMetrics;
class WebPlatformHost;

class StatusView : public View {
    public:
        StatusView(
            ApplicationRuntime* application_runtime,
            FileDialog* file_dialog,
            DesktopServices* desktop_services,
            WebPlatformHost* web_platform_host
        );

        virtual void attach_native_control_host(NativeControlHost* control_host);
        virtual void detach_native_control_host();

        virtual bool build_diagnostic_report(std::string& report) const {
            report.clear();
            report += "SalixWeb32 Diagnostic Report\r\n";
            report += "===========================\r\n\r\n";

            int active_index = workspace_tabs.get_active_index();
            const char* active_title = workspace_tabs.get_tab_title(active_index);

            report += "Active view: ";
            report += active_title == 0 ? "Unknown" : active_title;
            report += "\r\n";

            char size_text[96];
            sprintf(
                size_text,
                "Client size: %d x %d\r\n",
                client_width,
                client_height
            );
            report += size_text;

            report += runtime_label.get_text();
            report += "\r\n";
            report += host_label.get_text();
            report += "\r\n";
            report += web_backend_label.get_text();
            report += "\r\n";
            report += web_capability_label.get_text();
            report += "\r\n";
            report += runtime_status_label.get_text();
            report += "\r\n\r\n";

            if (active_index == web_tab_index) {
                report += "Browser Probe\r\n";
                report += "-------------\r\n";
                report += "Title: ";
                report += browser_probe_view.get_title_text();
                report += "\r\n";
                report += "URL: ";
                report += browser_probe_view.get_address_text();
                report += "\r\n";
                report += browser_probe_view.get_backend_text();
                report += "\r\n";
                report += browser_probe_view.get_capability_text();
                report += "\r\n";
                report += browser_probe_view.get_status_text();
                report += "\r\n";
                report += "Selected result: ";
                report += browser_probe_view.get_probe_mode_name();
                report += "\r\n\r\n";
                report += browser_probe_view.get_current_output_text();
                report += "\r\n";
            } else if (active_index == runtime_tab_index) {
                report += "Runtime Diagnostics\r\n";
                report += "-------------------\r\n";
                report += runtime_label.get_text();
                report += "\r\n";
                report += host_label.get_text();
                report += "\r\n";
                report += web_backend_label.get_text();
                report += "\r\n";
                report += web_capability_label.get_text();
                report += "\r\n";
                report += runtime_status_label.get_text();
                report += "\r\n";
                report += client_size_label.get_text();
                report += "\r\n";
            } else {
                report += "Conversation\r\n";
                report += "------------\r\n";
                report += "Message count: ";

                char message_count_text[32];
                sprintf(
                    message_count_text,
                    "%d\r\n",
                    conversation_view.get_message_count()
                );
                report += message_count_text;
            }

            return true;
        }

        virtual bool build_browser_diagnostic_report(
            std::string& report
        ) const {
            const std::string& summary =
                browser_probe_view.get_summary_output_text();
            const std::string& headers =
                browser_probe_view.get_headers_output_text();
            const std::string& raw =
                browser_probe_view.get_raw_output_text();
            const std::string& extracted =
                browser_probe_view.get_extracted_output_text();

            report.clear();
            report.reserve(
                summary.size() +
                headers.size() +
                raw.size() +
                extracted.size() +
                2048
            );

            report += "SalixWeb32 Browser Diagnostic Report\r\n";
            report += "====================================\r\n\r\n";
            report += "Title: ";
            report += browser_probe_view.get_title_text();
            report += "\r\n";
            report += "URL: ";
            report += browser_probe_view.get_address_text();
            report += "\r\n";
            report += browser_probe_view.get_backend_text();
            report += "\r\n";
            report += browser_probe_view.get_capability_text();
            report += "\r\n";
            report += browser_probe_view.get_status_text();
            report += "\r\n\r\n";

            report += "Summary:\r\n";
            report += summary;
            report += "\r\n\r\nHeaders:\r\n";
            report += headers;
            report += "\r\n\r\nRaw:\r\n";
            report += raw;
            report += "\r\n\r\nExtracted:\r\n";
            report += extracted;
            report += "\r\n";

            return true;
        }

        virtual void layout(int width, int height, TextMetrics* text_metrics = 0);
        virtual bool handle_event(const UIEvent& event);
        virtual void render(ComponentRenderer& renderer);

    private:
        static void on_message_submitted(
            MessageComposer* composer,
            const MessageDraft& draft,
            void* context
        );
        static void on_workspace_tab_changed(
            TabView* tab_view,
            int old_index,
            int new_index,
            void* context
        );

        bool handle_application_command(int command_id);
        bool attach_files_from_dialog();
        void update_dynamic_text();
        void show_submitted_message(const MessageDraft& draft);
        void update_active_native_controls();

        ApplicationRuntime* application_runtime;
        FileDialog* file_dialog;
        DesktopServices* desktop_services;
        WebPlatformHost* web_platform_host;
        NativeControlHost* native_control_host;
        int client_width;
        int client_height;
        int conversation_tab_index;
        int web_tab_index;
        int runtime_tab_index;

        Panel root_panel;

        Panel header_panel;
        Label header_title_label;
        Label header_subtitle_label;

        TabView workspace_tabs;
        Panel conversation_page;
        Panel web_page;
        Panel runtime_page;

        Panel conversation_panel;
        Label conversation_title_label;
        Label conversation_hint_label;
        ConversationView conversation_view;

        BrowserProbeView browser_probe_view;

        Panel sidebar_panel;
        Label sidebar_title_label;
        StackPanel diagnostics_stack;
        Label runtime_label;
        Label host_label;
        Label web_backend_label;
        Label web_capability_label;
        Label runtime_status_label;
        Label client_size_label;

        MessageComposer message_composer;
};
