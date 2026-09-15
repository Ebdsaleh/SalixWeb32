// =================================================================================
// Filename:    app/ConversationView.cpp
// Author:      Ebdsaleh
// Description: Implements the append-only scrollable conversation message surface.
// =================================================================================

#include "ConversationView.h"

ConversationView::ConversationView()
    : first_visible_index(0),
      row_spacing(4),
      content_padding(4),
      scroll_button_width(24) {

    get_style().background_color = Color(255, 255, 255);
    get_style().border_width = 0;

    scroll_up_button.set_text("^");
    scroll_down_button.set_text("v");

    scroll_up_button.get_style().background_color = Color(232, 241, 249);
    scroll_up_button.get_style().foreground_color = Color(35, 76, 112);
    scroll_up_button.get_style().border_color = Color(168, 194, 216);

    scroll_down_button.get_style().background_color = Color(232, 241, 249);
    scroll_down_button.get_style().foreground_color = Color(35, 76, 112);
    scroll_down_button.get_style().border_color = Color(168, 194, 216);

    scroll_up_button.set_click_handler(
        ConversationView::on_scroll_up,
        this
    );
    scroll_down_button.set_click_handler(
        ConversationView::on_scroll_down,
        this
    );

    add_child(&scroll_up_button);
    add_child(&scroll_down_button);
}

ConversationView::~ConversationView() {
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

    FormattedText display_text;
    display_text.append_plain_text(
        get_role_prefix(role),
        TextFormat(false, false, false, 12)
    );
    display_text.append_formatted_text(text);

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
    relayout();
}

int ConversationView::get_message_count() const {
    return (int)messages.size();
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
    relayout();
}

void ConversationView::scroll_to_bottom() {
    first_visible_index = calculate_first_index_for_bottom();
    relayout();
}

void ConversationView::on_scroll_up(Button* button, void* context) {
    (void)button;

    ConversationView* conversation_view = (ConversationView*)context;
    if (conversation_view != 0) {
        conversation_view->scroll_lines(-1);
    }
}

void ConversationView::on_scroll_down(Button* button, void* context) {
    (void)button;

    ConversationView* conversation_view = (ConversationView*)context;
    if (conversation_view != 0) {
        conversation_view->scroll_lines(1);
    }
}

void ConversationView::relayout() {
    clamp_first_visible_index();

    int view_x = get_x();
    int view_y = get_y();
    int view_width = get_width();
    int view_height = get_height();
    int message_count = (int)messages.size();

    int usable_bottom = view_y + view_height - content_padding;
    int current_y = view_y + content_padding;
    int last_visible_index = first_visible_index - 1;
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
        last_visible_index = index;
        current_y += row_height + row_spacing;
    }

    bool has_previous = first_visible_index > 0;
    bool has_next = last_visible_index < message_count - 1;
    bool show_scroll_controls = has_previous || has_next;

    int scroll_gap = show_scroll_controls ? 6 : 0;
    int reserved_scroll_width = show_scroll_controls
        ? scroll_button_width + scroll_gap
        : 0;

    int message_x = view_x + content_padding;
    int message_width = view_width - (content_padding * 2) - reserved_scroll_width;
    if (message_width < 0) {
        message_width = 0;
    }

    current_y = view_y + content_padding;

    for (int index = first_visible_index;
         index <= last_visible_index && index < message_count;
         ++index) {
        Label* label = messages[index].label;
        if (label == 0 || !label->get_is_visible()) {
            continue;
        }

        int row_height = messages[index].row_height;
        label->set_bounds(
            message_x,
            current_y,
            message_width,
            row_height
        );
        current_y += row_height + row_spacing;
    }

    scroll_up_button.set_visible(show_scroll_controls);
    scroll_down_button.set_visible(show_scroll_controls);

    if (show_scroll_controls) {
        int button_x = view_x + view_width - content_padding - scroll_button_width;
        int available_height = view_height - (content_padding * 2);
        int button_height = 24;

        if (available_height < 52) {
            button_height = available_height / 2;
        }

        if (button_height < 0) {
            button_height = 0;
        }

        scroll_up_button.set_bounds(
            button_x,
            view_y + content_padding,
            scroll_button_width,
            button_height
        );

        scroll_down_button.set_bounds(
            button_x,
            view_y + view_height - content_padding - button_height,
            scroll_button_width,
            button_height
        );

        scroll_up_button.set_enabled(has_previous);
        scroll_down_button.set_enabled(has_next);
    }
}

void ConversationView::clamp_first_visible_index() {
    if (messages.empty()) {
        first_visible_index = 0;
        return;
    }

    if (first_visible_index < 0) {
        first_visible_index = 0;
    }

    int maximum_first_index = (int)messages.size() - 1;
    if (first_visible_index > maximum_first_index) {
        first_visible_index = maximum_first_index;
    }
}

int ConversationView::calculate_entry_height(const Label& label) const {
    int row_height = label.get_max_font_size() + 12;
    if (row_height < 24) {
        row_height = 24;
    }

    return row_height;
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
