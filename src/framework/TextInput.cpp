// =================================================================================
// Filename:    framework/TextInput.cpp
// Author:      Ebdsaleh
// Description: Implements a backend-neutral single-line text input component.
// =================================================================================

#include "TextInput.h"
#include "UIEvent.h"
#include "rendering/ComponentRenderer.h"

TextInput::TextInput()
    : max_length(120),
      is_focused(false),
      cursor_position(0) {

    get_style().background_color = Color(255, 255, 255);
    get_style().border_color = Color(112, 112, 112);
}

void TextInput::set_text(const char* new_text) {
    if (new_text == 0) {
        text.clear();
        cursor_position = 0;
        return;
    }

    text = new_text;

    if ((int)text.length() > max_length) {
        text.erase(max_length);
    }

    cursor_position = (int)text.length();
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
}

int TextInput::get_max_length() const {
    return max_length;
}

void TextInput::set_focused(bool new_is_focused) {
    is_focused = new_is_focused;
    clamp_cursor_position();
}

bool TextInput::get_is_focused() const {
    return is_focused;
}

void TextInput::set_cursor_position(int new_cursor_position) {
    cursor_position = new_cursor_position;
    clamp_cursor_position();
}

int TextInput::get_cursor_position() const {
    return cursor_position;
}

bool TextInput::handle_event(const UIEvent& event) {
    if (!get_is_visible()) {
        return false;
    }

    if (event.type == UIEvent::event_mouse_down) {
        bool new_is_focused = contains_point(event.x, event.y);
        bool did_change = new_is_focused != is_focused;

        if (new_is_focused && !is_focused) {
            cursor_position = (int)text.length();
        }

        is_focused = new_is_focused;
        return new_is_focused || did_change;
    }

    if (!is_focused) {
        return false;
    }

    if (event.type == UIEvent::event_key_down) {
        switch (event.key_code) {
            case UIEvent::key_left:
                if (cursor_position > 0) {
                    --cursor_position;
                }
                return true;

            case UIEvent::key_right:
                if (cursor_position < (int)text.length()) {
                    ++cursor_position;
                }
                return true;

            case UIEvent::key_home:
                cursor_position = 0;
                return true;

            case UIEvent::key_end:
                cursor_position = (int)text.length();
                return true;

            case UIEvent::key_delete:
                if (cursor_position < (int)text.length()) {
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
        if (cursor_position > 0 && !text.empty()) {
            text.erase(cursor_position - 1, 1);
            --cursor_position;
        }
        return true;
    }

    if (
        event.character_code >= 32 &&
        event.character_code <= 126 &&
        (int)text.length() < max_length
    ) {
        text.insert(
            (std::string::size_type)cursor_position,
            1,
            (char)event.character_code
        );
        ++cursor_position;
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
