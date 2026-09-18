// =================================================================================
// Filename:    framework/TextWrapLayout.cpp
// Author:      Ebdsaleh
// Description: Implements backend-neutral soft-wrapped formatted text layout helpers.
// =================================================================================

#include "TextWrapLayout.h"
#include "EmoticonRegistry.h"
#include "TextFormat.h"
#include "TextMetrics.h"

namespace {
    const TextFormat* get_format_pointer(
        const TextFormat* formats,
        int format_count,
        int start
    ) {
        if (
            formats == 0 ||
            format_count <= 0 ||
            start < 0 ||
            start >= format_count
        ) {
            return 0;
        }

        return formats + start;
    }

    int get_remaining_format_count(int format_count, int start, int length) {
        if (format_count <= start || length <= 0) {
            return 0;
        }

        int remaining = format_count - start;
        return remaining < length ? remaining : length;
    }

    TextFormat get_format_at(
        const TextFormat* formats,
        int format_count,
        int index
    ) {
        if (formats != 0 && index >= 0 && index < format_count) {
            return formats[index];
        }

        return TextFormat();
    }

    int find_logical_line_end(
        const char* text,
        int text_length,
        int start
    ) {
        int position = start;

        while (position < text_length && text[position] != '\n') {
            ++position;
        }

        return position;
    }

    int get_token_length(
        const char* text,
        int text_length,
        const TextFormat* formats,
        int format_count,
        int position,
        int logical_end
    ) {
        TextFormat format = get_format_at(formats, format_count, position);

        if (format.code_style == TextFormat::code_none) {
            EmoticonRegistry::EmoticonId emoticon_id;
            int alias_length = 0;

            if (
                EmoticonRegistry::match_at(
                    text,
                    text_length,
                    position,
                    emoticon_id,
                    alias_length
                ) &&
                alias_length > 0 &&
                position + alias_length <= logical_end
            ) {
                return alias_length;
            }
        }

        return 1;
    }

    int get_measurement_span_length(
        const char* text,
        int text_length,
        const TextFormat* formats,
        int format_count,
        int position,
        int logical_end
    ) {
        int token_length = get_token_length(
            text,
            text_length,
            formats,
            format_count,
            position,
            logical_end
        );

        if (
            token_length > 1 ||
            text[position] == ' ' ||
            text[position] == '\t'
        ) {
            return token_length;
        }

        int end = position + 1;

        while (end < logical_end) {
            if (text[end] == ' ' || text[end] == '\t') {
                break;
            }

            int next_token_length = get_token_length(
                text,
                text_length,
                formats,
                format_count,
                end,
                logical_end
            );

            if (next_token_length > 1) {
                break;
            }

            ++end;
        }

        return end - position;
    }

    int measure_range_width(
        const char* text,
        int text_length,
        const TextFormat* formats,
        int format_count,
        int start,
        int end,
        TextMetrics* text_metrics
    ) {
        if (
            text_metrics == 0 ||
            text == 0 ||
            start < 0 ||
            end <= start ||
            start >= text_length
        ) {
            return 0;
        }

        if (end > text_length) {
            end = text_length;
        }

        int length = end - start;

        return text_metrics->measure_formatted_text_width(
            text + start,
            length,
            get_format_pointer(formats, format_count, start),
            get_remaining_format_count(format_count, start, length)
        );
    }

    int measure_line_height(
        const char* text,
        int text_length,
        const TextFormat* formats,
        int format_count,
        const TextWrapLine& line,
        TextMetrics* text_metrics
    ) {
        (void)text_length;

        if (text_metrics == 0) {
            return 16;
        }

        int length = line.end - line.start;
        if (length < 0) {
            length = 0;
        }

        const char* line_text = text == 0 ? "" : text + line.start;
        int height = text_metrics->measure_formatted_text_height(
            line_text,
            length,
            get_format_pointer(formats, format_count, line.start),
            get_remaining_format_count(format_count, line.start, length),
            0
        );

        return height > 0 ? height : 16;
    }
}

void TextWrapLayout::build_lines(
    const char* text,
    int text_length,
    const TextFormat* formats,
    int format_count,
    int maximum_width,
    TextMetrics* text_metrics,
    std::vector<TextWrapLine>& lines
) {
    lines.clear();

    if (text == 0 || text_length < 0) {
        lines.push_back(TextWrapLine(0, 0));
        return;
    }

    int logical_start = 0;

    while (logical_start <= text_length) {
        int logical_end = find_logical_line_end(
            text,
            text_length,
            logical_start
        );

        if (
            logical_start == logical_end ||
            maximum_width <= 0 ||
            text_metrics == 0
        ) {
            lines.push_back(TextWrapLine(logical_start, logical_end));
        } else {
            int visual_start = logical_start;

            while (visual_start < logical_end) {
                int position = visual_start;
                int current_width = 0;
                int last_break_end = -1;

                while (position < logical_end) {
                    int span_length = get_measurement_span_length(
                        text,
                        text_length,
                        formats,
                        format_count,
                        position,
                        logical_end
                    );

                    int span_end = position + span_length;
                    int span_width = measure_range_width(
                        text,
                        text_length,
                        formats,
                        format_count,
                        position,
                        span_end,
                        text_metrics
                    );

                    if (current_width + span_width <= maximum_width) {
                        current_width += span_width;

                        if (
                            span_length == 1 &&
                            (
                                text[position] == ' ' ||
                                text[position] == '\t'
                            )
                        ) {
                            last_break_end = span_end;
                        }

                        position = span_end;
                        continue;
                    }

                    if (last_break_end > visual_start) {
                        break;
                    }

                    // The current word/run crosses the edge without a usable
                    // whitespace break. Fall back to atomic tokens only for
                    // this overflowing span so long identifiers/URLs still
                    // wrap exactly as before.
                    int token_position = position;

                    while (token_position < span_end) {
                        int token_length = get_token_length(
                            text,
                            text_length,
                            formats,
                            format_count,
                            token_position,
                            span_end
                        );

                        int token_end = token_position + token_length;
                        int token_width = measure_range_width(
                            text,
                            text_length,
                            formats,
                            format_count,
                            token_position,
                            token_end,
                            text_metrics
                        );

                        if (
                            current_width + token_width > maximum_width &&
                            token_position > visual_start
                        ) {
                            break;
                        }

                        current_width += token_width;

                        if (
                            token_length == 1 &&
                            (
                                text[token_position] == ' ' ||
                                text[token_position] == '\t'
                            )
                        ) {
                            last_break_end = token_end;
                        }

                        token_position = token_end;

                        if (
                            current_width > maximum_width &&
                            token_position > visual_start
                        ) {
                            break;
                        }
                    }

                    position = token_position;
                    break;
                }

                int visual_end = position;

                if (
                    position < logical_end &&
                    last_break_end > visual_start &&
                    last_break_end <= position
                ) {
                    visual_end = last_break_end;
                }

                if (visual_end <= visual_start) {
                    visual_end = visual_start + 1;
                    if (visual_end > logical_end) {
                        visual_end = logical_end;
                    }
                }

                lines.push_back(TextWrapLine(visual_start, visual_end));
                visual_start = visual_end;
            }
        }

        if (logical_end >= text_length) {
            break;
        }

        logical_start = logical_end + 1;
    }

    if (lines.empty()) {
        lines.push_back(TextWrapLine(0, 0));
    }
}

int TextWrapLayout::measure_height(
    const char* text,
    int text_length,
    const TextFormat* formats,
    int format_count,
    int maximum_width,
    int line_spacing,
    TextMetrics* text_metrics
) {
    if (line_spacing < 0) {
        line_spacing = 0;
    }

    std::vector<TextWrapLine> lines;
    build_lines(
        text,
        text_length,
        formats,
        format_count,
        maximum_width,
        text_metrics,
        lines
    );

    int total_height = 0;

    for (int index = 0; index < (int)lines.size(); ++index) {
        if (index > 0) {
            total_height += line_spacing;
        }

        total_height += measure_line_height(
            text,
            text_length,
            formats,
            format_count,
            lines[index],
            text_metrics
        );
    }

    return total_height;
}

int TextWrapLayout::get_character_index_at_point(
    const char* text,
    int text_length,
    const TextFormat* formats,
    int format_count,
    int maximum_width,
    int pixel_x,
    int pixel_y,
    int line_spacing,
    TextMetrics* text_metrics
) {
    if (text == 0 || text_length <= 0 || text_metrics == 0) {
        return 0;
    }

    if (line_spacing < 0) {
        line_spacing = 0;
    }

    std::vector<TextWrapLine> lines;
    build_lines(
        text,
        text_length,
        formats,
        format_count,
        maximum_width,
        text_metrics,
        lines
    );

    int current_y = 0;

    for (int index = 0; index < (int)lines.size(); ++index) {
        const TextWrapLine& line = lines[index];
        int line_height = measure_line_height(
            text,
            text_length,
            formats,
            format_count,
            line,
            text_metrics
        );

        int line_bottom = current_y + line_height;

        if (
            pixel_y < line_bottom + line_spacing ||
            index + 1 >= (int)lines.size()
        ) {
            int length = line.end - line.start;
            if (length <= 0) {
                return line.start;
            }

            int relative_index = text_metrics->get_formatted_character_index_at_x(
                text + line.start,
                length,
                get_format_pointer(formats, format_count, line.start),
                get_remaining_format_count(format_count, line.start, length),
                pixel_x
            );

            if (relative_index < 0) {
                relative_index = 0;
            }
            if (relative_index > length) {
                relative_index = length;
            }

            return line.start + relative_index;
        }

        current_y = line_bottom + line_spacing;
    }

    return text_length;
}
