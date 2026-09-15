// =================================================================================
// Filename:    app/StatusView.cpp
// Author:      Ebdsaleh
// Description: Implements the first backend-neutral SalixWeb32 application view.
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
}

void StatusView::layout(int width, int height) {
    client_width = width;
    client_height = height;

    layout_label(runtime_label, -60);
    layout_label(host_label, -30);
    layout_label(web_backend_label, 0);
    layout_label(runtime_status_label, 30);
    layout_label(client_size_label, 60);
}

void StatusView::render(ComponentRenderer& renderer) {
    update_dynamic_text();

    runtime_label.render(renderer);
    host_label.render(renderer);
    web_backend_label.render(renderer);
    runtime_status_label.render(renderer);
    client_size_label.render(renderer);
}

void StatusView::layout_label(Label& label, int y_offset) {
    int center_y = client_height / 2;

    label.set_bounds(
        0,
        center_y + y_offset - 15,
        client_width,
        30
    );
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
