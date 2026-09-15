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
      is_focused(false) {

    get_style().background_color = Color(255, 255, 255);
    get_style().border_color = Color(112, 112, 112);
}

void TextInput::set_text(const char* new_text) {
    if (new_text == 0) {
        text.clear();
        return;
    }

    text = new_text;

    if ((int)text.length() > max_length) {
        text.erase(max_length);
    }
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
}

int TextInput::get_max_length() const {
    return max_length;
}

void TextInput::set_focused(bool new_is_focused) {
    is_focused = new_is_focused;
}

bool TextInput::get_is_focused() const {
    return is_focused;
}

bool TextInput::handle_event(const UIEvent& event) {
    if (!get_is_visible()) {
        return false;
    }

    if (event.type == UIEvent::event_mouse_down) {
        bool new_is_focused = contains_point(event.x, event.y);
        bool did_change = new_is_focused != is_focused;
        is_focused = new_is_focused;
        return new_is_focused || did_change;
    }

    if (event.type != UIEvent::event_character || !is_focused) {
        return false;
    }

    if (event.character_code == 8) {
        if (!text.empty()) {
            text.erase(text.length() - 1, 1);
        }
        return true;
    }

    if (
        event.character_code >= 32 &&
        event.character_code <= 126 &&
        (int)text.length() < max_length
    ) {
        text += (char)event.character_code;
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
