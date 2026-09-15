// =================================================================================
// Filename:    app/StatusView.cpp
// Author:      Ebdsaleh
// Description: Implements the backend-neutral SalixWeb32 messenger-style shell view.
// =================================================================================

#include <stdio.h>
#include <string>

#include "StatusView.h"
#include "runtime/ApplicationRuntime.h"
#include "framework/UIEvent.h"
#include "framework/rendering/ComponentRenderer.h"

namespace {
    const char* get_file_name_from_path(const char* path) {
        if (path == 0) {
            return "";
        }

        const char* file_name = path;

        for (const char* cursor = path; *cursor != '\0'; ++cursor) {
            if (*cursor == '\\' || *cursor == '/') {
                file_name = cursor + 1;
            }
        }

        return file_name;
    }
}

StatusView::StatusView(
    ApplicationRuntime* new_application_runtime,
    FileDialog* file_dialog
) : application_runtime(new_application_runtime),
    client_width(0),
    client_height(0),
    message_composer(file_dialog) {

    header_title_label.set_text("SalixWeb32 Messenger");
    header_subtitle_label.set_text("Legacy web runtime - local framework shell");

    conversation_title_label.set_text("Conversation");
    conversation_hint_label.set_text(
        "Web backend not loaded - local UI messages are shown below."
    );

    sidebar_title_label.set_text("Connection");
    runtime_label.set_text("Runtime: operational");
    host_label.set_text("Win32 host: operational");
    web_backend_label.set_text("Web backend: not loaded");

    header_title_label.set_horizontal_alignment(Label::align_left);
    header_subtitle_label.set_horizontal_alignment(Label::align_left);
    conversation_title_label.set_horizontal_alignment(Label::align_left);
    conversation_hint_label.set_horizontal_alignment(Label::align_left);
    sidebar_title_label.set_horizontal_alignment(Label::align_left);
    runtime_label.set_horizontal_alignment(Label::align_left);
    host_label.set_horizontal_alignment(Label::align_left);
    web_backend_label.set_horizontal_alignment(Label::align_left);
    runtime_status_label.set_horizontal_alignment(Label::align_left);
    client_size_label.set_horizontal_alignment(Label::align_left);

    root_panel.get_style().background_color = Color(232, 241, 249);
    root_panel.get_style().border_width = 0;

    header_panel.get_style().background_color = Color(214, 235, 249);
    header_panel.get_style().border_color = Color(121, 171, 211);
    header_panel.get_style().border_width = 1;

    header_title_label.get_style().foreground_color = Color(25, 67, 105);
    header_subtitle_label.get_style().foreground_color = Color(73, 105, 133);

    conversation_panel.get_style().background_color = Color(255, 255, 255);
    conversation_panel.get_style().border_color = Color(168, 194, 216);
    conversation_panel.get_style().border_width = 1;

    conversation_title_label.get_style().foreground_color = Color(35, 76, 112);
    conversation_hint_label.get_style().foreground_color = Color(104, 118, 130);

    sidebar_panel.get_style().background_color = Color(238, 246, 252);
    sidebar_panel.get_style().border_color = Color(168, 194, 216);
    sidebar_panel.get_style().border_width = 1;

    sidebar_title_label.get_style().foreground_color = Color(35, 76, 112);
    runtime_label.get_style().foreground_color = Color(48, 76, 101);
    host_label.get_style().foreground_color = Color(48, 76, 101);
    web_backend_label.get_style().foreground_color = Color(48, 76, 101);
    runtime_status_label.get_style().foreground_color = Color(48, 76, 101);
    client_size_label.get_style().foreground_color = Color(48, 76, 101);

    diagnostics_stack.set_orientation(StackPanel::orientation_vertical);
    diagnostics_stack.set_main_axis_alignment(StackPanel::main_axis_start);
    diagnostics_stack.set_item_extent(24);
    diagnostics_stack.set_spacing(2);

    diagnostics_stack.add_child(&runtime_label);
    diagnostics_stack.add_child(&host_label);
    diagnostics_stack.add_child(&web_backend_label);
    diagnostics_stack.add_child(&runtime_status_label);
    diagnostics_stack.add_child(&client_size_label);

    header_panel.add_child(&header_title_label);
    header_panel.add_child(&header_subtitle_label);

    conversation_panel.add_child(&conversation_title_label);
    conversation_panel.add_child(&conversation_hint_label);
    conversation_panel.add_child(&conversation_view);

    conversation_view.append_system_message(
        "Framework components online."
    );

    sidebar_panel.add_child(&sidebar_title_label);
    sidebar_panel.add_child(&diagnostics_stack);

    message_composer.set_text("Hello from Pentium 4");
    message_composer.set_button_text("Send");
    message_composer.set_submit_on_enter(true);
    message_composer.set_submit_handler(
        StatusView::on_message_submitted,
        this
    );

    root_panel.add_child(&header_panel);
    root_panel.add_child(&conversation_panel);
    root_panel.add_child(&sidebar_panel);
    root_panel.add_child(&message_composer);
}

void StatusView::attach_native_control_host(
    NativeControlHost* control_host
) {
    message_composer.attach_native_controls(control_host);
}

void StatusView::detach_native_control_host() {
    message_composer.detach_native_controls();
}

void StatusView::layout(int width, int height) {
    const int outer_padding = 8;
    const int gap = 6;
    const int header_height = 58;
    const int composer_height = 132;
    const int sidebar_width = 190;

    client_width = width;
    client_height = height;

    root_panel.set_bounds(0, 0, client_width, client_height);

    int content_width = client_width - (outer_padding * 2);
    if (content_width < 0) {
        content_width = 0;
    }

    header_panel.set_bounds(
        outer_padding,
        outer_padding,
        content_width,
        header_height
    );

    header_title_label.set_bounds(
        outer_padding + 14,
        outer_padding + 7,
        content_width - 28,
        22
    );

    header_subtitle_label.set_bounds(
        outer_padding + 14,
        outer_padding + 29,
        content_width - 28,
        20
    );

    int body_y = outer_padding + header_height + gap;
    int composer_y = client_height - outer_padding - composer_height;

    if (composer_y < body_y) {
        composer_y = body_y;
    }

    int body_height = composer_y - gap - body_y;
    if (body_height < 0) {
        body_height = 0;
    }

    message_composer.arrange(
        outer_padding,
        composer_y,
        content_width,
        composer_height
    );

    bool show_sidebar = content_width >= 620 && body_height >= 120;
    sidebar_panel.set_visible(show_sidebar);

    int conversation_width = content_width;

    if (show_sidebar) {
        conversation_width -= sidebar_width + gap;
    }

    if (conversation_width < 0) {
        conversation_width = 0;
    }

    conversation_panel.set_bounds(
        outer_padding,
        body_y,
        conversation_width,
        body_height
    );

    conversation_title_label.set_bounds(
        outer_padding + 14,
        body_y + 10,
        conversation_width - 28,
        22
    );

    conversation_hint_label.set_bounds(
        outer_padding + 14,
        body_y + 36,
        conversation_width - 28,
        20
    );

    int conversation_view_height = body_height - 74;
    if (conversation_view_height < 0) {
        conversation_view_height = 0;
    }

    conversation_view.arrange(
        outer_padding + 10,
        body_y + 64,
        conversation_width - 20,
        conversation_view_height
    );

    if (show_sidebar) {
        int sidebar_x = outer_padding + conversation_width + gap;

        sidebar_panel.set_bounds(
            sidebar_x,
            body_y,
            sidebar_width,
            body_height
        );

        sidebar_title_label.set_bounds(
            sidebar_x + 10,
            body_y + 10,
            sidebar_width - 20,
            22
        );

        diagnostics_stack.arrange(
            sidebar_x + 10,
            body_y + 40,
            sidebar_width - 20,
            body_height - 50
        );
    }
}

bool StatusView::handle_event(const UIEvent& event) {
    bool is_mouse_event =
        event.type == UIEvent::event_mouse_move ||
        event.type == UIEvent::event_mouse_down ||
        event.type == UIEvent::event_mouse_up;

    if (
        is_mouse_event &&
        message_composer.contains_popup_point(event.x, event.y)
    ) {
        return message_composer.handle_event(event);
    }

    return root_panel.handle_event(event);
}

void StatusView::render(ComponentRenderer& renderer) {
    update_dynamic_text();
    root_panel.render(renderer);
}

void StatusView::on_message_submitted(
    MessageComposer* composer,
    const MessageDraft& draft,
    void* context
) {
    (void)composer;

    StatusView* status_view = (StatusView*)context;
    if (status_view != 0) {
        status_view->show_submitted_message(draft);
    }
}

void StatusView::update_dynamic_text() {
    char service_text[128];
    char size_text[128];

    if (application_runtime != 0) {
        sprintf(
            service_text,
            "Services: %d | ticks: %lu",
            application_runtime->get_service_count(),
            application_runtime->get_update_count()
        );
    } else {
        sprintf(service_text, "Runtime services unavailable");
    }

    sprintf(
        size_text,
        "Client: %d x %d",
        client_width,
        client_height
    );

    runtime_status_label.set_text(service_text);
    client_size_label.set_text(size_text);
}

void StatusView::show_submitted_message(const MessageDraft& draft) {
    const FormattedText& body = draft.get_body();

    if (!body.empty()) {
        conversation_view.append_local_message(body);
    }

    int attachment_count = draft.get_attachment_count();

    for (int index = 0; index < attachment_count; ++index) {
        const char* path = draft.get_attachment_path(index);
        std::string attachment_message("Attached: ");
        attachment_message += get_file_name_from_path(path);

        conversation_view.append_system_message(
            attachment_message.c_str()
        );
    }
}
