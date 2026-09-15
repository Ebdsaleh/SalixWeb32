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

void Component::set_visible(bool new_is_visible) {
    is_visible = new_is_visible;
}

bool Component::get_is_visible() const {
    return is_visible;
}
