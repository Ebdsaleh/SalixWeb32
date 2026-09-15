// =================================================================================
// Filename:    framework/ToggleButton.cpp
// Author:      Ebdsaleh
// Description: Implements a backend-neutral toggleable button component.
// =================================================================================

#include "ToggleButton.h"
#include "UIEvent.h"

ToggleButton::ToggleButton()
    : is_checked(false),
      checked_background_color(181, 216, 240),
      unchecked_background_color(238, 245, 251) {

    update_visual_state();
}

void ToggleButton::set_checked(bool new_is_checked) {
    is_checked = new_is_checked;
    update_visual_state();
}

bool ToggleButton::get_is_checked() const {
    return is_checked;
}

void ToggleButton::set_checked_background_color(const Color& color) {
    checked_background_color = color;
    update_visual_state();
}

void ToggleButton::set_unchecked_background_color(const Color& color) {
    unchecked_background_color = color;
    update_visual_state();
}

bool ToggleButton::handle_event(const UIEvent& event) {
    bool was_pressed = get_is_pressed();
    bool was_handled = Button::handle_event(event);

    if (
        event.type == UIEvent::event_mouse_up &&
        get_is_enabled() &&
        was_pressed &&
        contains_point(event.x, event.y)
    ) {
        set_checked(!is_checked);
        return true;
    }

    return was_handled;
}

void ToggleButton::update_visual_state() {
    get_style().background_color = is_checked
        ? checked_background_color
        : unchecked_background_color;
}
