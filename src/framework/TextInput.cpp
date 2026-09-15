// =================================================================================
// Filename:    framework/TextInput.cpp
// Author:      Ebdsaleh
// Description: Implements a backend-neutral single-line text input component.
// =================================================================================

#include <string.h>
#include <vector>

#include "TextInput.h"
#include "UIEvent.h"
#include "Clipboard.h"
#include "MimeData.h"
#include "MimeTypes.h"
#include "TextMetrics.h"
#include "TextNavigation.h"
#include "rendering/ComponentRenderer.h"

TextInput::TextInput()
    : max_length(120),
      is_focused(false),
      is_mouse_selecting(false),
      is_mouse_deselecting(false),
      mouse_deselect_anchor(0),
      text_padding(6) {

    get_style().background_color = Color(255, 255, 255);
    get_style().border_color = Color(112, 112, 112);
}

void TextInput::set_text(const char* new_text) {
    if (new_text == 0) {
        text.clear();
        selection.reset(0, 0);
        return;
    }

    text = new_text;

    if ((int)text.length() > max_length) {
        text.erase(max_length);
    }

    selection.reset((int)text.length(), (int)text.length());
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

    selection.clamp_to_length((int)text.length());
}

int TextInput::get_max_length() const {
    return max_length;
}

void TextInput::set_focused(bool new_is_focused) {
    is_focused = new_is_focused;
    selection.clamp_to_length((int)text.length());

    if (!is_focused) {
        is_mouse_selecting = false;
        is_mouse_deselecting = false;
    }
}

bool TextInput::get_is_focused() const {
    return is_focused;
}

void TextInput::set_cursor_position(int new_cursor_position) {
    selection.reset(new_cursor_position, (int)text.length());
}

int TextInput::get_cursor_position() const {
    return selection.get_caret_position();
}

bool TextInput::has_selection() const {
    return selection.has_selection();
}

int TextInput::get_selection_start() const {
    return selection.get_first_selection_start();
}

int TextInput::get_selection_end() const {
    return selection.get_last_selection_end();
}

int TextInput::get_selection_range_count() const {
    return selection.get_range_count();
}

bool TextInput::get_selection_range(
    int index,
    int& start,
    int& end
) const {
    return selection.get_range(index, start, end);
}

void TextInput::clear_selection() {
    selection.clear_selection();
}

void TextInput::select_all() {
    selection.select_all((int)text.length());
}

bool TextInput::accepts_mime_type(const char* mime_type) const {
    if (mime_type == 0) {
        return false;
    }

    return strcmp(mime_type, MimeTypes::text_plain()) == 0 ||
        strcmp(mime_type, MimeTypes::salix_selection_preserved()) == 0;
}

bool TextInput::insert_mime_data(const MimeData& data) {
    return insert_mime_data(data, paste_compact);
}

bool TextInput::insert_mime_data(
    const MimeData& data,
    PasteMode paste_mode
) {
    const char* inserted_text = 0;

    if (
        paste_mode == paste_keep_formatting &&
        data.has_format(MimeTypes::salix_selection_preserved())
    ) {
        inserted_text = data.get_data(
            MimeTypes::salix_selection_preserved()
        );
    }

    if (inserted_text == 0 && data.has_format(MimeTypes::text_plain())) {
        inserted_text = data.get_text();
    }

    if (inserted_text == 0) {
        return false;
    }

    return insert_plain_text(inserted_text);
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

                case UIEvent::key_x:
                    cut_selection(event.clipboard);
                    return true;

                case UIEvent::key_v:
                    paste_from_clipboard(
                        event.clipboard,
                        event.shift_down
                            ? paste_keep_formatting
                            : paste_compact
                    );
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

            case UIEvent::key_delete:
                if (has_selection()) {
                    delete_selection();
                } else if (
                    selection.get_caret_position() < (int)text.length()
                ) {
                    text.erase(selection.get_caret_position(), 1);
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
        } else if (
            selection.get_caret_position() > 0 &&
            !text.empty()
        ) {
            int new_cursor_position = selection.get_caret_position() - 1;
            text.erase(new_cursor_position, 1);
            selection.reset(new_cursor_position, (int)text.length());
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

void TextInput::move_cursor(
    int new_cursor_position,
    bool extend_selection
) {
    selection.move_caret(
        new_cursor_position,
        (int)text.length(),
        extend_selection
    );
}

void TextInput::delete_selection() {
    if (!has_selection()) {
        return;
    }

    std::vector<TextRange> ranges;
    selection.get_normalized_ranges(ranges);

    if (ranges.empty()) {
        return;
    }

    int new_cursor_position = ranges[0].start;

    for (int index = (int)ranges.size() - 1; index >= 0; --index) {
        text.erase(
            (std::string::size_type)ranges[index].start,
            (std::string::size_type)(
                ranges[index].end - ranges[index].start
            )
        );
    }

    selection.reset(new_cursor_position, (int)text.length());
}

bool TextInput::insert_plain_text(const char* new_text) {
    if (new_text == 0) {
        return false;
    }

    if (selection.has_active_range()) {
        delete_selection();
    } else if (selection.has_persistent_ranges()) {
        // Ctrl+click can intentionally preserve earlier highlights while moving
        // the caret. Typing inserts at that caret rather than deleting picks.
        selection.clear_selection();
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

    int cursor_position = selection.get_caret_position();

    text.insert(
        (std::string::size_type)cursor_position,
        filtered_text
    );

    cursor_position += (int)filtered_text.length();
    selection.reset(cursor_position, (int)text.length());
    return true;
}

int TextInput::get_cursor_position_from_event(
    const UIEvent& event
) const {
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

bool TextInput::paste_from_clipboard(
    Clipboard* clipboard,
    PasteMode paste_mode
) {
    if (clipboard == 0) {
        return false;
    }

    MimeData data;
    if (!clipboard->get_data(data)) {
        return false;
    }

    return insert_mime_data(data, paste_mode);
}
