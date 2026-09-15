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
#include "MessageInputStrip.h"
#include "ConversationView.h"

class ApplicationRuntime;

class StatusView : public View {
    public:
        StatusView(ApplicationRuntime* application_runtime);

        virtual void layout(int width, int height);
        virtual bool handle_event(const UIEvent& event);
        virtual void render(ComponentRenderer& renderer);

    private:
        static void on_message_submitted(
            MessageInputStrip* input_strip,
            const char* text,
            void* context
        );

        void update_dynamic_text();
        void show_submitted_message(const char* text);

        ApplicationRuntime* application_runtime;
        int client_width;
        int client_height;

        Panel root_panel;

        Panel header_panel;
        Label header_title_label;
        Label header_subtitle_label;

        Panel conversation_panel;
        Label conversation_title_label;
        Label conversation_hint_label;
        ConversationView conversation_view;

        Panel sidebar_panel;
        Label sidebar_title_label;
        StackPanel diagnostics_stack;
        Label runtime_label;
        Label host_label;
        Label web_backend_label;
        Label runtime_status_label;
        Label client_size_label;

        MessageInputStrip message_input_strip;
};
