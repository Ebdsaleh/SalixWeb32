// =================================================================================
// Filename:    framework/TextWrapLayout.h
// Author:      Ebdsaleh
// Description: Declares backend-neutral soft-wrapped formatted text layout helpers.
// =================================================================================
#pragma once

#include <vector>

struct TextFormat;
class TextMetrics;

struct TextWrapLine {
    TextWrapLine()
        : start(0),
          end(0) {
    }

    TextWrapLine(int new_start, int new_end)
        : start(new_start),
          end(new_end) {
    }

    int start;
    int end;
};

class TextWrapLayout {
    public:
        static void build_lines(
            const char* text,
            int text_length,
            const TextFormat* formats,
            int format_count,
            int maximum_width,
            TextMetrics* text_metrics,
            std::vector<TextWrapLine>& lines
        );

        static int measure_height(
            const char* text,
            int text_length,
            const TextFormat* formats,
            int format_count,
            int maximum_width,
            int line_spacing,
            TextMetrics* text_metrics
        );

        static int get_character_index_at_point(
            const char* text,
            int text_length,
            const TextFormat* formats,
            int format_count,
            int maximum_width,
            int pixel_x,
            int pixel_y,
            int line_spacing,
            TextMetrics* text_metrics
        );
};
