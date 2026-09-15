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

        virtual int measure_formatted_text_height(
            const char* text,
            int text_length,
            const TextFormat* formats,
            int format_count,
            int line_spacing
        ) {
            (void)text;
            (void)text_length;
            (void)formats;
            (void)format_count;
            (void)line_spacing;
            return 16;
        }

        virtual int get_formatted_character_index_at_point(
            const char* text,
            int text_length,
            const TextFormat* formats,
            int format_count,
            int pixel_x,
            int pixel_y,
            int line_spacing
        ) {
            (void)pixel_y;
            (void)line_spacing;
            return get_formatted_character_index_at_x(
                text,
                text_length,
                formats,
                format_count,
                pixel_x
            );
        }
};
