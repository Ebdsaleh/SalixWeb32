// =================================================================================
// Filename:    app/StatusView.h
// Author:      Ebdsaleh
// Description: Declares the backend-neutral SalixWeb32 status view.
// =================================================================================
#pragma once

#include "framework/View.h"
#include "framework/Label.h"
#include "framework/StackPanel.h"
#include "framework/Button.h"
#include "framework/TextInput.h"

class ApplicationRuntime;

class StatusView : public View {
    public:
        StatusView(ApplicationRuntime* application_runtime);

        virtual void layout(int width, int height);
        virtual bool handle_event(const UIEvent& event);
        virtual void render(ComponentRenderer& renderer);

    private:
        static void on_apply_button_clicked(Button* button, void* context);

        void update_dynamic_text();
        void apply_input_message();

        ApplicationRuntime* application_runtime;
        int client_width;
        int client_height;

        StackPanel status_stack;
        Label runtime_label;
        Label host_label;
        Label web_backend_label;
        Label runtime_status_label;
        Label client_size_label;
        Label input_instruction_label;
        TextInput message_input;
        Button apply_button;
        Label result_label;
};
