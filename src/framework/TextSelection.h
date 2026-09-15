// =================================================================================
// Filename:    framework/TextSelection.h
// Author:      Ebdsaleh
// Description: Declares backend-neutral caret and discontinuous text selection state.
// =================================================================================
#pragma once

#include <string>
#include <vector>

struct TextRange {
    TextRange();
    TextRange(int new_start, int new_end);

    int start;
    int end;
};

class TextSelection {
    public:
        TextSelection();

        void reset(int new_caret_position, int text_length);
        void clamp_to_length(int text_length);

        int get_caret_position() const;
        int get_anchor_position() const;

        bool has_active_range() const;
        bool has_persistent_ranges() const;
        bool has_selection() const;

        void clear_selection();
        void preserve_active_range(int text_length);

        void move_caret(
            int new_caret_position,
            int text_length,
            bool extend_selection
        );

        void move_caret_preserving_selection(
            int new_caret_position,
            int text_length
        );

        void select_range(
            int start,
            int end,
            int text_length,
            bool additive
        );

        void remove_range(
            int start,
            int end,
            int text_length
        );

        void select_all(int text_length);

        int get_range_count() const;
        bool get_range(int index, int& start, int& end) const;
        void get_normalized_ranges(std::vector<TextRange>& ranges) const;

        int get_first_selection_start() const;
        int get_last_selection_end() const;

        std::string get_compact_text(const std::string& text) const;
        std::string get_preserved_text(const std::string& text) const;

    private:
        void add_persistent_range(
            int start,
            int end,
            int text_length
        );

        int caret_position;
        int anchor_position;
        std::vector<TextRange> persistent_ranges;
};
