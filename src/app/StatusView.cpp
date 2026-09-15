// =================================================================================
// Filename:    app/StatusView.cpp
// Author:      Ebdsaleh
// Description: Implements the backend-neutral SalixWeb32 status view.
// =================================================================================

#include <stdio.h>

#include "StatusView.h"
#include "runtime/ApplicationRuntime.h"
#include "framework/rendering/ComponentRenderer.h"

StatusView::StatusView(ApplicationRuntime* new_application_runtime)
    : application_runtime(new_application_runtime),
      client_width(0),
      client_height(0) {

    runtime_label.set_text("SalixWeb32 runtime operational");
    host_label.set_text("Win32 application host operational");
    web_backend_label.set_text("Web platform backend: not loaded");

    runtime_label.set_horizontal_alignment(Label::align_center);
    host_label.set_horizontal_alignment(Label::align_center);
    web_backend_label.set_horizontal_alignment(Label::align_center);
    runtime_status_label.set_horizontal_alignment(Label::align_center);
    client_size_label.set_horizontal_alignment(Label::align_center);

    status_stack.set_orientation(StackPanel::orientation_vertical);
    status_stack.set_item_extent(30);
    status_stack.set_spacing(0);

    status_stack.add_child(&runtime_label);
    status_stack.add_child(&host_label);
    status_stack.add_child(&web_backend_label);
    status_stack.add_child(&runtime_status_label);
    status_stack.add_child(&client_size_label);
}

void StatusView::layout(int width, int height) {
    client_width = width;
    client_height = height;

    status_stack.arrange(0, 0, client_width, client_height);
}

void StatusView::render(ComponentRenderer& renderer) {
    update_dynamic_text();
    status_stack.render(renderer);
}

void StatusView::update_dynamic_text() {
    char service_text[128];
    char size_text[128];

    if (application_runtime != 0) {
        sprintf(
            service_text,
            "Runtime services: %d | update ticks: %lu",
            application_runtime->get_service_count(),
            application_runtime->get_update_count()
        );
    } else {
        sprintf(service_text, "Runtime services unavailable");
    }

    sprintf(
        size_text,
        "Client area: %d x %d",
        client_width,
        client_height
    );

    runtime_status_label.set_text(service_text);
    client_size_label.set_text(size_text);
}
