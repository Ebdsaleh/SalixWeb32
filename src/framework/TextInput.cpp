// =================================================================================
// Filename:    framework/TextInput.cpp
// Author:      Ebdsaleh
// Description: Implements a backend-neutral single-line text input component.
// =================================================================================

#include <string.h>

#include "TextInput.h"
#include "UIEvent.h"
#include "Clipboard.h"
#include "MimeData.h"
#include "TextMetrics.h"
#include "TextNavigation.h"
#include "rendering/ComponentRenderer.h"

TextInput::TextInput()
    : max_length(120),
      is_focused(false),
      cursor_position(0),
      selection_anchor(0),
      is_mouse_selecting(false),
      text_padding(6) {

    get_style().background_color = Color(255, 255, 255);
    get_style().border_color = Color(112, 112, 112);
}

void TextInput::set_text(const char* new_text) {
    if (new_text == 0) {
        text.clear();
        cursor_position = 0;
        selection_anchor = 0;
        return;
    }

    text = new_text;

    if ((int)text.length() > max_length) {
        text.erase(max_length);
    }

    cursor_position = (int)text.length();
    selection_anchor = cursor_position;
}

const char* TextInput::get_text() const {
    return text.c_str();
}

void TextInput::set_max_length(int new_max_length) {
    if (new_max_length < 0) {
        new_max_length = 0;
    }

    max_length = new_max_length;

    if ((int)text.length() > max_length) {
        text.erase(max_length);
    }

    clamp_cursor_position();
    if (selection_anchor > (int)text.length()) {
        selection_anchor = (int)text.length();
    }
}

int TextInput::get_max_length() const {
    return max_length;
}

void TextInput::set_focused(bool new_is_focused) {
    is_focused = new_is_focused;
    clamp_cursor_position();

    if (!is_focused) {
        is_mouse_selecting = false;
    }
}

bool TextInput::get_is_focused() const {
    return is_focused;
}

void TextInput::set_cursor_position(int new_cursor_position) {
    cursor_position = new_cursor_position;
    clamp_cursor_position();
    selection_anchor = cursor_position;
}

int TextInput::get_cursor_position() const {
    return cursor_position;
}

bool TextInput::has_selection() const {
    return cursor_position != selection_anchor;
}

int TextInput::get_selection_start() const {
    if (cursor_position < selection_anchor) {
        return cursor_position;
    }

    return selection_anchor;
}

int TextInput::get_selection_end() const {
    if (cursor_position > selection_anchor) {
        return cursor_position;
    }

    return selection_anchor;
}

void TextInput::clear_selection() {
    selection_anchor = cursor_position;
}

void TextInput::select_all() {
    selection_anchor = 0;
    cursor_position = (int)text.length();
}

bool TextInput::accepts_mime_type(const char* mime_type) const {
    if (mime_type == 0) {
        return false;
    }

    return strcmp(mime_type, "text/plain") == 0;
}

bool TextInput::insert_mime_data(const MimeData& data) {
    if (!data.has_format("text/plain")) {
        return false;
    }

    return insert_plain_text(data.get_text());
}

int TextInput::get_text_padding() const {
    return text_padding;
}

bool TextInput::handle_event(const UIEvent& event) {
    if (!get_is_visible()) {
        return false;
    }

    if (event.type == UIEvent::event_mouse_down) {
        bool new_is_focused = contains_point(event.x, event.y);
        bool did_change = new_is_focused != is_focused;

        if (new_is_focused) {
            int new_cursor_position = get_cursor_position_from_event(event);

            if (event.click_count >= 3) {
                select_all();
                is_mouse_selecting = false;
            } else if (event.click_count == 2) {
                selection_anchor = TextNavigation::find_word_start(
                    text,
                    new_cursor_position
                );
                cursor_position = TextNavigation::find_word_end(
                    text,
                    new_cursor_position
                );
                is_mouse_selecting = false;
            } else {
                if (!event.shift_down || !is_focused) {
                    selection_anchor = new_cursor_position;
                }

                cursor_position = new_cursor_position;
                is_mouse_selecting = true;
            }
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

    if (!is_focused) {
        return false;
    }

    if (event.type == UIEvent::event_key_down) {
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

                case UIEvent::key_x:
                    cut_selection(event.clipboard);
                    return true;

                case UIEvent::key_v:
                    paste_from_clipboard(event.clipboard);
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

            case UIEvent::key_delete:
                if (has_selection()) {
                    delete_selection();
                } else if (cursor_position < (int)text.length()) {
                    text.erase(cursor_position, 1);
                }
                return true;

            default:
                break;
        }
    }

    if (event.type != UIEvent::event_character) {
        return false;
    }

    if (event.character_code == 8) {
        if (has_selection()) {
            delete_selection();
        } else if (cursor_position > 0 && !text.empty()) {
            text.erase(cursor_position - 1, 1);
            --cursor_position;
            selection_anchor = cursor_position;
        }
        return true;
    }

    if (
        event.character_code >= 32 &&
        event.character_code <= 126
    ) {
        char character_text[2];
        character_text[0] = (char)event.character_code;
        character_text[1] = '\0';
        insert_plain_text(character_text);
        return true;
    }

    return true;
}

void TextInput::render(ComponentRenderer& renderer) const {
    if (!get_is_visible()) {
        return;
    }

    renderer.render_text_input(*this);
}

void TextInput::clamp_cursor_position() {
    if (cursor_position < 0) {
        cursor_position = 0;
    }

    if (cursor_position > (int)text.length()) {
        cursor_position = (int)text.length();
    }
}

void TextInput::move_cursor(int new_cursor_position, bool extend_selection) {
    cursor_position = new_cursor_position;
    clamp_cursor_position();

    if (!extend_selection) {
        selection_anchor = cursor_position;
    }
}

void TextInput::delete_selection() {
    if (!has_selection()) {
        return;
    }

    int selection_start = get_selection_start();
    int selection_end = get_selection_end();

    text.erase(
        selection_start,
        selection_end - selection_start
    );

    cursor_position = selection_start;
    selection_anchor = cursor_position;
}

bool TextInput::insert_plain_text(const char* new_text) {
    if (new_text == 0) {
        return false;
    }

    if (has_selection()) {
        delete_selection();
    }

    int remaining_capacity = max_length - (int)text.length();
    if (remaining_capacity <= 0) {
        return true;
    }

    std::string filtered_text;

    for (int index = 0; new_text[index] != '\0'; ++index) {
        char character = new_text[index];

        if (character == '\r') {
            continue;
        }

        if (character == '\n' || character == '\t') {
            character = ' ';
        }

        filtered_text += character;

        if ((int)filtered_text.length() >= remaining_capacity) {
            break;
        }
    }

    if (filtered_text.empty()) {
        return true;
    }

    text.insert(
        (std::string::size_type)cursor_position,
        filtered_text
    );

    cursor_position += (int)filtered_text.length();
    selection_anchor = cursor_position;
    return true;
}

int TextInput::get_cursor_position_from_event(const UIEvent& event) const {
    if (event.text_metrics == 0) {
        return (int)text.length();
    }

    int relative_x = event.x - get_x() - text_padding;

    return event.text_metrics->get_character_index_at_x(
        text.c_str(),
        (int)text.length(),
        relative_x
    );
}

bool TextInput::copy_selection(Clipboard* clipboard) const {
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

bool TextInput::cut_selection(Clipboard* clipboard) {
    if (!has_selection()) {
        return false;
    }

    if (!copy_selection(clipboard)) {
        return false;
    }

    delete_selection();
    return true;
}

bool TextInput::paste_from_clipboard(Clipboard* clipboard) {
    if (clipboard == 0) {
        return false;
    }

    MimeData data;
    if (!clipboard->get_data(data)) {
        return false;
    }

    return insert_mime_data(data);
}
