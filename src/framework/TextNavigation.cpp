// =================================================================================
// Filename:    framework/TextNavigation.cpp
// Author:      Ebdsaleh
// Description: Implements backend-neutral word-boundary cursor navigation.
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
