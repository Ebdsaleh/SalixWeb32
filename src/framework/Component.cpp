// =================================================================================
// Filename:    framework/Component.cpp
// Author:      Ebdsaleh
// Description: Implements the semantic base class for framework UI components.
// =================================================================================

#include "Component.h"

Component::Component()
    : x(0),
      y(0),
      width(0),
      height(0),
      is_visible(true) {
}

Component::~Component() {
}

void Component::set_bounds(int new_x, int new_y, int new_width, int new_height) {
    x = new_x;
    y = new_y;
    width = new_width;
    height = new_height;
}

int Component::get_x() const {
    return x;
}

int Component::get_y() const {
    return y;
}

int Component::get_width() const {
    return width;
}

int Component::get_height() const {
    return height;
}

bool Component::contains_point(int point_x, int point_y) const {
    return point_x >= x &&
           point_y >= y &&
           point_x < (x + width) &&
           point_y < (y + height);
}

void Component::set_visible(bool new_is_visible) {
    is_visible = new_is_visible;
}

bool Component::get_is_visible() const {
    return is_visible;
}

ComponentStyle& Component::get_style() {
    return style;
}

const ComponentStyle& Component::get_style() const {
    return style;
}

bool Component::handle_event(const UIEvent& event) {
    (void)event;
    return false;
}
