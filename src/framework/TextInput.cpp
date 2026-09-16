// =================================================================================
// Filename:    framework/TextInput.cpp
// Author:      Ebdsaleh
// Description: Implements a backend-neutral formatted text input.
// =================================================================================

#include <stdio.h>
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

namespace {
    const int control_character_y = 25;
    const int control_character_z = 26;

    bool contains_line_start(
        const std::vector<int>& line_starts,
        int value
    ) {
        for (int index = 0; index < (int)line_starts.size(); ++index) {
            if (line_starts[index] == value) {
                return true;
            }
        }

        return false;
    }

    void append_text_with_format(
        std::string& destination_text,
        std::vector<TextFormat>& destination_formats,
        const char* source_text,
        const TextFormat& format
    ) {
        if (source_text == 0) {
            return;
        }

        for (int index = 0; source_text[index] != '\0'; ++index) {
            destination_text += source_text[index];
            destination_formats.push_back(format);
        }
    }
}

TextInput::TextInput()
    : max_length(120),
      is_multiline(false),
      line_spacing(2),
      is_focused(false),
      is_mouse_selecting(false),
      is_mouse_deselecting(false),
      mouse_deselect_anchor(0),
      vertical_navigation_active(false),
      preferred_vertical_x(0),
      preferred_vertical_column(0),
      text_padding(6),
      active_edit_kind(edit_none),
      history_limit(100) {

    get_style().background_color = Color(255, 255, 255);
    get_style().border_color = Color(112, 112, 112);
}

void TextInput::set_text(const char* new_text) {
    reset_vertical_navigation_goal();
    clear_history();

    if (new_text == 0) {
        text.clear();
        character_formats.clear();
        selection.reset(0, 0);
        return;
    }

    text = new_text;

    if (!is_multiline) {
        for (int index = 0; index < (int)text.length(); ++index) {
            if (text[index] == '\r' || text[index] == '\n' || text[index] == '\t') {
                text[index] = ' ';
            }
        }
    } else {
        for (int index = 0; index < (int)text.length(); ++index) {
            if (text[index] == '\r') {
                text.erase(index, 1);
                --index;
            } else if (text[index] == '\t') {
                text[index] = ' ';
            }
        }
    }

    if ((int)text.length() > max_length) {
        text.erase(max_length);
    }

    character_formats.assign(text.length(), typing_format);
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

        if ((int)character_formats.size() > max_length) {
            character_formats.erase(
                character_formats.begin() + max_length,
                character_formats.end()
            );
        }

        clear_history();
        reset_vertical_navigation_goal();
    }

    ensure_format_length();
    selection.clamp_to_length((int)text.length());
}

int TextInput::get_max_length() const {
    return max_length;
}

void TextInput::set_multiline(bool new_is_multiline) {
    if (is_multiline == new_is_multiline) {
        return;
    }

    is_multiline = new_is_multiline;
    reset_vertical_navigation_goal();
    end_edit_group();

    if (!is_multiline) {
        for (int index = 0; index < (int)text.length(); ++index) {
            if (text[index] == '\r' || text[index] == '\n' || text[index] == '\t') {
                text[index] = ' ';
            }
        }
        clear_history();
    }

    selection.clamp_to_length((int)text.length());
}

bool TextInput::get_is_multiline() const {
    return is_multiline;
}

void TextInput::set_line_spacing(int new_line_spacing) {
    if (new_line_spacing < 0) {
        new_line_spacing = 0;
    }

    line_spacing = new_line_spacing;
}

int TextInput::get_line_spacing() const {
    return line_spacing;
}

void TextInput::set_focused(bool new_is_focused) {
    if (is_focused != new_is_focused) {
        reset_vertical_navigation_goal();
    }

    is_focused = new_is_focused;
    selection.clamp_to_length((int)text.length());

    if (!is_focused) {
        is_mouse_selecting = false;
        is_mouse_deselecting = false;
        end_edit_group();
    }
}

bool TextInput::get_is_focused() const {
    return is_focused;
}

void TextInput::set_cursor_position(int new_cursor_position) {
    reset_vertical_navigation_goal();
    end_edit_group();
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

bool TextInput::is_character_selected(int character_index) const {
    if (character_index < 0 || character_index >= (int)text.length()) {
        return false;
    }

    int range_count = selection.get_range_count();

    for (int index = 0; index < range_count; ++index) {
        int range_start = 0;
        int range_end = 0;

        if (!selection.get_range(index, range_start, range_end)) {
            continue;
        }

        if (
            character_index >= range_start &&
            character_index < range_end
        ) {
            return true;
        }
    }

    return false;
}

void TextInput::clear_selection() {
    reset_vertical_navigation_goal();
    end_edit_group();
    selection.clear_selection();
}

void TextInput::select_all() {
    reset_vertical_navigation_goal();
    end_edit_group();
    selection.select_all((int)text.length());
}

void TextInput::set_text_format(
    bool bold,
    bool italic,
    bool underline,
    int font_size
) {
    reset_vertical_navigation_goal();

    if (font_size < 1) {
        font_size = 1;
    }

    if (font_size > 96) {
        font_size = 96;
    }

    TextFormat new_format(bold, italic, underline, font_size);
    typing_format = new_format;

    if (!has_selection()) {
        end_edit_group();
        return;
    }

    begin_edit(edit_none);
    ensure_format_length();

    std::vector<TextRange> ranges;
    selection.get_normalized_ranges(ranges);

    for (int range_index = 0; range_index < (int)ranges.size(); ++range_index) {
        int start = ranges[range_index].start;
        int end = ranges[range_index].end;

        if (start < 0) {
            start = 0;
        }

        if (end > (int)character_formats.size()) {
            end = (int)character_formats.size();
        }

        for (int position = start; position < end; ++position) {
            if (text[position] != '\n') {
                character_formats[position] = new_format;
            }
        }
    }

    end_edit_group();
}

TextFormat TextInput::get_typing_format() const {
    return typing_format;
}

TextFormat TextInput::get_character_format(int index) const {
    if (index < 0 || index >= (int)character_formats.size()) {
        return typing_format;
    }

    return character_formats[index];
}

const TextFormat* TextInput::get_format_data() const {
    if (character_formats.empty()) {
        return 0;
    }

    return &character_formats[0];
}

int TextInput::get_format_count() const {
    return (int)character_formats.size();
}

bool TextInput::apply_list_style(ListStyle style) {
    if (!is_multiline) {
        return false;
    }

    std::vector<int> line_starts;
    collect_target_line_starts(line_starts);

    if (line_starts.empty()) {
        return false;
    }

    ListStyle effective_style = style;

    if (style != list_clear) {
        bool all_match = true;

        for (int index = 0; index < (int)line_starts.size(); ++index) {
            if (!line_matches_list_style(line_starts[index], style)) {
                all_match = false;
                break;
            }
        }

        if (all_match) {
            effective_style = list_clear;
        }
    }

    ensure_format_length();

    std::string new_text;
    std::vector<TextFormat> new_formats;
    int old_caret = selection.get_caret_position();
    int mapped_caret = -1;
    int line_start = 0;
    int list_number = 1;
    int text_length = (int)text.length();

    while (line_start <= text_length) {
        int line_end = get_line_end(line_start);
        bool is_target = contains_line_start(line_starts, line_start);
        int old_prefix_length = is_target
            ? get_list_prefix_length(line_start)
            : 0;

        std::string prefix;

        if (is_target && effective_style == list_bulleted) {
            prefix = "* ";
        } else if (is_target && effective_style == list_numbered) {
            char number_text[32];
            sprintf(number_text, "%d. ", list_number);
            prefix = number_text;
            ++list_number;
        }

        int new_line_start = (int)new_text.length();

        if (!prefix.empty()) {
            append_text_with_format(
                new_text,
                new_formats,
                prefix.c_str(),
                typing_format
            );
        }

        int copy_start = line_start + old_prefix_length;
        if (copy_start > line_end) {
            copy_start = line_end;
        }

        for (int position = copy_start; position < line_end; ++position) {
            new_text += text[position];
            new_formats.push_back(character_formats[position]);
        }

        if (old_caret >= line_start && old_caret <= line_end) {
            if (is_target && old_caret <= line_start + old_prefix_length) {
                mapped_caret = new_line_start + (int)prefix.length();
            } else {
                int source_body_start = is_target
                    ? line_start + old_prefix_length
                    : line_start;
                int relative_position = old_caret - source_body_start;
                if (relative_position < 0) {
                    relative_position = 0;
                }

                mapped_caret = new_line_start + (int)prefix.length() +
                    relative_position;
            }
        }

        if (line_end < text_length) {
            new_text += '\n';

            if (line_end < (int)character_formats.size()) {
                new_formats.push_back(character_formats[line_end]);
            } else {
                new_formats.push_back(typing_format);
            }

            line_start = line_end + 1;
        } else {
            break;
        }
    }

    if ((int)new_text.length() > max_length) {
        return false;
    }

    begin_edit(edit_none);
    text = new_text;
    character_formats = new_formats;
    ensure_format_length();

    if (mapped_caret < 0) {
        mapped_caret = (int)text.length();
    }

    selection.reset(mapped_caret, (int)text.length());
    end_edit_group();
    return true;
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

    return insert_plain_text(inserted_text, edit_none);
}

bool TextInput::can_undo() const {
    return !undo_history.empty();
}

bool TextInput::can_redo() const {
    return !redo_history.empty();
}

bool TextInput::undo() {
    end_edit_group();

    if (undo_history.empty()) {
        return false;
    }

    redo_history.push_back(capture_edit_state());
    trim_history(redo_history);

    EditState state = undo_history[undo_history.size() - 1];
    undo_history.pop_back();
    restore_edit_state(state);
    return true;
}

bool TextInput::redo() {
    end_edit_group();

    if (redo_history.empty()) {
        return false;
    }

    undo_history.push_back(capture_edit_state());
    trim_history(undo_history);

    EditState state = redo_history[redo_history.size() - 1];
    redo_history.pop_back();
    restore_edit_state(state);
    return true;
}

int TextInput::get_text_padding() const {
    return text_padding;
}

bool TextInput::handle_event(const UIEvent& event) {
    if (!get_is_visible()) {
        return false;
    }

    if (event.type == UIEvent::event_mouse_down) {
        reset_vertical_navigation_goal();
        end_edit_group();

        bool new_is_focused = contains_point(event.x, event.y);
        bool did_change = new_is_focused != is_focused;

        if (new_is_focused) {
            int new_cursor_position = get_cursor_position_from_event(event);
            int text_length = (int)text.length();

            if (event.control_down && event.alt_down) {
                selection.preserve_active_range(text_length);

                if (event.click_count >= 3) {
                    selection.remove_range(
                        TextNavigation::find_line_start(text, new_cursor_position),
                        TextNavigation::find_line_end(text, new_cursor_position),
                        text_length
                    );
                    selection.move_caret_preserving_selection(
                        new_cursor_position,
                        text_length
                    );
                    is_mouse_deselecting = false;
                } else if (event.click_count == 2) {
                    selection.remove_range(
                        TextNavigation::find_word_start(text, new_cursor_position),
                        TextNavigation::find_word_end(text, new_cursor_position),
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
                        TextNavigation::find_line_start(text, new_cursor_position),
                        TextNavigation::find_line_end(text, new_cursor_position),
                        text_length,
                        true
                    );
                } else if (event.click_count == 2) {
                    selection.select_range(
                        TextNavigation::find_word_start(text, new_cursor_position),
                        TextNavigation::find_word_end(text, new_cursor_position),
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
                    TextNavigation::find_line_start(text, new_cursor_position),
                    TextNavigation::find_line_end(text, new_cursor_position),
                    text_length,
                    false
                );
                is_mouse_selecting = false;
                is_mouse_deselecting = false;
            } else if (event.click_count == 2) {
                selection.select_range(
                    TextNavigation::find_word_start(text, new_cursor_position),
                    TextNavigation::find_word_end(text, new_cursor_position),
                    text_length,
                    false
                );
                is_mouse_selecting = false;
                is_mouse_deselecting = false;
            } else if (event.shift_down && is_focused) {
                selection.move_caret(new_cursor_position, text_length, true);
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

    if (event.type == UIEvent::event_mouse_up && is_mouse_deselecting) {
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
                    end_edit_group();
                    move_cursor(
                        TextNavigation::find_word_boundary_left(
                            text,
                            selection.get_caret_position()
                        ),
                        event.shift_down
                    );
                    return true;

                case UIEvent::key_right:
                    end_edit_group();
                    move_cursor(
                        TextNavigation::find_word_boundary_right(
                            text,
                            selection.get_caret_position()
                        ),
                        event.shift_down
                    );
                    return true;

                case UIEvent::key_home:
                    end_edit_group();
                    move_cursor(0, event.shift_down);
                    return true;

                case UIEvent::key_end:
                    end_edit_group();
                    move_cursor((int)text.length(), event.shift_down);
                    return true;

                case UIEvent::key_a:
                    select_all();
                    return true;

                case UIEvent::key_c:
                    end_edit_group();
                    copy_selection(event.clipboard);
                    return true;

                case UIEvent::key_x:
                    cut_selection(
                        event.clipboard,
                        event.shift_down
                            ? cut_keep_formatting
                            : cut_compact
                    );
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
                end_edit_group();
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
                end_edit_group();
                if (!event.shift_down && has_selection()) {
                    move_cursor(get_selection_end(), false);
                } else {
                    move_cursor(
                        selection.get_caret_position() + 1,
                        event.shift_down
                    );
                }
                return true;

            case UIEvent::key_up:
                if (is_multiline) {
                    end_edit_group();
                    move_cursor_vertical(
                        -1,
                        event.shift_down,
                        event.text_metrics
                    );
                    return true;
                }
                break;

            case UIEvent::key_down:
                if (is_multiline) {
                    end_edit_group();
                    move_cursor_vertical(
                        1,
                        event.shift_down,
                        event.text_metrics
                    );
                    return true;
                }
                break;

            case UIEvent::key_home:
                end_edit_group();
                move_cursor(
                    is_multiline
                        ? get_line_start(selection.get_caret_position())
                        : 0,
                    event.shift_down
                );
                return true;

            case UIEvent::key_end:
                end_edit_group();
                move_cursor(
                    is_multiline
                        ? get_line_end(selection.get_caret_position())
                        : (int)text.length(),
                    event.shift_down
                );
                return true;

            case UIEvent::key_delete:
                if (has_selection()) {
                    begin_edit(edit_none);
                    delete_selection();
                } else if (
                    selection.get_caret_position() < (int)text.length()
                ) {
                    begin_edit(edit_delete);
                    int position = selection.get_caret_position();
                    text.erase(position, 1);

                    if (position < (int)character_formats.size()) {
                        character_formats.erase(
                            character_formats.begin() + position
                        );
                    }

                    selection.clamp_to_length((int)text.length());
                    ensure_format_length();
                }
                return true;

            default:
                break;
        }
    }

    if (event.type != UIEvent::event_character) {
        return false;
    }

    if (event.control_down && event.character_code == control_character_z) {
        if (event.shift_down) {
            redo();
        } else {
            undo();
        }
        return true;
    }

    if (event.control_down && event.character_code == control_character_y) {
        redo();
        return true;
    }

    if (event.character_code == 8) {
        if (has_selection()) {
            begin_edit(edit_none);
            delete_selection();
        } else if (
            selection.get_caret_position() > 0 &&
            !text.empty()
        ) {
            begin_edit(edit_backspace);
            int new_cursor_position = selection.get_caret_position() - 1;
            text.erase(new_cursor_position, 1);

            if (new_cursor_position < (int)character_formats.size()) {
                character_formats.erase(
                    character_formats.begin() + new_cursor_position
                );
            }

            selection.reset(new_cursor_position, (int)text.length());
            ensure_format_length();
        }
        return true;
    }

    if (event.character_code == 13) {
        if (is_multiline) {
            insert_plain_text("\n", edit_typing);
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
        insert_plain_text(character_text, edit_typing);
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
    reset_vertical_navigation_goal();
    selection.move_caret(
        new_cursor_position,
        (int)text.length(),
        extend_selection
    );
}

void TextInput::move_cursor_vertical(
    int direction,
    bool extend_selection,
    TextMetrics* text_metrics
) {
    int caret = selection.get_caret_position();
    int current_start = get_line_start(caret);
    int current_end = get_line_end(caret);
    int current_column = caret - current_start;

    if (!vertical_navigation_active) {
        preferred_vertical_column = current_column;
        preferred_vertical_x = current_column;

        if (text_metrics != 0) {
            const TextFormat* formats = get_format_data();
            preferred_vertical_x = text_metrics->measure_formatted_text_width(
                text.c_str() + current_start,
                current_column,
                formats == 0 ? 0 : formats + current_start,
                current_column
            );
        }

        vertical_navigation_active = true;
    }

    int target_start = current_start;
    int target_end = current_end;

    if (direction < 0) {
        if (current_start <= 0) {
            selection.move_caret(0, (int)text.length(), extend_selection);
            return;
        }

        target_end = current_start - 1;
        target_start = get_line_start(target_end);
    } else if (direction > 0) {
        if (current_end >= (int)text.length()) {
            selection.move_caret(
                (int)text.length(),
                (int)text.length(),
                extend_selection
            );
            return;
        }

        target_start = current_end + 1;
        target_end = get_line_end(target_start);
    } else {
        return;
    }

    int target_length = target_end - target_start;
    int target_column = preferred_vertical_column;

    if (text_metrics != 0) {
        const TextFormat* formats = get_format_data();
        target_column = text_metrics->get_formatted_character_index_at_x(
            text.c_str() + target_start,
            target_length,
            formats == 0 ? 0 : formats + target_start,
            target_length,
            preferred_vertical_x
        );
    }

    if (target_column < 0) {
        target_column = 0;
    }
    if (target_column > target_length) {
        target_column = target_length;
    }

    selection.move_caret(
        target_start + target_column,
        (int)text.length(),
        extend_selection
    );
}

void TextInput::reset_vertical_navigation_goal() {
    vertical_navigation_active = false;
    preferred_vertical_x = 0;
    preferred_vertical_column = 0;
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

    ensure_format_length();
    int new_cursor_position = ranges[0].start;

    for (int index = (int)ranges.size() - 1; index >= 0; --index) {
        int start = ranges[index].start;
        int count = ranges[index].end - ranges[index].start;

        text.erase(
            (std::string::size_type)start,
            (std::string::size_type)count
        );

        if (
            start >= 0 &&
            start < (int)character_formats.size() &&
            count > 0
        ) {
            int end = start + count;
            if (end > (int)character_formats.size()) {
                end = (int)character_formats.size();
            }

            character_formats.erase(
                character_formats.begin() + start,
                character_formats.begin() + end
            );
        }
    }

    ensure_format_length();
    selection.reset(new_cursor_position, (int)text.length());
}

void TextInput::blank_selection_with_spaces() {
    if (!has_selection()) {
        return;
    }

    std::vector<TextRange> ranges;
    selection.get_normalized_ranges(ranges);

    if (ranges.empty()) {
        return;
    }

    int new_cursor_position = ranges[0].start;

    for (int range_index = 0; range_index < (int)ranges.size(); ++range_index) {
        int start = ranges[range_index].start;
        int end = ranges[range_index].end;

        if (start < 0) {
            start = 0;
        }

        if (end > (int)text.length()) {
            end = (int)text.length();
        }

        for (int position = start; position < end; ++position) {
            if (text[position] != '\n') {
                text[position] = ' ';
            }
        }
    }

    selection.reset(new_cursor_position, (int)text.length());
}

bool TextInput::insert_plain_text(
    const char* new_text,
    EditKind edit_kind
) {
    if (new_text == 0) {
        return false;
    }

    std::string filtered_text;

    for (int index = 0; new_text[index] != '\0'; ++index) {
        char character = new_text[index];

        if (character == '\r') {
            continue;
        }

        if (character == '\n') {
            if (!is_multiline) {
                character = ' ';
            }
        } else if (character == '\t') {
            character = ' ';
        }

        filtered_text += character;

        if ((int)filtered_text.length() >= max_length) {
            break;
        }
    }

    if (filtered_text.empty()) {
        return true;
    }

    bool will_delete_selection = selection.has_active_range();

    if (!will_delete_selection && (int)text.length() >= max_length) {
        return true;
    }

    begin_edit(edit_kind);

    if (selection.has_active_range()) {
        delete_selection();
    } else if (selection.has_persistent_ranges()) {
        selection.clear_selection();
    }

    int remaining_capacity = max_length - (int)text.length();
    if (remaining_capacity <= 0) {
        return true;
    }

    if ((int)filtered_text.length() > remaining_capacity) {
        filtered_text.erase(remaining_capacity);
    }

    ensure_format_length();
    int cursor_position = selection.get_caret_position();

    text.insert(
        (std::string::size_type)cursor_position,
        filtered_text
    );

    character_formats.insert(
        character_formats.begin() + cursor_position,
        filtered_text.length(),
        typing_format
    );

    cursor_position += (int)filtered_text.length();
    selection.reset(cursor_position, (int)text.length());
    ensure_format_length();
    return true;
}

int TextInput::get_cursor_position_from_event(
    const UIEvent& event
) const {
    if (event.text_metrics == 0) {
        return (int)text.length();
    }

    int relative_x = event.x - get_x() - text_padding;

    if (!is_multiline) {
        return event.text_metrics->get_formatted_character_index_at_x(
            text.c_str(),
            (int)text.length(),
            get_format_data(),
            get_format_count(),
            relative_x
        );
    }

    int relative_y = event.y - get_y() - text_padding;

    return event.text_metrics->get_formatted_character_index_at_point(
        text.c_str(),
        (int)text.length(),
        get_format_data(),
        get_format_count(),
        relative_x,
        relative_y,
        line_spacing
    );
}

void TextInput::ensure_format_length() {
    if (character_formats.size() < text.length()) {
        character_formats.resize(text.length(), typing_format);
    } else if (character_formats.size() > text.length()) {
        character_formats.erase(
            character_formats.begin() + text.length(),
            character_formats.end()
        );
    }
}

int TextInput::get_line_start(int position) const {
    if (position < 0) {
        position = 0;
    }

    if (position > (int)text.length()) {
        position = (int)text.length();
    }

    return TextNavigation::find_line_start(text, position);
}

int TextInput::get_line_end(int position) const {
    if (position < 0) {
        position = 0;
    }

    if (position > (int)text.length()) {
        position = (int)text.length();
    }

    return TextNavigation::find_line_end(text, position);
}

int TextInput::get_list_prefix_length(int line_start) const {
    int text_length = (int)text.length();

    if (line_start < 0 || line_start >= text_length) {
        return 0;
    }

    if (
        line_start + 1 < text_length &&
        (text[line_start] == '*' || text[line_start] == '-') &&
        text[line_start + 1] == ' '
    ) {
        return 2;
    }

    int position = line_start;
    bool found_digit = false;

    while (
        position < text_length &&
        text[position] >= '0' &&
        text[position] <= '9'
    ) {
        found_digit = true;
        ++position;
    }

    if (
        found_digit &&
        position + 1 < text_length &&
        text[position] == '.' &&
        text[position + 1] == ' '
    ) {
        return position + 2 - line_start;
    }

    return 0;
}

bool TextInput::line_matches_list_style(
    int line_start,
    ListStyle style
) const {
    if (style == list_clear) {
        return get_list_prefix_length(line_start) == 0;
    }

    if (line_start < 0 || line_start >= (int)text.length()) {
        return false;
    }

    if (style == list_bulleted) {
        return line_start + 1 < (int)text.length() &&
            (text[line_start] == '*' || text[line_start] == '-') &&
            text[line_start + 1] == ' ';
    }

    if (style == list_numbered) {
        int position = line_start;
        bool found_digit = false;

        while (
            position < (int)text.length() &&
            text[position] >= '0' &&
            text[position] <= '9'
        ) {
            found_digit = true;
            ++position;
        }

        return found_digit &&
            position + 1 < (int)text.length() &&
            text[position] == '.' &&
            text[position + 1] == ' ';
    }

    return false;
}

void TextInput::collect_target_line_starts(
    std::vector<int>& line_starts
) const {
    line_starts.clear();

    if (!has_selection()) {
        line_starts.push_back(get_line_start(selection.get_caret_position()));
        return;
    }

    std::vector<TextRange> ranges;
    selection.get_normalized_ranges(ranges);

    for (int range_index = 0; range_index < (int)ranges.size(); ++range_index) {
        int range_start = ranges[range_index].start;
        int range_end = ranges[range_index].end;

        if (range_start < 0) {
            range_start = 0;
        }

        if (range_end > (int)text.length()) {
            range_end = (int)text.length();
        }

        int last_position = range_end > range_start
            ? range_end - 1
            : range_start;
        int current_start = get_line_start(range_start);

        while (current_start <= last_position) {
            if (!contains_line_start(line_starts, current_start)) {
                line_starts.push_back(current_start);
            }

            int current_end = get_line_end(current_start);
            if (current_end >= (int)text.length()) {
                break;
            }

            current_start = current_end + 1;
        }
    }
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

bool TextInput::cut_selection(
    Clipboard* clipboard,
    CutMode cut_mode
) {
    if (!has_selection()) {
        return false;
    }

    if (!copy_selection(clipboard)) {
        return false;
    }

    begin_edit(edit_none);

    if (cut_mode == cut_keep_formatting) {
        blank_selection_with_spaces();
    } else {
        delete_selection();
    }

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

void TextInput::begin_edit(EditKind new_edit_kind) {
    reset_vertical_navigation_goal();

    if (
        new_edit_kind == edit_none ||
        active_edit_kind != new_edit_kind
    ) {
        push_undo_state();
    }

    active_edit_kind = new_edit_kind;
}

void TextInput::end_edit_group() {
    active_edit_kind = edit_none;
}

TextInput::EditState TextInput::capture_edit_state() const {
    EditState state;
    state.text = text;
    state.selection = selection;
    state.character_formats = character_formats;
    return state;
}

void TextInput::restore_edit_state(const EditState& state) {
    text = state.text;
    selection = state.selection;
    character_formats = state.character_formats;
    ensure_format_length();
    selection.clamp_to_length((int)text.length());
    is_mouse_selecting = false;
    is_mouse_deselecting = false;
    reset_vertical_navigation_goal();
}

void TextInput::push_undo_state() {
    undo_history.push_back(capture_edit_state());
    trim_history(undo_history);
    redo_history.clear();
}

void TextInput::trim_history(std::vector<EditState>& history) {
    while ((int)history.size() > history_limit) {
        history.erase(history.begin());
    }
}

void TextInput::clear_history() {
    undo_history.clear();
    redo_history.clear();
    active_edit_kind = edit_none;
}
