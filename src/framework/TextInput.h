// =================================================================================
// Filename:    framework/TextInput.h
// Author:      Ebdsaleh
// Description: Declares a backend-neutral single-line text input component.
// =================================================================================
#pragma once

#include <string>

#include "Component.h"
#include "TextSelection.h"

class Clipboard;
class MimeData;
class UIEvent;

class TextInput : public Component {
    public:
        enum PasteMode {
            paste_compact = 0,
            paste_keep_formatting
        };

        TextInput();

        void set_text(const char* new_text);
        const char* get_text() const;

        void set_max_length(int new_max_length);
        int get_max_length() const;

        void set_focused(bool new_is_focused);
        bool get_is_focused() const;

        void set_cursor_position(int new_cursor_position);
        int get_cursor_position() const;

        bool has_selection() const;
        int get_selection_start() const;
        int get_selection_end() const;
        int get_selection_range_count() const;
        bool get_selection_range(int index, int& start, int& end) const;
        void clear_selection();
        void select_all();

        bool accepts_mime_type(const char* mime_type) const;
        bool insert_mime_data(const MimeData& data);
        bool insert_mime_data(const MimeData& data, PasteMode paste_mode);

        int get_text_padding() const;

        virtual bool handle_event(const UIEvent& event);
        virtual void render(ComponentRenderer& renderer) const;

    private:
        void move_cursor(int new_cursor_position, bool extend_selection);
        void delete_selection();
        bool insert_plain_text(const char* new_text);
        int get_cursor_position_from_event(const UIEvent& event) const;

        bool copy_selection(Clipboard* clipboard) const;
        bool cut_selection(Clipboard* clipboard);
        bool paste_from_clipboard(
            Clipboard* clipboard,
            PasteMode paste_mode
        );

        std::string text;
        int max_length;
        bool is_focused;
        TextSelection selection;
        bool is_mouse_selecting;
        int text_padding;
};
