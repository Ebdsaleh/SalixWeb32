// =================================================================================
// Filename:    app/ConversationView.cpp
// Author:      Ebdsaleh
// Description: Implements the append-only scrollable conversation message surface.
// =================================================================================

#include <string>

#include "ConversationView.h"

ConversationView::ConversationView()
    : first_visible_index(0),
      visible_capacity(0),
      row_height(24),
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

bool ConversationView::append_message(MessageRole role, const char* text) {
    if (text == 0 || text[0] == '\0') {
        return false;
    }

    Label* message_label = new Label();
    if (message_label == 0) {
        return false;
    }

    std::string display_text = get_role_prefix(role);
    display_text += text;

    message_label->set_text(display_text.c_str());
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

bool ConversationView::append_remote_message(const char* text) {
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
    visible_capacity = calculate_visible_capacity();

    if (visible_capacity <= 0) {
        first_visible_index = 0;
    } else {
        first_visible_index = (int)messages.size() - visible_capacity;
        if (first_visible_index < 0) {
            first_visible_index = 0;
        }
    }

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
    visible_capacity = calculate_visible_capacity();
    clamp_first_visible_index();

    int view_x = get_x();
    int view_y = get_y();
    int view_width = get_width();
    int view_height = get_height();
    int message_count = (int)messages.size();

    bool show_scroll_controls =
        visible_capacity > 0 &&
        message_count > visible_capacity;

    scroll_up_button.set_visible(show_scroll_controls);
    scroll_down_button.set_visible(show_scroll_controls);

    int scroll_gap = show_scroll_controls ? 6 : 0;
    int reserved_scroll_width = show_scroll_controls ? scroll_button_width + scroll_gap : 0;

    int message_x = view_x + content_padding;
    int message_width = view_width - (content_padding * 2) - reserved_scroll_width;
    if (message_width < 0) {
        message_width = 0;
    }

    for (int index = 0; index < message_count; ++index) {
        Label* label = messages[index].label;
        if (label == 0) {
            continue;
        }

        bool is_visible =
            visible_capacity > 0 &&
            index >= first_visible_index &&
            index < first_visible_index + visible_capacity;

        label->set_visible(is_visible);

        if (is_visible) {
            int visible_index = index - first_visible_index;
            int message_y = view_y + content_padding +
                (visible_index * (row_height + row_spacing));

            label->set_bounds(
                message_x,
                message_y,
                message_width,
                row_height
            );
        }
    }

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
    }
}

void ConversationView::clamp_first_visible_index() {
    int maximum_first_index = 0;

    if (visible_capacity > 0) {
        maximum_first_index = (int)messages.size() - visible_capacity;
        if (maximum_first_index < 0) {
            maximum_first_index = 0;
        }
    }

    if (first_visible_index < 0) {
        first_visible_index = 0;
    }

    if (first_visible_index > maximum_first_index) {
        first_visible_index = maximum_first_index;
    }
}

int ConversationView::calculate_visible_capacity() const {
    int usable_height = get_height() - (content_padding * 2);
    if (usable_height <= 0 || row_height <= 0) {
        return 0;
    }

    int row_extent = row_height + row_spacing;
    if (row_extent <= 0) {
        return 0;
    }

    int capacity = (usable_height + row_spacing) / row_extent;
    if (capacity < 0) {
        capacity = 0;
    }

    return capacity;
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
