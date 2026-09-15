// =================================================================================
// Filename:    framework/TextNavigation.cpp
// Author:      Ebdsaleh
// Description: Implements backend-neutral word and line cursor/selection helpers.
// =================================================================================

#include <ctype.h>

#include "TextNavigation.h"

namespace {
    bool is_whitespace(char character) {
        return isspace((unsigned char)character) != 0;
    }

    int clamp_position(const std::string& text, int position) {
        if (position < 0) {
            return 0;
        }

        if (position > (int)text.length()) {
            return (int)text.length();
        }

        return position;
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

        if (cursor < text_length && !is_whitespace(text[cursor])) {
            return cursor;
        }

        if (cursor > 0 && !is_whitespace(text[cursor - 1])) {
            return cursor - 1;
        }

        int right_cursor = cursor;
        while (
            right_cursor < text_length &&
            is_whitespace(text[right_cursor])
        ) {
            ++right_cursor;
        }

        if (right_cursor < text_length) {
            return right_cursor;
        }

        int left_cursor = cursor - 1;
        while (
            left_cursor >= 0 &&
            is_whitespace(text[left_cursor])
        ) {
            --left_cursor;
        }

        return left_cursor;
    }
}

int TextNavigation::find_word_boundary_left(
    const std::string& text,
    int position
) {
    int cursor = clamp_position(text, position);

    while (cursor > 0 && is_whitespace(text[cursor - 1])) {
        --cursor;
    }

    while (cursor > 0 && !is_whitespace(text[cursor - 1])) {
        --cursor;
    }

    return cursor;
}

int TextNavigation::find_word_boundary_right(
    const std::string& text,
    int position
) {
    int cursor = clamp_position(text, position);
    int text_length = (int)text.length();

    while (cursor < text_length && is_whitespace(text[cursor])) {
        ++cursor;
    }

    while (cursor < text_length && !is_whitespace(text[cursor])) {
        ++cursor;
    }

    return cursor;
}

int TextNavigation::find_word_start(
    const std::string& text,
    int position
) {
    int word_cursor = find_nearest_word_character(text, position);
    if (word_cursor < 0) {
        return 0;
    }

    while (
        word_cursor > 0 &&
        !is_whitespace(text[word_cursor - 1])
    ) {
        --word_cursor;
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

    ++word_cursor;

    while (
        word_cursor < text_length &&
        !is_whitespace(text[word_cursor])
    ) {
        ++word_cursor;
    }

    return word_cursor;
}

int TextNavigation::find_line_start(
    const std::string& text,
    int position
) {
    int cursor = clamp_position(text, position);

    while (cursor > 0) {
        char previous_character = text[cursor - 1];
        if (previous_character == '\r' || previous_character == '\n') {
            break;
        }

        --cursor;
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
        if (character == '\r' || character == '\n') {
            break;
        }

        ++cursor;
    }

    return cursor;
}
