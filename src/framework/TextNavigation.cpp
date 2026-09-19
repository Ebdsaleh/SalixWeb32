// =================================================================================
// Filename:    framework/TextNavigation.cpp
// Author:      Ebdsaleh
// Description: Implements backend-neutral word and line cursor/selection helpers.
// =================================================================================

#include "TextNavigation.h"
#include "Utf8Text.h"

namespace {
    int clamp_position(const std::string& text, int position) {
        int text_length = (int)text.length();

        if (position < 0) {
            return 0;
        }

        if (position > text_length) {
            position = text_length;
        }

        return Utf8Text::clamp_to_boundary(
            text.c_str(),
            text_length,
            position
        );
    }

    bool is_whitespace_at(
        const std::string& text,
        int position
    ) {
        return Utf8Text::is_ascii_whitespace_at(
            text.c_str(),
            (int)text.length(),
            position
        );
    }

    int find_nearest_word_character(
        const std::string& text,
        int position
    ) {
        int text_length = (int)text.length();
        if (text_length <= 0) {
            return -1;
        }

        int cursor = clamp_position(text, position);

        if (
            cursor < text_length &&
            !is_whitespace_at(text, cursor)
        ) {
            return cursor;
        }

        if (cursor > 0) {
            int previous = Utf8Text::previous_index(
                text.c_str(),
                text_length,
                cursor
            );

            if (!is_whitespace_at(text, previous)) {
                return previous;
            }
        }

        int right_cursor = cursor;
        while (
            right_cursor < text_length &&
            is_whitespace_at(text, right_cursor)
        ) {
            right_cursor = Utf8Text::next_index(
                text.c_str(),
                text_length,
                right_cursor
            );
        }

        if (right_cursor < text_length) {
            return right_cursor;
        }

        int left_cursor = cursor;
        while (left_cursor > 0) {
            int previous = Utf8Text::previous_index(
                text.c_str(),
                text_length,
                left_cursor
            );

            if (!is_whitespace_at(text, previous)) {
                return previous;
            }

            left_cursor = previous;
        }

        return -1;
    }
}

int TextNavigation::find_word_boundary_left(
    const std::string& text,
    int position
) {
    int cursor = clamp_position(text, position);
    int text_length = (int)text.length();

    while (cursor > 0) {
        int previous = Utf8Text::previous_index(
            text.c_str(),
            text_length,
            cursor
        );

        if (!is_whitespace_at(text, previous)) {
            break;
        }

        cursor = previous;
    }

    while (cursor > 0) {
        int previous = Utf8Text::previous_index(
            text.c_str(),
            text_length,
            cursor
        );

        if (is_whitespace_at(text, previous)) {
            break;
        }

        cursor = previous;
    }

    return cursor;
}

int TextNavigation::find_word_boundary_right(
    const std::string& text,
    int position
) {
    int cursor = clamp_position(text, position);
    int text_length = (int)text.length();

    while (
        cursor < text_length &&
        is_whitespace_at(text, cursor)
    ) {
        cursor = Utf8Text::next_index(
            text.c_str(),
            text_length,
            cursor
        );
    }

    while (
        cursor < text_length &&
        !is_whitespace_at(text, cursor)
    ) {
        cursor = Utf8Text::next_index(
            text.c_str(),
            text_length,
            cursor
        );
    }

    return cursor;
}

int TextNavigation::find_word_start(
    const std::string& text,
    int position
) {
    int word_cursor = find_nearest_word_character(text, position);
    int text_length = (int)text.length();

    if (word_cursor < 0) {
        return 0;
    }

    while (word_cursor > 0) {
        int previous = Utf8Text::previous_index(
            text.c_str(),
            text_length,
            word_cursor
        );

        if (is_whitespace_at(text, previous)) {
            break;
        }

        word_cursor = previous;
    }

    return word_cursor;
}

int TextNavigation::find_word_end(
    const std::string& text,
    int position
) {
    int word_cursor = find_nearest_word_character(text, position);
    int text_length = (int)text.length();

    if (word_cursor < 0) {
        return 0;
    }

    word_cursor = Utf8Text::next_index(
        text.c_str(),
        text_length,
        word_cursor
    );

    while (
        word_cursor < text_length &&
        !is_whitespace_at(text, word_cursor)
    ) {
        word_cursor = Utf8Text::next_index(
            text.c_str(),
            text_length,
            word_cursor
        );
    }

    return word_cursor;
}

int TextNavigation::find_line_start(
    const std::string& text,
    int position
) {
    int cursor = clamp_position(text, position);

    while (cursor > 0) {
        int previous = Utf8Text::previous_index(
            text.c_str(),
            (int)text.length(),
            cursor
        );

        char previous_character = text[previous];
        if (
            previous_character == '\r' ||
            previous_character == '\n'
        ) {
            break;
        }

        cursor = previous;
    }

    return cursor;
}

int TextNavigation::find_line_end(
    const std::string& text,
    int position
) {
    int cursor = clamp_position(text, position);
    int text_length = (int)text.length();

    while (cursor < text_length) {
        char character = text[cursor];

        if (
            character == '\r' ||
            character == '\n'
        ) {
            break;
        }

        cursor = Utf8Text::next_index(
            text.c_str(),
            text_length,
            cursor
        );
    }

    return cursor;
}
