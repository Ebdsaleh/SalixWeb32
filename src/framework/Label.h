// =================================================================================
// Filename:    framework/Label.h
// Author:      Ebdsaleh
// Description: Declares a backend-neutral text label component.
// =================================================================================
#pragma once

#include <string>

#include "Component.h"
#include "TextSelection.h"

class Clipboard;
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
        const char* get_text() const;

        void set_horizontal_alignment(HorizontalAlignment new_alignment);
        HorizontalAlignment get_horizontal_alignment() const;

        void set_selectable(bool new_is_selectable);
        bool get_is_selectable() const;
        bool get_is_focused() const;

        int get_cursor_position() const;
        bool has_selection() const;
        int get_selection_start() const;
        int get_selection_end() const;
        int get_selection_range_count() const;
        bool get_selection_range(int index, int& start, int& end) const;
        void clear_selection();
        void select_all();

        virtual bool handle_event(const UIEvent& event);
        virtual void render(ComponentRenderer& renderer) const;

    private:
        void move_cursor(int new_cursor_position, bool extend_selection);
        int get_cursor_position_from_event(const UIEvent& event) const;
        bool copy_selection(Clipboard* clipboard) const;

        std::string text;
        HorizontalAlignment horizontal_alignment;
        bool is_selectable;
        bool is_focused;
        TextSelection selection;
        bool is_mouse_selecting;
};
