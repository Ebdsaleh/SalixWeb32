// =================================================================================
// Filename:    framework/TextMetrics.h
// Author:      Ebdsaleh
// Description: Declares backend-neutral text measurement used by interactive controls.
// =================================================================================
#pragma once

struct TextFormat;

class TextMetrics {
    public:
        virtual ~TextMetrics() {}

        virtual int measure_text_width(
            const char* text,
            int text_length
        ) = 0;

        virtual int get_character_index_at_x(
            const char* text,
            int text_length,
            int pixel_x
        ) = 0;

        virtual int measure_formatted_text_width(
            const char* text,
            int text_length,
            const TextFormat* formats,
            int format_count
        ) {
            (void)formats;
            (void)format_count;
            return measure_text_width(text, text_length);
        }

        virtual int get_formatted_character_index_at_x(
            const char* text,
            int text_length,
            const TextFormat* formats,
            int format_count,
            int pixel_x
        ) {
            (void)formats;
            (void)format_count;
            return get_character_index_at_x(text, text_length, pixel_x);
        }
};
