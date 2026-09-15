// =================================================================================
// Filename:    framework/Button.cpp
// Author:      Ebdsaleh
// Description: Implements a backend-neutral clickable button component.
// =================================================================================

#include "Button.h"
#include "UIEvent.h"
#include "rendering/ComponentRenderer.h"

Button::Button()
    : is_pressed(false),
      is_hovered(false),
      click_handler(0),
      click_context(0) {

    get_style().background_color = Color(224, 224, 224);
    get_style().border_color = Color(96, 96, 96);
}

void Button::set_text(const char* new_text) {
    if (new_text == 0) {
        text.clear();
        return;
    }

    text = new_text;
}

const char* Button::get_text() const {
    return text.c_str();
}

void Button::set_click_handler(ClickHandler new_click_handler, void* new_context) {
    click_handler = new_click_handler;
    click_context = new_context;
}

bool Button::get_is_pressed() const {
    return is_pressed;
}

bool Button::get_is_hovered() const {
    return is_hovered;
}

bool Button::handle_event(const UIEvent& event) {
    if (!get_is_visible()) {
        return false;
    }

    if (event.type == UIEvent::event_mouse_move) {
        bool new_is_hovered = contains_point(event.x, event.y);
        bool did_change = new_is_hovered != is_hovered;
        is_hovered = new_is_hovered;
        return did_change;
    }

    if (event.type == UIEvent::event_mouse_down) {
        bool was_pressed = is_pressed;
        is_pressed = contains_point(event.x, event.y);
        return is_pressed || was_pressed;
    }

    if (event.type == UIEvent::event_mouse_up) {
        bool was_pressed = is_pressed;
        bool is_inside = contains_point(event.x, event.y);
        is_pressed = false;

        if (was_pressed && is_inside && click_handler != 0) {
            click_handler(this, click_context);
        }

        return was_pressed || is_inside;
    }

    return false;
}

void Button::render(ComponentRenderer& renderer) const {
    if (!get_is_visible()) {
        return;
    }

    renderer.render_button(*this);
}
