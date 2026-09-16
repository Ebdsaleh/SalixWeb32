// =================================================================================
// Filename:    framework/FormattedText.cpp
// Author:      Ebdsaleh
// Description: Implements portable text content with per-character formatting.
// =================================================================================

#include "FormattedText.h"

FormattedText::FormattedText() {
}

void FormattedText::clear() {
    text.clear();
    character_formats.clear();
}

bool FormattedText::empty() const {
    return text.empty();
}

void FormattedText::set_plain_text(
    const char* new_text,
    const TextFormat& format
) {
    text = new_text == 0 ? "" : new_text;
    character_formats.assign(text.length(), format);
}

void FormattedText::set_formatted_text(
    const char* new_text,
    const TextFormat* formats,
    int format_count,
    const TextFormat& fallback_format
) {
    text = new_text == 0 ? "" : new_text;
    character_formats.clear();

    if (formats != 0 && format_count > 0) {
        int copy_count = format_count;
        if (copy_count > (int)text.length()) {
            copy_count = (int)text.length();
        }

        character_formats.insert(
            character_formats.end(),
            formats,
            formats + copy_count
        );
    }

    normalize_format_count(fallback_format);
}

void FormattedText::append_plain_text(
    const char* appended_text,
    const TextFormat& format
) {
    if (appended_text == 0 || appended_text[0] == '\0') {
        return;
    }

    normalize_format_count(TextFormat());

    std::string value(appended_text);
    text += value;
    character_formats.insert(
        character_formats.end(),
        value.length(),
        format
    );
}

void FormattedText::append_formatted_text(
    const FormattedText& appended_text
) {
    normalize_format_count(TextFormat());

    text += appended_text.text;

    if (!appended_text.character_formats.empty()) {
        character_formats.insert(
            character_formats.end(),
            appended_text.character_formats.begin(),
            appended_text.character_formats.end()
        );
    }

    normalize_format_count(TextFormat());
}

const char* FormattedText::get_text() const {
    return text.c_str();
}

int FormattedText::get_length() const {
    return (int)text.length();
}

int FormattedText::get_format_count() const {
    return (int)character_formats.size();
}

const TextFormat* FormattedText::get_format_data() const {
    if (character_formats.empty()) {
        return 0;
    }

    return &character_formats[0];
}

TextFormat FormattedText::get_character_format(int index) const {
    if (index < 0 || index >= (int)character_formats.size()) {
        return TextFormat();
    }

    return character_formats[index];
}

bool FormattedText::set_character_format(
    int index,
    const TextFormat& format
) {
    if (index < 0 || index >= (int)text.length()) {
        return false;
    }

    normalize_format_count(TextFormat());
    character_formats[index] = format;
    return true;
}

int FormattedText::get_max_font_size() const {
    int maximum = 12;

    for (int index = 0; index < (int)character_formats.size(); ++index) {
        if (character_formats[index].font_size > maximum) {
            maximum = character_formats[index].font_size;
        }
    }

    return maximum;
}

void FormattedText::normalize_format_count(
    const TextFormat& fallback_format
) {
    if (character_formats.size() < text.length()) {
        character_formats.resize(text.length(), fallback_format);
    } else if (character_formats.size() > text.length()) {
        character_formats.erase(
            character_formats.begin() + text.length(),
            character_formats.end()
        );
    }
}
