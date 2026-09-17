// =================================================================================
// Filename:    app/StatusView.h
// Author:      Ebdsaleh
// Description: Declares the backend-neutral SalixWeb32 messenger-style shell view.
// =================================================================================
#pragma once

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
