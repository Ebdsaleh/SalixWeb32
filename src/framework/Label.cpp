// =================================================================================
// Filename:    framework/Label.cpp
// Author:      Ebdsaleh
// Description: Implements a backend-neutral text label component.
// =================================================================================

#include "Label.h"
#include "UIEvent.h"
#include "Clipboard.h"
#include "MimeData.h"
#include "TextMetrics.h"
#include "TextNavigation.h"
#include "rendering/ComponentRenderer.h"

Label::Label()
    : horizontal_alignment(align_left),
      is_selectable(false),
      is_focused(false),
      cursor_position(0),
      selection_anchor(0),
      is_mouse_selecting(false) {
}

void Label::set_text(const char* new_text) {
    if (new_text == 0) {
        text.clear();
        cursor_position = 0;
        selection_anchor = 0;
        return;
    }

    text = new_text;
    cursor_position = (int)text.length();
    selection_anchor = cursor_position;
}

const char* Label::get_text() const {
    return text.c_str();
}

void Label::set_horizontal_alignment(HorizontalAlignment new_alignment) {
    horizontal_alignment = new_alignment;
}

Label::HorizontalAlignment Label::get_horizontal_alignment() const {
    return horizontal_alignment;
}

void Label::set_selectable(bool new_is_selectable) {
    is_selectable = new_is_selectable;

    if (!is_selectable) {
        is_focused = false;
        is_mouse_selecting = false;
        selection_anchor = cursor_position;
    }
}

bool Label::get_is_selectable() const {
    return is_selectable;
}

bool Label::get_is_focused() const {
    return is_focused;
}

int Label::get_cursor_position() const {
    return cursor_position;
}

bool Label::has_selection() const {
    return cursor_position != selection_anchor;
}

int Label::get_selection_start() const {
    if (cursor_position < selection_anchor) {
        return cursor_position;
    }

    return selection_anchor;
}

int Label::get_selection_end() const {
    if (cursor_position > selection_anchor) {
        return cursor_position;
    }

    return selection_anchor;
}

void Label::clear_selection() {
    selection_anchor = cursor_position;
}

void Label::select_all() {
    selection_anchor = 0;
    cursor_position = (int)text.length();
}

bool Label::handle_event(const UIEvent& event) {
    if (!get_is_visible() || !is_selectable) {
        return false;
    }

    if (event.type == UIEvent::event_mouse_down) {
        bool new_is_focused = contains_point(event.x, event.y);
        bool did_change = new_is_focused != is_focused;

        if (new_is_focused) {
            int new_cursor_position = get_cursor_position_from_event(event);

            if (!event.shift_down || !is_focused) {
                selection_anchor = new_cursor_position;
            }

            cursor_position = new_cursor_position;
            is_mouse_selecting = true;
        } else {
            is_mouse_selecting = false;
        }

        is_focused = new_is_focused;
        return new_is_focused || did_change;
    }

    if (
        event.type == UIEvent::event_mouse_move &&
        is_focused &&
        is_mouse_selecting &&
        event.left_button_down
    ) {
        cursor_position = get_cursor_position_from_event(event);
        return true;
    }

    if (event.type == UIEvent::event_mouse_up && is_mouse_selecting) {
        if (is_focused) {
            cursor_position = get_cursor_position_from_event(event);
        }

        is_mouse_selecting = false;
        return true;
    }

    if (!is_focused || event.type != UIEvent::event_key_down) {
        return false;
    }

    if (event.control_down) {
        switch (event.key_code) {
            case UIEvent::key_left:
                move_cursor(
                    TextNavigation::find_word_boundary_left(
                        text,
                        cursor_position
                    ),
                    event.shift_down
                );
                return true;

            case UIEvent::key_right:
                move_cursor(
                    TextNavigation::find_word_boundary_right(
                        text,
                        cursor_position
                    ),
                    event.shift_down
                );
                return true;

            case UIEvent::key_a:
                select_all();
                return true;

            case UIEvent::key_c:
                copy_selection(event.clipboard);
                return true;

            default:
                break;
        }
    }

    switch (event.key_code) {
        case UIEvent::key_left:
            if (!event.shift_down && has_selection()) {
                move_cursor(get_selection_start(), false);
            } else {
                move_cursor(cursor_position - 1, event.shift_down);
            }
            return true;

        case UIEvent::key_right:
            if (!event.shift_down && has_selection()) {
                move_cursor(get_selection_end(), false);
            } else {
                move_cursor(cursor_position + 1, event.shift_down);
            }
            return true;

        case UIEvent::key_home:
            move_cursor(0, event.shift_down);
            return true;

        case UIEvent::key_end:
            move_cursor((int)text.length(), event.shift_down);
            return true;

        default:
            break;
    }

    return false;
}

void Label::render(ComponentRenderer& renderer) const {
    if (!get_is_visible()) {
        return;
    }

    renderer.render_label(*this);
}

void Label::clamp_cursor_position() {
    if (cursor_position < 0) {
        cursor_position = 0;
    }

    if (cursor_position > (int)text.length()) {
        cursor_position = (int)text.length();
    }
}

void Label::move_cursor(int new_cursor_position, bool extend_selection) {
    cursor_position = new_cursor_position;
    clamp_cursor_position();

    if (!extend_selection) {
        selection_anchor = cursor_position;
    }
}

int Label::get_cursor_position_from_event(const UIEvent& event) const {
    if (event.text_metrics == 0) {
        return (int)text.length();
    }

    int text_width = event.text_metrics->measure_text_width(
        text.c_str(),
        (int)text.length()
    );

    int text_x = get_x();

    switch (horizontal_alignment) {
        case align_center:
            text_x += (get_width() - text_width) / 2;
            break;

        case align_right:
            text_x += get_width() - text_width;
            break;

        case align_left:
        default:
            break;
    }

    return event.text_metrics->get_character_index_at_x(
        text.c_str(),
        (int)text.length(),
        event.x - text_x
    );
}

bool Label::copy_selection(Clipboard* clipboard) const {
    if (clipboard == 0 || !has_selection()) {
        return false;
    }

    int selection_start = get_selection_start();
    int selection_end = get_selection_end();

    std::string selected_text = text.substr(
        selection_start,
        selection_end - selection_start
    );

    MimeData data;
    data.set_text(selected_text.c_str());
    return clipboard->set_data(data);
}
