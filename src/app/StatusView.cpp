// =================================================================================
// Filename:    app/StatusView.cpp
// Author:      Ebdsaleh
// Description: Implements the backend-neutral SalixWeb32 messenger-style shell view.
// =================================================================================

#include <stdio.h>
#include <string>
#include <vector>

#include "StatusView.h"
#include "runtime/ApplicationRuntime.h"
#include "framework/ApplicationCommand.h"
#include "framework/DesktopServices.h"
#include "framework/FileDialog.h"
#include "framework/NativeControlHost.h"
#include "framework/UIEvent.h"
#include "framework/rendering/ComponentRenderer.h"

StatusView::StatusView(
    ApplicationRuntime* new_application_runtime,
    FileDialog* new_file_dialog,
    DesktopServices* new_desktop_services
) : application_runtime(new_application_runtime),
    file_dialog(new_file_dialog),
    desktop_services(new_desktop_services),
    native_control_host(0),
    client_width(0),
    client_height(0),
    conversation_tab_index(-1),
    runtime_tab_index(-1),
    message_composer(new_file_dialog) {

    header_title_label.set_text("SalixWeb32 Messenger");
    header_subtitle_label.set_text("Legacy web runtime - local framework shell");

    conversation_title_label.set_text("Conversation");
    conversation_hint_label.set_text(
        "Web backend not loaded - local UI messages are shown below."
    );

    sidebar_title_label.set_text("Runtime diagnostics");
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

    conversation_page.get_style().background_color = Color(255, 255, 255);
    conversation_page.get_style().border_width = 0;

    runtime_page.get_style().background_color = Color(238, 246, 252);
    runtime_page.get_style().border_width = 0;

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

    conversation_view.set_desktop_services(desktop_services);
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

    conversation_page.add_child(&conversation_panel);
    conversation_page.add_child(&message_composer);
    runtime_page.add_child(&sidebar_panel);

    conversation_tab_index = workspace_tabs.add_tab(
        "Conversation",
        &conversation_page
    );
    runtime_tab_index = workspace_tabs.add_tab(
        "Runtime",
        &runtime_page
    );
    workspace_tabs.set_tab_changed_handler(
        StatusView::on_workspace_tab_changed,
        this
    );

    root_panel.add_child(&header_panel);
    root_panel.add_child(&workspace_tabs);
}

void StatusView::attach_native_control_host(
    NativeControlHost* control_host
) {
    native_control_host = control_host;

    if (native_control_host != 0) {
        native_control_host->attach_tab_view(&workspace_tabs);
        native_control_host->sync_tab_view(&workspace_tabs);
    }

    update_active_native_controls();
}

void StatusView::detach_native_control_host() {
    conversation_view.detach_native_controls();
    message_composer.detach_native_controls();

    if (native_control_host != 0) {
        native_control_host->detach_tab_view(&workspace_tabs);
    }

    native_control_host = 0;
}

void StatusView::layout(
    int width,
    int height,
    TextMetrics* text_metrics
) {
    const int outer_padding = 8;
    const int gap = 6;
    const int header_height = 58;
    const int composer_height = 132;
    const int page_padding = 8;

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

    int workspace_y = outer_padding + header_height + gap;
    int workspace_height = client_height - workspace_y - outer_padding;
    if (workspace_height < 0) {
        workspace_height = 0;
    }

    workspace_tabs.arrange(
        outer_padding,
        workspace_y,
        content_width,
        workspace_height
    );

    if (native_control_host != 0) {
        native_control_host->sync_tab_view(&workspace_tabs);
    }

    int page_x = workspace_tabs.get_content_x() + page_padding;
    int page_y = workspace_tabs.get_content_y() + page_padding;
    int page_width = workspace_tabs.get_content_width() - (page_padding * 2);
    int page_height = workspace_tabs.get_content_height() - (page_padding * 2);

    if (page_width < 0) {
        page_width = 0;
    }

    if (page_height < 0) {
        page_height = 0;
    }

    int composer_y = page_y + page_height - composer_height;
    if (composer_y < page_y) {
        composer_y = page_y;
    }

    int conversation_height = composer_y - gap - page_y;
    if (conversation_height < 0) {
        conversation_height = 0;
    }

    conversation_panel.set_bounds(
        page_x,
        page_y,
        page_width,
        conversation_height
    );

    conversation_title_label.set_bounds(
        page_x + 14,
        page_y + 10,
        page_width - 28,
        22
    );

    conversation_hint_label.set_bounds(
        page_x + 14,
        page_y + 36,
        page_width - 28,
        20
    );

    int conversation_view_height = conversation_height - 74;
    if (conversation_view_height < 0) {
        conversation_view_height = 0;
    }

    conversation_view.arrange(
        page_x + 10,
        page_y + 64,
        page_width - 20,
        conversation_view_height,
        text_metrics
    );

    message_composer.arrange(
        page_x,
        composer_y,
        page_width,
        composer_height
    );

    sidebar_panel.set_bounds(
        page_x,
        page_y,
        page_width,
        page_height
    );

    sidebar_title_label.set_bounds(
        page_x + 12,
        page_y + 12,
        page_width - 24,
        22
    );

    diagnostics_stack.arrange(
        page_x + 12,
        page_y + 44,
        page_width - 24,
        page_height - 56
    );
}

bool StatusView::handle_event(const UIEvent& event) {
    if (event.type == UIEvent::event_command) {
        return handle_application_command(event.command_id);
    }

    bool is_mouse_event =
        event.type == UIEvent::event_mouse_move ||
        event.type == UIEvent::event_mouse_down ||
        event.type == UIEvent::event_mouse_up ||
        event.type == UIEvent::event_mouse_wheel;

    if (
        workspace_tabs.get_active_index() == conversation_tab_index &&
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

void StatusView::on_workspace_tab_changed(
    TabView* tab_view,
    int old_index,
    int new_index,
    void* context
) {
    (void)tab_view;
    (void)old_index;
    (void)new_index;

    StatusView* status_view = (StatusView*)context;
    if (status_view != 0) {
        status_view->update_active_native_controls();
    }
}

bool StatusView::handle_application_command(int command_id) {
    switch (command_id) {
        case application_command_attach_file:
            if (workspace_tabs.get_active_index() != conversation_tab_index) {
                workspace_tabs.set_active_index(conversation_tab_index);
                update_active_native_controls();
            }
            return attach_files_from_dialog();

        case application_command_show_conversation:
            workspace_tabs.set_active_index(conversation_tab_index);
            update_active_native_controls();
            return true;

        case application_command_show_runtime:
            workspace_tabs.set_active_index(runtime_tab_index);
            update_active_native_controls();
            return true;
    }

    return false;
}

bool StatusView::attach_files_from_dialog() {
    if (file_dialog == 0) {
        return false;
    }

    std::vector<std::string> selected_paths;
    if (!file_dialog->open_files(selected_paths) || selected_paths.empty()) {
        return false;
    }

    message_composer.add_attachment_paths(selected_paths);
    return true;
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
        conversation_view.append_attachment(
            draft.get_attachment(index)
        );
    }
}

void StatusView::update_active_native_controls() {
    if (native_control_host == 0) {
        return;
    }

    native_control_host->sync_tab_view(&workspace_tabs);

    bool conversation_active =
        workspace_tabs.get_active_index() == conversation_tab_index;

    if (conversation_active) {
        conversation_view.attach_native_controls(native_control_host);
        message_composer.attach_native_controls(native_control_host);
    } else {
        conversation_view.detach_native_controls();
        message_composer.detach_native_controls();
    }
}
