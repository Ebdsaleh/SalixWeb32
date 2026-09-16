// =================================================================================
// Filename:    framework/FormattedText.h
// Author:      Ebdsaleh
// Description: Declares portable text content with per-character formatting.
// =================================================================================
#pragma once

#include <string>
#include <vector>

#include "TextFormat.h"

class FormattedText {
    public:
        FormattedText();

        void clear();
        bool empty() const;

        void set_plain_text(
            const char* new_text,
            const TextFormat& format
        );
        void set_formatted_text(
            const char* new_text,
            const TextFormat* formats,
            int format_count,
            const TextFormat& fallback_format
        );

        void append_plain_text(
            const char* appended_text,
            const TextFormat& format
        );
        void append_formatted_text(const FormattedText& appended_text);

        const char* get_text() const;
        int get_length() const;
        int get_format_count() const;
        const TextFormat* get_format_data() const;
        TextFormat get_character_format(int index) const;
        bool set_character_format(int index, const TextFormat& format);
        int get_max_font_size() const;

    private:
        void normalize_format_count(const TextFormat& fallback_format);

        std::string text;
        std::vector<TextFormat> character_formats;
};
