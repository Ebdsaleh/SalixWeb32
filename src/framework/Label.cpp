// =================================================================================
// Filename:    framework/Label.cpp
// Author:      Ebdsaleh
// Description: Implements a backend-neutral text label component.
// =================================================================================

#include "Label.h"
#include "UIEvent.h"
#include "Clipboard.h"
#include "MimeData.h"
#include "MimeTypes.h"
#include "TextMetrics.h"
#include "TextNavigation.h"
#include "rendering/ComponentRenderer.h"

Label::Label()
    : horizontal_alignment(align_left),
      is_selectable(false),
      is_focused(false),
      is_mouse_selecting(false),
      is_mouse_deselecting(false),
      mouse_deselect_anchor(0) {
}

void Label::set_text(const char* new_text) {
    if (new_text == 0) {
        text.clear();
        selection.reset(0, 0);
        return;
    }

    text = new_text;
    selection.reset((int)text.length(), (int)text.length());
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
        is_mouse_deselecting = false;
        selection.clear_selection();
    }
}

bool Label::get_is_selectable() const {
    return is_selectable;
}

bool Label::get_is_focused() const {
    return is_focused;
}

int Label::get_cursor_position() const {
    return selection.get_caret_position();
}

bool Label::has_selection() const {
    return selection.has_selection();
}

int Label::get_selection_start() const {
    return selection.get_first_selection_start();
}

int Label::get_selection_end() const {
    return selection.get_last_selection_end();
}

int Label::get_selection_range_count() const {
    return selection.get_range_count();
}

bool Label::get_selection_range(
    int index,
    int& start,
    int& end
) const {
    return selection.get_range(index, start, end);
}

void Label::clear_selection() {
    selection.clear_selection();
}

void Label::select_all() {
    selection.select_all((int)text.length());
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
            int text_length = (int)text.length();

            if (event.control_down && event.alt_down) {
                selection.preserve_active_range(text_length);

                if (event.click_count >= 3) {
                    selection.remove_range(
                        TextNavigation::find_line_start(
                            text,
                            new_cursor_position
                        ),
                        TextNavigation::find_line_end(
                            text,
                            new_cursor_position
                        ),
                        text_length
                    );
                    selection.move_caret_preserving_selection(
                        new_cursor_position,
                        text_length
                    );
                    is_mouse_deselecting = false;
                } else if (event.click_count == 2) {
                    selection.remove_range(
                        TextNavigation::find_word_start(
                            text,
                            new_cursor_position
                        ),
                        TextNavigation::find_word_end(
                            text,
                            new_cursor_position
                        ),
                        text_length
                    );
                    selection.move_caret_preserving_selection(
                        new_cursor_position,
                        text_length
                    );
                    is_mouse_deselecting = false;
                } else {
                    selection.move_caret_preserving_selection(
                        new_cursor_position,
                        text_length
                    );
                    mouse_deselect_anchor = new_cursor_position;
                    is_mouse_deselecting = true;
                }

                is_mouse_selecting = false;
            } else if (event.control_down) {
                if (event.click_count >= 3) {
                    selection.select_range(
                        TextNavigation::find_line_start(
                            text,
                            new_cursor_position
                        ),
                        TextNavigation::find_line_end(
                            text,
                            new_cursor_position
                        ),
                        text_length,
                        true
                    );
                } else if (event.click_count == 2) {
                    selection.select_range(
                        TextNavigation::find_word_start(
                            text,
                            new_cursor_position
                        ),
                        TextNavigation::find_word_end(
                            text,
                            new_cursor_position
                        ),
                        text_length,
                        true
                    );
                } else {
                    selection.move_caret_preserving_selection(
                        new_cursor_position,
                        text_length
                    );
                }

                is_mouse_selecting = false;
                is_mouse_deselecting = false;
            } else if (event.click_count >= 3) {
                selection.select_range(
                    TextNavigation::find_line_start(
                        text,
                        new_cursor_position
                    ),
                    TextNavigation::find_line_end(
                        text,
                        new_cursor_position
                    ),
                    text_length,
                    false
                );
                is_mouse_selecting = false;
                is_mouse_deselecting = false;
            } else if (event.click_count == 2) {
                selection.select_range(
                    TextNavigation::find_word_start(
                        text,
                        new_cursor_position
                    ),
                    TextNavigation::find_word_end(
                        text,
                        new_cursor_position
                    ),
                    text_length,
                    false
                );
                is_mouse_selecting = false;
                is_mouse_deselecting = false;
            } else if (event.shift_down && is_focused) {
                selection.move_caret(
                    new_cursor_position,
                    text_length,
                    true
                );
                is_mouse_selecting = true;
                is_mouse_deselecting = false;
            } else {
                selection.reset(new_cursor_position, text_length);
                is_mouse_selecting = true;
                is_mouse_deselecting = false;
            }
        } else {
            is_mouse_selecting = false;
            is_mouse_deselecting = false;
        }

        is_focused = new_is_focused;
        return new_is_focused || did_change;
    }

    if (
        event.type == UIEvent::event_mouse_move &&
        is_focused &&
        is_mouse_deselecting &&
        event.left_button_down
    ) {
        int new_cursor_position = get_cursor_position_from_event(event);
        int text_length = (int)text.length();

        selection.remove_range(
            mouse_deselect_anchor,
            new_cursor_position,
            text_length
        );
        selection.move_caret_preserving_selection(
            new_cursor_position,
            text_length
        );
        return true;
    }

    if (
        event.type == UIEvent::event_mouse_up &&
        is_mouse_deselecting
    ) {
        if (is_focused) {
            int new_cursor_position = get_cursor_position_from_event(event);
            int text_length = (int)text.length();

            selection.remove_range(
                mouse_deselect_anchor,
                new_cursor_position,
                text_length
            );
            selection.move_caret_preserving_selection(
                new_cursor_position,
                text_length
            );
        }

        is_mouse_deselecting = false;
        return true;
    }

    if (
        event.type == UIEvent::event_mouse_move &&
        is_focused &&
        is_mouse_selecting &&
        event.left_button_down
    ) {
        selection.move_caret(
            get_cursor_position_from_event(event),
            (int)text.length(),
            true
        );
        return true;
    }

    if (event.type == UIEvent::event_mouse_up && is_mouse_selecting) {
        if (is_focused) {
            selection.move_caret(
                get_cursor_position_from_event(event),
                (int)text.length(),
                true
            );
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
                        selection.get_caret_position()
                    ),
                    event.shift_down
                );
                return true;

            case UIEvent::key_right:
                move_cursor(
                    TextNavigation::find_word_boundary_right(
                        text,
                        selection.get_caret_position()
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
                move_cursor(
                    selection.get_caret_position() - 1,
                    event.shift_down
                );
            }
            return true;

        case UIEvent::key_right:
            if (!event.shift_down && has_selection()) {
                move_cursor(get_selection_end(), false);
            } else {
                move_cursor(
                    selection.get_caret_position() + 1,
                    event.shift_down
                );
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

void Label::move_cursor(
    int new_cursor_position,
    bool extend_selection
) {
    selection.move_caret(
        new_cursor_position,
        (int)text.length(),
        extend_selection
    );
}

int Label::get_cursor_position_from_event(
    const UIEvent& event
) const {
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

    std::string compact_text = selection.get_compact_text(text);
    std::string preserved_text = selection.get_preserved_text(text);

    MimeData data;
    data.set_text(compact_text.c_str());
    data.set_data(
        MimeTypes::salix_selection_preserved(),
        preserved_text.c_str(),
        (int)preserved_text.length()
    );

    return clipboard->set_data(data);
}
