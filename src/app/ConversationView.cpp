// =================================================================================
// Filename:    app/ConversationView.cpp
// Author:      Ebdsaleh
// Description: Implements the append-only scrollable conversation message surface.
// =================================================================================

#include "ConversationView.h"
#include "framework/MarkdownFormatter.h"
#include "framework/NativeControlHost.h"
#include "framework/UIEvent.h"

ConversationView::ConversationView()
    : vertical_scroll_bar(ScrollBar::vertical),
      native_control_host(0),
      first_visible_index(0),
      row_spacing(4),
      content_padding(4),
      scroll_bar_width(16) {

    get_style().background_color = Color(255, 255, 255);
    get_style().border_width = 0;

    vertical_scroll_bar.set_line_step(1);
    vertical_scroll_bar.set_value_changed_handler(
        ConversationView::on_scroll_changed,
        this
    );
    vertical_scroll_bar.set_visible(false);

    add_child(&vertical_scroll_bar);
}

ConversationView::~ConversationView() {
    detach_native_controls();
    clear_messages();
}

bool ConversationView::append_message(
    MessageRole role,
    const char* text
) {
    if (text == 0 || text[0] == '\0') {
        return false;
    }

    FormattedText formatted_text;
    formatted_text.set_plain_text(text, TextFormat());
    return append_message(role, formatted_text);
}

bool ConversationView::append_message(
    MessageRole role,
    const FormattedText& text
) {
    if (text.empty()) {
        return false;
    }

    Label* message_label = new Label();
    if (message_label == 0) {
        return false;
    }

    FormattedText markdown_text;
    if (!MarkdownFormatter::format(text, markdown_text)) {
        markdown_text = text;
    }

    bool block_layout = MarkdownFormatter::has_block_structure(
        text.get_text()
    );

    FormattedText display_text;
    TextFormat role_format(true, false, false, 12);

    if (block_layout) {
        display_text.append_plain_text(
            get_role_label(role),
            role_format
        );
        display_text.append_plain_text("\n\n", role_format);
    } else {
        display_text.append_plain_text(
            get_role_prefix(role),
            role_format
        );
    }

    display_text.append_formatted_text(markdown_text);

    message_label->set_formatted_text(display_text);
    message_label->set_horizontal_alignment(Label::align_left);
    message_label->set_selectable(true);
    message_label->get_style().foreground_color = get_role_color(role);

    if (!add_child(message_label)) {
        delete message_label;
        return false;
    }

    MessageEntry entry;
    entry.role = role;
    entry.source_text = text;
    entry.label = message_label;
    entry.row_height = calculate_entry_height(*message_label);
    messages.push_back(entry);

    scroll_to_bottom();
    return true;
}

bool ConversationView::append_system_message(const char* text) {
    return append_message(message_system, text);
}

bool ConversationView::append_local_message(const char* text) {
    return append_message(message_local, text);
}

bool ConversationView::append_local_message(
    const FormattedText& text
) {
    return append_message(message_local, text);
}

bool ConversationView::append_remote_message(const char* text) {
    return append_message(message_remote, text);
}

bool ConversationView::append_remote_message(
    const FormattedText& text
) {
    return append_message(message_remote, text);
}

void ConversationView::clear_messages() {
    for (int index = 0; index < (int)messages.size(); ++index) {
        Label* label = messages[index].label;
        if (label != 0) {
            remove_child(label);
            delete label;
        }
    }

    messages.clear();
    first_visible_index = 0;
    vertical_scroll_bar.set_range(0, 0, 1);
    vertical_scroll_bar.set_value(0);
    vertical_scroll_bar.set_visible(false);
    sync_native_scrollbar();
    relayout();
}

int ConversationView::get_message_count() const {
    return (int)messages.size();
}

void ConversationView::attach_native_controls(
    NativeControlHost* control_host
) {
    if (native_control_host == control_host) {
        return;
    }

    detach_native_controls();
    native_control_host = control_host;

    if (native_control_host != 0) {
        native_control_host->attach_scroll_bar(&vertical_scroll_bar);
        sync_native_scrollbar();
    }
}

void ConversationView::detach_native_controls() {
    if (native_control_host == 0) {
        return;
    }

    native_control_host->detach_scroll_bar(&vertical_scroll_bar);
    native_control_host = 0;
}

void ConversationView::arrange(int x, int y, int width, int height) {
    set_bounds(x, y, width, height);
    relayout();
}

void ConversationView::scroll_lines(int line_count) {
    if (line_count == 0 || messages.empty()) {
        return;
    }

    first_visible_index += line_count;
    clamp_first_visible_index();
    vertical_scroll_bar.set_value(first_visible_index);
    relayout();
}

void ConversationView::scroll_to_bottom() {
    first_visible_index = calculate_first_index_for_bottom();
    vertical_scroll_bar.set_value(first_visible_index);
    relayout();
}

bool ConversationView::handle_event(const UIEvent& event) {
    if (!get_is_visible()) {
        return false;
    }

    if (
        event.type == UIEvent::event_mouse_wheel &&
        contains_point(event.x, event.y) &&
        event.wheel_delta != 0
    ) {
        int notches = event.wheel_delta / 120;
        if (notches == 0) {
            notches = event.wheel_delta > 0 ? 1 : -1;
        }

        scroll_lines(-notches * 3);
        return true;
    }

    return Panel::handle_event(event);
}

void ConversationView::on_scroll_changed(
    ScrollBar* scroll_bar,
    int value,
    void* context
) {
    (void)scroll_bar;

    ConversationView* conversation_view = (ConversationView*)context;
    if (conversation_view == 0) {
        return;
    }

    conversation_view->first_visible_index = value;
    conversation_view->clamp_first_visible_index();
    conversation_view->relayout();
}

void ConversationView::relayout() {
    clamp_first_visible_index();

    int view_x = get_x();
    int view_y = get_y();
    int view_width = get_width();
    int view_height = get_height();
    int message_count = (int)messages.size();

    int available_height = view_height - (content_padding * 2);
    if (available_height < 0) {
        available_height = 0;
    }

    bool show_scrollbar =
        calculate_total_content_height() > available_height;

    int scroll_gap = show_scrollbar ? 4 : 0;
    int reserved_scroll_width = show_scrollbar
        ? scroll_bar_width + scroll_gap
        : 0;

    int message_x = view_x + content_padding;
    int message_width = view_width -
        (content_padding * 2) -
        reserved_scroll_width;
    if (message_width < 0) {
        message_width = 0;
    }

    int usable_bottom = view_y + view_height - content_padding;
    int current_y = view_y + content_padding;
    int last_visible_index = first_visible_index - 1;
    int visible_count = 0;
    bool reached_bottom = false;

    for (int index = 0; index < message_count; ++index) {
        Label* label = messages[index].label;
        if (label == 0) {
            continue;
        }

        if (index < first_visible_index || reached_bottom) {
            label->set_visible(false);
            continue;
        }

        int row_height = messages[index].row_height;
        if (row_height < 1) {
            row_height = 1;
        }

        if (current_y + row_height > usable_bottom) {
            label->set_visible(false);
            reached_bottom = true;
            continue;
        }

        label->set_visible(true);
        label->set_bounds(
            message_x,
            current_y,
            message_width,
            row_height
        );
        last_visible_index = index;
        ++visible_count;
        current_y += row_height + row_spacing;
    }

    if (last_visible_index < first_visible_index) {
        visible_count = 0;
    }

    vertical_scroll_bar.set_visible(show_scrollbar);

    if (show_scrollbar) {
        vertical_scroll_bar.arrange(
            view_x + view_width - content_padding - scroll_bar_width,
            view_y + content_padding,
            scroll_bar_width,
            available_height
        );
    } else {
        vertical_scroll_bar.arrange(0, 0, 0, 0);
    }

    update_scrollbar_state(visible_count);
}

void ConversationView::clamp_first_visible_index() {
    if (messages.empty()) {
        first_visible_index = 0;
        return;
    }

    if (first_visible_index < 0) {
        first_visible_index = 0;
    }

    int maximum_first_index = calculate_first_index_for_bottom();
    if (maximum_first_index < 0) {
        maximum_first_index = 0;
    }

    if (first_visible_index > maximum_first_index) {
        first_visible_index = maximum_first_index;
    }
}

void ConversationView::update_scrollbar_state(int visible_count) {
    int maximum_first_index = calculate_first_index_for_bottom();
    if (maximum_first_index < 0) {
        maximum_first_index = 0;
    }

    if (visible_count < 1) {
        visible_count = 1;
    }

    vertical_scroll_bar.set_range(
        0,
        maximum_first_index,
        visible_count
    );
    vertical_scroll_bar.set_value(first_visible_index);
    sync_native_scrollbar();
}

void ConversationView::sync_native_scrollbar() {
    if (native_control_host == 0) {
        return;
    }

    native_control_host->sync_scroll_bar(&vertical_scroll_bar);
}

int ConversationView::calculate_entry_height(const Label& label) const {
    int maximum_font_size = label.get_max_font_size();
    int line_count = 1;
    const char* text = label.get_text();

    if (text != 0) {
        for (int index = 0; text[index] != '\0'; ++index) {
            if (text[index] == '\n') {
                ++line_count;
            }
        }
    }

    if (line_count <= 1) {
        int row_height = maximum_font_size + 12;
        if (row_height < 24) {
            row_height = 24;
        }
        return row_height;
    }

    int line_height = maximum_font_size + 8;
    if (line_height < 20) {
        line_height = 20;
    }

    return (line_count * line_height) + ((line_count - 1) * 2) + 8;
}

int ConversationView::calculate_first_index_for_bottom() const {
    int message_count = (int)messages.size();
    if (message_count <= 0) {
        return 0;
    }

    int usable_height = get_height() - (content_padding * 2);
    if (usable_height <= 0) {
        return message_count - 1;
    }

    int used_height = 0;
    int first_index = message_count - 1;

    for (int index = message_count - 1; index >= 0; --index) {
        int extent = messages[index].row_height;
        if (index < message_count - 1) {
            extent += row_spacing;
        }

        if (used_height + extent > usable_height) {
            break;
        }

        used_height += extent;
        first_index = index;
    }

    return first_index;
}

int ConversationView::calculate_total_content_height() const {
    if (messages.empty()) {
        return 0;
    }

    int total_height = 0;

    for (int index = 0; index < (int)messages.size(); ++index) {
        int row_height = messages[index].row_height;
        if (row_height < 1) {
            row_height = 1;
        }

        total_height += row_height;
        if (index + 1 < (int)messages.size()) {
            total_height += row_spacing;
        }
    }

    return total_height;
}

const char* ConversationView::get_role_prefix(MessageRole role) const {
    switch (role) {
        case message_local:
            return "You: ";

        case message_remote:
            return "Remote: ";

        case message_system:
        default:
            return "System: ";
    }
}

const char* ConversationView::get_role_label(MessageRole role) const {
    switch (role) {
        case message_local:
            return "You:";

        case message_remote:
            return "Remote:";

        case message_system:
        default:
            return "System:";
    }
}

Color ConversationView::get_role_color(MessageRole role) const {
    switch (role) {
        case message_local:
            return Color(30, 72, 118);

        case message_remote:
            return Color(46, 101, 58);

        case message_system:
        default:
            return Color(92, 92, 92);
    }
}
