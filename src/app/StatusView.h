// =================================================================================
// Filename:    app/StatusView.h
// Author:      Ebdsaleh
// Description: Declares the backend-neutral SalixWeb32 status view.
// =================================================================================
#pragma once

#include "framework/View.h"
#include "framework/Label.h"
#include "framework/StackPanel.h"

class ApplicationRuntime;

class StatusView : public View {
    public:
        StatusView(ApplicationRuntime* application_runtime);

        virtual void layout(int width, int height);
        virtual void render(ComponentRenderer& renderer);

    private:
        void update_dynamic_text();

        ApplicationRuntime* application_runtime;
        int client_width;
        int client_height;

        StackPanel status_stack;
        Label runtime_label;
        Label host_label;
        Label web_backend_label;
        Label runtime_status_label;
        Label client_size_label;
};
