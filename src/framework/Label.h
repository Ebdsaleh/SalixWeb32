// =================================================================================
// Filename:    framework/Label.h
// Author:      Ebdsaleh
// Description: Declares a backend-neutral selectable formatted text label.
// =================================================================================
#pragma once

#include <string>
#include <vector>

#include "Component.h"
#include "TextFormat.h"
#include "TextSelection.h"

class Clipboard;
class FormattedText;
class UIEvent;

class Label : public Component {
    public:
        enum HorizontalAlignment {
            align_left = 0,
            align_center,
            align_right
        };

        Label();

        void set_text(const char* new_text);
        void set_formatted_text(const FormattedText& formatted_text);
        const char* get_text() const;

        TextFormat get_character_format(int index) const;
        const TextFormat* get_format_data() const;
        int get_format_count() const;
        int get_max_font_size() const;

        void set_horizontal_alignment(HorizontalAlignment new_alignment);
        HorizontalAlignment get_horizontal_alignment() const;

        void set_word_wrap(bool new_word_wrap);
        bool get_word_wrap() const;

        void set_selectable(bool new_is_selectable);
        bool get_is_selectable() const;
        bool get_is_focused() const;
        void set_focused(bool new_is_focused) {
            is_focused = new_is_focused;
            if (!is_focused) {
                is_mouse_selecting = false;
                is_mouse_deselecting = false;
            }
        }

        int get_cursor_position() const;
        int get_character_index_at_event(const UIEvent& event) const;
        bool has_selection() const;
        int get_selection_start() const;
        int get_selection_end() const;
        int get_selection_range_count() const;
        bool get_selection_range(int index, int& start, int& end) const;
        bool is_character_selected(int character_index) const;
        void clear_selection();
        void set_selection_range(int start, int end);
        void select_all();

        virtual bool handle_event(const UIEvent& event);
        virtual void render(ComponentRenderer& renderer) const;

    private:
        void move_cursor(int new_cursor_position, bool extend_selection);
        int get_cursor_position_from_event(const UIEvent& event) const;
        bool copy_selection(Clipboard* clipboard) const;
        void ensure_format_length();

        std::string text;
        std::vector<TextFormat> character_formats;
        HorizontalAlignment horizontal_alignment;
        bool word_wrap;
        bool is_selectable;
        bool is_focused;
        TextSelection selection;
        bool is_mouse_selecting;
        bool is_mouse_deselecting;
        int mouse_deselect_anchor;
};
