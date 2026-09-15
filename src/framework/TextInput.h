// =================================================================================
// Filename:    framework/TextInput.h
// Author:      Ebdsaleh
// Description: Declares a backend-neutral single-line text input component.
// =================================================================================
#pragma once

#include <string>
#include <vector>

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

        enum CutMode {
            cut_compact = 0,
            cut_keep_formatting
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

        bool can_undo() const;
        bool can_redo() const;
        bool undo();
        bool redo();

        int get_text_padding() const;

        virtual bool handle_event(const UIEvent& event);
        virtual void render(ComponentRenderer& renderer) const;

    private:
        enum EditKind {
            edit_none = 0,
            edit_typing,
            edit_backspace,
            edit_delete
        };

        struct EditState {
            std::string text;
            TextSelection selection;
        };

        void move_cursor(int new_cursor_position, bool extend_selection);
        void delete_selection();
        void blank_selection_with_spaces();
        bool insert_plain_text(const char* new_text, EditKind edit_kind);
        int get_cursor_position_from_event(const UIEvent& event) const;

        bool copy_selection(Clipboard* clipboard) const;
        bool cut_selection(Clipboard* clipboard, CutMode cut_mode);
        bool paste_from_clipboard(
            Clipboard* clipboard,
            PasteMode paste_mode
        );

        void begin_edit(EditKind new_edit_kind);
        void end_edit_group();
        EditState capture_edit_state() const;
        void restore_edit_state(const EditState& state);
        void push_undo_state();
        void trim_history(std::vector<EditState>& history);
        void clear_history();

        std::string text;
        int max_length;
        bool is_focused;
        TextSelection selection;
        bool is_mouse_selecting;
        bool is_mouse_deselecting;
        int mouse_deselect_anchor;
        int text_padding;

        std::vector<EditState> undo_history;
        std::vector<EditState> redo_history;
        EditKind active_edit_kind;
        int history_limit;
};
