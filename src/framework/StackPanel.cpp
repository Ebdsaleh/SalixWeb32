// =================================================================================
// Filename:    framework/StackPanel.cpp
// Author:      Ebdsaleh
// Description: Implements a simple row/column stack layout container.
// =================================================================================

#include "StackPanel.h"

StackPanel::StackPanel()
    : orientation(orientation_vertical),
      item_extent(30),
      spacing(0) {
}

void StackPanel::set_orientation(Orientation new_orientation) {
    orientation = new_orientation;
}

StackPanel::Orientation StackPanel::get_orientation() const {
    return orientation;
}

void StackPanel::set_item_extent(int new_item_extent) {
    if (new_item_extent < 0) {
        new_item_extent = 0;
    }

    item_extent = new_item_extent;
}

int StackPanel::get_item_extent() const {
    return item_extent;
}

void StackPanel::set_spacing(int new_spacing) {
    if (new_spacing < 0) {
        new_spacing = 0;
    }

    spacing = new_spacing;
}

int StackPanel::get_spacing() const {
    return spacing;
}

void StackPanel::arrange(int x, int y, int width, int height) {
    set_bounds(x, y, width, height);

    if (orientation == orientation_horizontal) {
        arrange_horizontal();
        return;
    }

    arrange_vertical();
}

void StackPanel::arrange_vertical() {
    int child_count = get_child_count();
    if (child_count <= 0) {
        return;
    }

    int total_height = (child_count * item_extent) + ((child_count - 1) * spacing);
    int current_y = get_y() + ((get_height() - total_height) / 2);

    for (int index = 0; index < child_count; ++index) {
        Component* child = get_child(index);
        if (child != 0) {
            child->set_bounds(
                get_x(),
                current_y,
                get_width(),
                item_extent
            );
        }

        current_y += item_extent + spacing;
    }
}

void StackPanel::arrange_horizontal() {
    int child_count = get_child_count();
    if (child_count <= 0) {
        return;
    }

    int total_width = (child_count * item_extent) + ((child_count - 1) * spacing);
    int current_x = get_x() + ((get_width() - total_width) / 2);

    for (int index = 0; index < child_count; ++index) {
        Component* child = get_child(index);
        if (child != 0) {
            child->set_bounds(
                current_x,
                get_y(),
                item_extent,
                get_height()
            );
        }

        current_x += item_extent + spacing;
    }
}
