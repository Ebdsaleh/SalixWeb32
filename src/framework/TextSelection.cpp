// =================================================================================
// Filename:    framework/TextSelection.cpp
// Author:      Ebdsaleh
// Description: Implements backend-neutral caret and discontinuous text selection state.
// =================================================================================

#include <algorithm>

#include "TextSelection.h"

namespace {
    int clamp_position(int position, int text_length) {
        if (position < 0) {
            return 0;
        }

        if (position > text_length) {
            return text_length;
        }

        return position;
    }

    bool range_less(const TextRange& left, const TextRange& right) {
        if (left.start != right.start) {
            return left.start < right.start;
        }

        return left.end < right.end;
    }

    void normalize_ranges(
        std::vector<TextRange>& ranges,
        int text_length
    ) {
        std::vector<TextRange> cleaned_ranges;

        for (int index = 0; index < (int)ranges.size(); ++index) {
            int start = clamp_position(ranges[index].start, text_length);
            int end = clamp_position(ranges[index].end, text_length);

            if (end < start) {
                int temporary = start;
                start = end;
                end = temporary;
            }

            if (end > start) {
                cleaned_ranges.push_back(TextRange(start, end));
            }
        }

        std::sort(
            cleaned_ranges.begin(),
            cleaned_ranges.end(),
            range_less
        );

        ranges.clear();

        for (int index = 0; index < (int)cleaned_ranges.size(); ++index) {
            const TextRange& current_range = cleaned_ranges[index];

            if (ranges.empty()) {
                ranges.push_back(current_range);
                continue;
            }

            TextRange& previous_range = ranges[ranges.size() - 1];

            if (current_range.start <= previous_range.end) {
                if (current_range.end > previous_range.end) {
                    previous_range.end = current_range.end;
                }
            } else {
                ranges.push_back(current_range);
            }
        }
    }

    bool is_preserved_gap_character(char character) {
        return character == '\r' ||
            character == '\n' ||
            character == '\t';
    }
}

TextRange::TextRange()
    : start(0),
      end(0) {
}

TextRange::TextRange(int new_start, int new_end)
    : start(new_start),
      end(new_end) {
}

TextSelection::TextSelection()
    : caret_position(0),
      anchor_position(0) {
}

void TextSelection::reset(
    int new_caret_position,
    int text_length
) {
    caret_position = clamp_position(new_caret_position, text_length);
    anchor_position = caret_position;
    persistent_ranges.clear();
}

void TextSelection::clamp_to_length(int text_length) {
    caret_position = clamp_position(caret_position, text_length);
    anchor_position = clamp_position(anchor_position, text_length);
    normalize_ranges(persistent_ranges, text_length);
}

int TextSelection::get_caret_position() const {
    return caret_position;
}

int TextSelection::get_anchor_position() const {
    return anchor_position;
}

bool TextSelection::has_active_range() const {
    return caret_position != anchor_position;
}

bool TextSelection::has_persistent_ranges() const {
    return !persistent_ranges.empty();
}

bool TextSelection::has_selection() const {
    return has_active_range() || has_persistent_ranges();
}

void TextSelection::clear_selection() {
    persistent_ranges.clear();
    anchor_position = caret_position;
}

void TextSelection::preserve_active_range(int text_length) {
    if (!has_active_range()) {
        return;
    }

    add_persistent_range(
        anchor_position,
        caret_position,
        text_length
    );

    anchor_position = caret_position;
}

void TextSelection::move_caret(
    int new_caret_position,
    int text_length,
    bool extend_selection
) {
    caret_position = clamp_position(new_caret_position, text_length);

    if (!extend_selection) {
        persistent_ranges.clear();
        anchor_position = caret_position;
    }
}

void TextSelection::move_caret_preserving_selection(
    int new_caret_position,
    int text_length
) {
    preserve_active_range(text_length);
    caret_position = clamp_position(new_caret_position, text_length);
    anchor_position = caret_position;
}

void TextSelection::select_range(
    int start,
    int end,
    int text_length,
    bool additive
) {
    start = clamp_position(start, text_length);
    end = clamp_position(end, text_length);

    if (end < start) {
        int temporary = start;
        start = end;
        end = temporary;
    }

    if (additive) {
        preserve_active_range(text_length);
        add_persistent_range(start, end, text_length);
        caret_position = end;
        anchor_position = caret_position;
        return;
    }

    persistent_ranges.clear();
    anchor_position = start;
    caret_position = end;
}

void TextSelection::select_all(int text_length) {
    select_range(0, text_length, text_length, false);
}

int TextSelection::get_range_count() const {
    std::vector<TextRange> ranges;
    get_normalized_ranges(ranges);
    return (int)ranges.size();
}

bool TextSelection::get_range(
    int index,
    int& start,
    int& end
) const {
    std::vector<TextRange> ranges;
    get_normalized_ranges(ranges);

    if (index < 0 || index >= (int)ranges.size()) {
        return false;
    }

    start = ranges[index].start;
    end = ranges[index].end;
    return true;
}

void TextSelection::get_normalized_ranges(
    std::vector<TextRange>& ranges
) const {
    ranges = persistent_ranges;

    if (has_active_range()) {
        int start = anchor_position;
        int end = caret_position;

        if (end < start) {
            int temporary = start;
            start = end;
            end = temporary;
        }

        ranges.push_back(TextRange(start, end));
    }

    int text_length = caret_position;

    if (anchor_position > text_length) {
        text_length = anchor_position;
    }

    for (int index = 0; index < (int)persistent_ranges.size(); ++index) {
        if (persistent_ranges[index].end > text_length) {
            text_length = persistent_ranges[index].end;
        }
    }

    normalize_ranges(ranges, text_length);
}

int TextSelection::get_first_selection_start() const {
    std::vector<TextRange> ranges;
    get_normalized_ranges(ranges);

    if (ranges.empty()) {
        return caret_position;
    }

    return ranges[0].start;
}

int TextSelection::get_last_selection_end() const {
    std::vector<TextRange> ranges;
    get_normalized_ranges(ranges);

    if (ranges.empty()) {
        return caret_position;
    }

    return ranges[ranges.size() - 1].end;
}

std::string TextSelection::get_compact_text(
    const std::string& text
) const {
    std::vector<TextRange> ranges;
    get_normalized_ranges(ranges);

    std::string result;

    for (int index = 0; index < (int)ranges.size(); ++index) {
        int start = clamp_position(ranges[index].start, (int)text.length());
        int end = clamp_position(ranges[index].end, (int)text.length());

        if (end <= start) {
            continue;
        }

        if (!result.empty()) {
            result += ' ';
        }

        result.append(
            text,
            (std::string::size_type)start,
            (std::string::size_type)(end - start)
        );
    }

    return result;
}

std::string TextSelection::get_preserved_text(
    const std::string& text
) const {
    std::vector<TextRange> ranges;
    get_normalized_ranges(ranges);

    if (ranges.empty()) {
        return std::string();
    }

    int first_position = clamp_position(
        ranges[0].start,
        (int)text.length()
    );

    int last_position = clamp_position(
        ranges[ranges.size() - 1].end,
        (int)text.length()
    );

    std::string result;
    int range_index = 0;

    for (int position = first_position; position < last_position; ++position) {
        while (
            range_index < (int)ranges.size() &&
            position >= ranges[range_index].end
        ) {
            ++range_index;
        }

        bool is_selected =
            range_index < (int)ranges.size() &&
            position >= ranges[range_index].start &&
            position < ranges[range_index].end;

        char character = text[position];

        if (is_selected || is_preserved_gap_character(character)) {
            result += character;
        } else {
            result += ' ';
        }
    }

    return result;
}

void TextSelection::add_persistent_range(
    int start,
    int end,
    int text_length
) {
    start = clamp_position(start, text_length);
    end = clamp_position(end, text_length);

    if (end < start) {
        int temporary = start;
        start = end;
        end = temporary;
    }

    if (end <= start) {
        return;
    }

    persistent_ranges.push_back(TextRange(start, end));
    normalize_ranges(persistent_ranges, text_length);
}
