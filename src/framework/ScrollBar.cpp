// =================================================================================
// Filename:    framework/ScrollBar.cpp
// Author:      Ebdsaleh
// Description: Implements a backend-neutral scrollbar with optional native peer.
// =================================================================================

#include "ScrollBar.h"
#include "UIEvent.h"

namespace {
    int clamp_int(int value, int minimum, int maximum) {
        if (value < minimum) {
            return minimum;
        }
        if (value > maximum) {
            return maximum;
        }
        return value;
    }
}

ScrollBar::ScrollBar(Orientation new_orientation)
    : orientation(new_orientation),
      minimum(0),
      maximum(0),
      page_size(1),
      value(0),
      line_step(16),
      track_start(0),
      track_length(0),
      thumb_start(0),
      thumb_length(0),
      dragging_thumb(false),
      drag_offset(0),
      native_peer_active(false),
      value_changed_handler(0),
      value_changed_context(0) {

    get_style().background_color = Color(224, 232, 239);
    get_style().border_color = Color(150, 168, 184);
    get_style().border_width = 1;

    decrement_button.set_text(orientation == vertical ? "^" : "<");
    increment_button.set_text(orientation == vertical ? "v" : ">");

    decrement_button.get_style().background_color = Color(238, 244, 249);
    increment_button.get_style().background_color = Color(238, 244, 249);
    decrement_button.get_style().foreground_color = Color(54, 76, 96);
    increment_button.get_style().foreground_color = Color(54, 76, 96);
    decrement_button.get_style().border_color = Color(150, 168, 184);
    increment_button.get_style().border_color = Color(150, 168, 184);

    thumb_panel.get_style().background_color = Color(188, 207, 222);
    thumb_panel.get_style().border_color = Color(122, 150, 174);
    thumb_panel.get_style().border_width = 1;

    decrement_button.set_click_handler(
        ScrollBar::on_decrement_clicked,
        this
    );
    increment_button.set_click_handler(
        ScrollBar::on_increment_clicked,
        this
    );

    add_child(&decrement_button);
    add_child(&increment_button);
    add_child(&thumb_panel);
    update_fallback_visibility();
}

ScrollBar::Orientation ScrollBar::get_orientation() const {
    return orientation;
}

void ScrollBar::set_range(
    int new_minimum,
    int new_maximum,
    int new_page_size
) {
    if (new_maximum < new_minimum) {
        new_maximum = new_minimum;
    }
    if (new_page_size < 1) {
        new_page_size = 1;
    }

    minimum = new_minimum;
    maximum = new_maximum;
    page_size = new_page_size;
    value = clamp_int(value, minimum, maximum);

    bool enabled = maximum > minimum;
    decrement_button.set_enabled(enabled);
    increment_button.set_enabled(enabled);

    update_thumb_bounds();
}

int ScrollBar::get_minimum() const {
    return minimum;
}

int ScrollBar::get_maximum() const {
    return maximum;
}

int ScrollBar::get_page_size() const {
    return page_size;
}

void ScrollBar::set_value(int new_value) {
    change_value(new_value, false);
}

int ScrollBar::get_value() const {
    return value;
}

void ScrollBar::set_line_step(int new_line_step) {
    if (new_line_step < 1) {
        new_line_step = 1;
    }
    line_step = new_line_step;
}

int ScrollBar::get_line_step() const {
    return line_step;
}

void ScrollBar::set_native_peer_active(bool active) {
    if (native_peer_active == active) {
        return;
    }

    native_peer_active = active;
    dragging_thumb = false;
    update_fallback_visibility();
}

bool ScrollBar::get_native_peer_active() const {
    return native_peer_active;
}

void ScrollBar::notify_native_value_changed(int new_value) {
    change_value(new_value, true);
}

void ScrollBar::set_value_changed_handler(
    ValueChangedHandler new_handler,
    void* new_context
) {
    value_changed_handler = new_handler;
    value_changed_context = new_context;
}

void ScrollBar::arrange(int x, int y, int width, int height) {
    set_bounds(x, y, width, height);

    int length = orientation == vertical ? height : width;
    int thickness = orientation == vertical ? width : height;
    int arrow_extent = thickness;

    if (arrow_extent * 2 > length) {
        arrow_extent = length / 2;
    }
    if (arrow_extent < 0) {
        arrow_extent = 0;
    }

    if (orientation == vertical) {
        decrement_button.set_bounds(x, y, width, arrow_extent);
        increment_button.set_bounds(
            x,
            y + height - arrow_extent,
            width,
            arrow_extent
        );
        track_start = y + arrow_extent;
        track_length = height - (arrow_extent * 2);
    } else {
        decrement_button.set_bounds(x, y, arrow_extent, height);
        increment_button.set_bounds(
            x + width - arrow_extent,
            y,
            arrow_extent,
            height
        );
        track_start = x + arrow_extent;
        track_length = width - (arrow_extent * 2);
    }

    if (track_length < 0) {
        track_length = 0;
    }

    update_thumb_bounds();
    update_fallback_visibility();
}

bool ScrollBar::handle_event(const UIEvent& event) {
    if (!get_is_visible() || native_peer_active) {
        return false;
    }

    int axis_position = get_axis_position(event.x, event.y);

    if (
        event.type == UIEvent::event_mouse_move &&
        dragging_thumb &&
        event.left_button_down
    ) {
        int movable_length = track_length - thumb_length;
        if (movable_length <= 0 || maximum <= minimum) {
            change_value(minimum, true);
            return true;
        }

        int thumb_axis = axis_position - drag_offset;
        int relative = thumb_axis - track_start;
        relative = clamp_int(relative, 0, movable_length);

        int range = maximum - minimum;
        int new_value = minimum +
            ((relative * range) / movable_length);
        change_value(new_value, true);
        return true;
    }

    if (event.type == UIEvent::event_mouse_up && dragging_thumb) {
        dragging_thumb = false;
        return true;
    }

    bool child_handled = Panel::handle_event(event);

    if (event.type != UIEvent::event_mouse_down) {
        return child_handled;
    }

    if (!contains_point(event.x, event.y)) {
        return child_handled;
    }

    if (
        decrement_button.contains_point(event.x, event.y) ||
        increment_button.contains_point(event.x, event.y)
    ) {
        return true;
    }

    if (thumb_panel.contains_point(event.x, event.y)) {
        dragging_thumb = true;
        drag_offset = axis_position - thumb_start;
        return true;
    }

    if (axis_position < thumb_start) {
        change_value(value - page_size, true);
    } else if (axis_position >= thumb_start + thumb_length) {
        change_value(value + page_size, true);
    }

    return true;
}

void ScrollBar::on_decrement_clicked(Button* button, void* context) {
    (void)button;
    ScrollBar* scroll_bar = (ScrollBar*)context;
    if (scroll_bar != 0) {
        scroll_bar->change_value(
            scroll_bar->value - scroll_bar->line_step,
            true
        );
    }
}

void ScrollBar::on_increment_clicked(Button* button, void* context) {
    (void)button;
    ScrollBar* scroll_bar = (ScrollBar*)context;
    if (scroll_bar != 0) {
        scroll_bar->change_value(
            scroll_bar->value + scroll_bar->line_step,
            true
        );
    }
}

void ScrollBar::change_value(int new_value, bool notify) {
    int clamped = clamp_int(new_value, minimum, maximum);
    if (clamped == value) {
        update_thumb_bounds();
        return;
    }

    value = clamped;
    update_thumb_bounds();

    if (notify) {
        notify_value_changed();
    }
}

void ScrollBar::update_thumb_bounds() {
    if (track_length <= 0) {
        thumb_length = 0;
        thumb_start = track_start;
        thumb_panel.set_bounds(0, 0, 0, 0);
        return;
    }

    if (maximum <= minimum) {
        thumb_length = track_length;
        thumb_start = track_start;
    } else {
        int range = maximum - minimum;
        int total_extent = range + page_size;
        if (total_extent < 1) {
            total_extent = 1;
        }

        thumb_length = (track_length * page_size) / total_extent;
        if (thumb_length < 12) {
            thumb_length = 12;
        }
        if (thumb_length > track_length) {
            thumb_length = track_length;
        }

        int movable_length = track_length - thumb_length;
        thumb_start = track_start;
        if (movable_length > 0) {
            thumb_start +=
                ((value - minimum) * movable_length) / range;
        }
    }

    if (orientation == vertical) {
        thumb_panel.set_bounds(
            get_x(),
            thumb_start,
            get_width(),
            thumb_length
        );
    } else {
        thumb_panel.set_bounds(
            thumb_start,
            get_y(),
            thumb_length,
            get_height()
        );
    }
}

void ScrollBar::update_fallback_visibility() {
    bool visible = get_is_visible() && !native_peer_active;
    decrement_button.set_visible(visible);
    increment_button.set_visible(visible);
    thumb_panel.set_visible(visible);
}

void ScrollBar::notify_value_changed() {
    if (value_changed_handler != 0) {
        value_changed_handler(this, value, value_changed_context);
    }
}

int ScrollBar::get_axis_position(int x, int y) const {
    return orientation == vertical ? y : x;
}
