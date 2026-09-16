// =================================================================================
// Filename:    framework/MarkdownFormatter.cpp
// Author:      Ebdsaleh
// Description: Implements backend-neutral Markdown presentation formatting.
// =================================================================================

#include <string.h>
#include <string>
#include <vector>

#include "MarkdownFormatter.h"
#include "TextFormat.h"

namespace {
    struct SemanticStyle {
        SemanticStyle()
            : bold(false),
              italic(false),
              underline(false),
              font_size_override(0) {
        }

        bool bold;
        bool italic;
        bool underline;
        int font_size_override;
    };

    TextFormat apply_semantic_style(
        const TextFormat& source_format,
        const SemanticStyle& style
    ) {
        TextFormat format = source_format;

        if (style.bold) {
            format.bold = true;
        }
        if (style.italic) {
            format.italic = true;
        }
        if (style.underline) {
            format.underline = true;
        }
        if (style.font_size_override > 0) {
            format.font_size = style.font_size_override;
        }

        return format;
    }

    TextFormat get_source_format(
        const FormattedText& source,
        int index
    ) {
        if (index < 0 || index >= source.get_length()) {
            return TextFormat();
        }

        return source.get_character_format(index);
    }

    void append_character(
        std::string& output_text,
        std::vector<TextFormat>& output_formats,
        char character,
        const TextFormat& format
    ) {
        output_text += character;
        output_formats.push_back(format);
    }

    void append_generated_text(
        std::string& output_text,
        std::vector<TextFormat>& output_formats,
        const char* text,
        const TextFormat& format
    ) {
        if (text == 0) {
            return;
        }

        for (int index = 0; text[index] != '\0'; ++index) {
            append_character(
                output_text,
                output_formats,
                text[index],
                format
            );
        }
    }

    int find_delimiter(
        const char* text,
        int start,
        int end,
        const char* delimiter,
        int delimiter_length
    ) {
        if (
            text == 0 ||
            delimiter == 0 ||
            delimiter_length <= 0
        ) {
            return -1;
        }

        for (int index = start;
             index + delimiter_length <= end;
             ++index) {
            if (
                strncmp(
                    text + index,
                    delimiter,
                    delimiter_length
                ) == 0
            ) {
                return index;
            }
        }

        return -1;
    }

    void parse_inline_range(
        const FormattedText& source,
        int start,
        int end,
        const SemanticStyle& inherited_style,
        std::string& output_text,
        std::vector<TextFormat>& output_formats
    ) {
        const char* text = source.get_text();
        if (text == 0) {
            return;
        }

        int position = start;

        while (position < end) {
            if (
                text[position] == '\\' &&
                position + 1 < end
            ) {
                TextFormat format = apply_semantic_style(
                    get_source_format(source, position + 1),
                    inherited_style
                );
                append_character(
                    output_text,
                    output_formats,
                    text[position + 1],
                    format
                );
                position += 2;
                continue;
            }

            if (text[position] == '`') {
                int closing = find_delimiter(
                    text,
                    position + 1,
                    end,
                    "`",
                    1
                );

                if (closing > position + 1) {
                    SemanticStyle code_style = inherited_style;
                    code_style.font_size_override = 11;

                    for (int index = position + 1;
                         index < closing;
                         ++index) {
                        TextFormat format = apply_semantic_style(
                            get_source_format(source, index),
                            code_style
                        );
                        append_character(
                            output_text,
                            output_formats,
                            text[index],
                            format
                        );
                    }

                    position = closing + 1;
                    continue;
                }
            }

            if (
                position + 2 <= end &&
                (
                    strncmp(text + position, "**", 2) == 0 ||
                    strncmp(text + position, "__", 2) == 0
                )
            ) {
                const char* delimiter = text[position] == '*'
                    ? "**"
                    : "__";
                int closing = find_delimiter(
                    text,
                    position + 2,
                    end,
                    delimiter,
                    2
                );

                if (closing > position + 2) {
                    SemanticStyle strong_style = inherited_style;
                    strong_style.bold = true;
                    parse_inline_range(
                        source,
                        position + 2,
                        closing,
                        strong_style,
                        output_text,
                        output_formats
                    );
                    position = closing + 2;
                    continue;
                }
            }

            if (text[position] == '*' || text[position] == '_') {
                char delimiter_text[2];
                delimiter_text[0] = text[position];
                delimiter_text[1] = '\0';

                int closing = find_delimiter(
                    text,
                    position + 1,
                    end,
                    delimiter_text,
                    1
                );

                if (closing > position + 1) {
                    SemanticStyle emphasis_style = inherited_style;
                    emphasis_style.italic = true;
                    parse_inline_range(
                        source,
                        position + 1,
                        closing,
                        emphasis_style,
                        output_text,
                        output_formats
                    );
                    position = closing + 1;
                    continue;
                }
            }

            TextFormat format = apply_semantic_style(
                get_source_format(source, position),
                inherited_style
            );
            append_character(
                output_text,
                output_formats,
                text[position],
                format
            );
            ++position;
        }
    }

    int find_line_end(
        const char* text,
        int text_length,
        int line_start
    ) {
        int position = line_start;
        while (position < text_length && text[position] != '\n') {
            ++position;
        }
        return position;
    }

    int find_first_non_space(
        const char* text,
        int start,
        int end
    ) {
        int position = start;
        while (
            position < end &&
            (text[position] == ' ' || text[position] == '\t')
        ) {
            ++position;
        }
        return position;
    }

    bool is_fence_line(
        const char* text,
        int start,
        int end
    ) {
        int position = find_first_non_space(text, start, end);
        return position + 3 <= end &&
            text[position] == '`' &&
            text[position + 1] == '`' &&
            text[position + 2] == '`';
    }

    int get_heading_level(
        const char* text,
        int start,
        int end,
        int& content_start
    ) {
        int position = find_first_non_space(text, start, end);
        int level = 0;

        while (
            position + level < end &&
            text[position + level] == '#' &&
            level < 6
        ) {
            ++level;
        }

        if (
            level > 0 &&
            position + level < end &&
            text[position + level] == ' '
        ) {
            content_start = position + level + 1;
            return level;
        }

        content_start = start;
        return 0;
    }

    bool get_unordered_list_prefix(
        const char* text,
        int start,
        int end,
        int& marker_start,
        int& content_start
    ) {
        marker_start = find_first_non_space(text, start, end);

        if (
            marker_start + 1 < end &&
            (
                text[marker_start] == '*' ||
                text[marker_start] == '-' ||
                text[marker_start] == '+'
            ) &&
            text[marker_start + 1] == ' '
        ) {
            content_start = marker_start + 2;
            return true;
        }

        content_start = start;
        return false;
    }

    bool get_ordered_list_prefix(
        const char* text,
        int start,
        int end,
        int& marker_start,
        int& marker_end,
        int& content_start
    ) {
        marker_start = find_first_non_space(text, start, end);
        int position = marker_start;
        bool found_digit = false;

        while (
            position < end &&
            text[position] >= '0' &&
            text[position] <= '9'
        ) {
            found_digit = true;
            ++position;
        }

        if (
            found_digit &&
            position + 1 < end &&
            text[position] == '.' &&
            text[position + 1] == ' '
        ) {
            marker_end = position + 2;
            content_start = marker_end;
            return true;
        }

        marker_end = marker_start;
        content_start = start;
        return false;
    }

    bool get_blockquote_prefix(
        const char* text,
        int start,
        int end,
        int& marker_start,
        int& content_start
    ) {
        marker_start = find_first_non_space(text, start, end);
        if (marker_start < end && text[marker_start] == '>') {
            content_start = marker_start + 1;
            if (content_start < end && text[content_start] == ' ') {
                ++content_start;
            }
            return true;
        }

        content_start = start;
        return false;
    }

    bool is_horizontal_rule(
        const char* text,
        int start,
        int end
    ) {
        int position = find_first_non_space(text, start, end);
        if (position >= end) {
            return false;
        }

        char marker = text[position];
        if (marker != '-' && marker != '*' && marker != '_') {
            return false;
        }

        int marker_count = 0;

        for (int index = position; index < end; ++index) {
            if (text[index] == marker) {
                ++marker_count;
            } else if (text[index] != ' ' && text[index] != '\t') {
                return false;
            }
        }

        return marker_count >= 3;
    }

    int get_heading_font_size(int level) {
        switch (level) {
            case 1:
                return 18;
            case 2:
                return 16;
            case 3:
                return 14;
            case 4:
                return 13;
            case 5:
            case 6:
            default:
                return 12;
        }
    }

    void append_source_range(
        const FormattedText& source,
        int start,
        int end,
        const SemanticStyle& style,
        std::string& output_text,
        std::vector<TextFormat>& output_formats
    ) {
        const char* text = source.get_text();
        if (text == 0) {
            return;
        }

        for (int index = start; index < end; ++index) {
            TextFormat format = apply_semantic_style(
                get_source_format(source, index),
                style
            );
            append_character(
                output_text,
                output_formats,
                text[index],
                format
            );
        }
    }

    bool line_has_block_marker(
        const char* text,
        int start,
        int end
    ) {
        int content_start = start;
        int marker_start = start;
        int marker_end = start;

        if (is_fence_line(text, start, end)) {
            return true;
        }

        if (get_heading_level(text, start, end, content_start) > 0) {
            return true;
        }

        if (get_unordered_list_prefix(
                text,
                start,
                end,
                marker_start,
                content_start
            )) {
            return true;
        }

        if (get_ordered_list_prefix(
                text,
                start,
                end,
                marker_start,
                marker_end,
                content_start
            )) {
            return true;
        }

        if (get_blockquote_prefix(
                text,
                start,
                end,
                marker_start,
                content_start
            )) {
            return true;
        }

        return is_horizontal_rule(text, start, end);
    }
}

bool MarkdownFormatter::format(
    const char* source_text,
    FormattedText& output
) {
    FormattedText source;
    source.set_plain_text(
        source_text == 0 ? "" : source_text,
        TextFormat()
    );
    return format(source, output);
}

bool MarkdownFormatter::format(
    const FormattedText& source,
    FormattedText& output
) {
    output.clear();

    const char* text = source.get_text();
    int text_length = source.get_length();

    if (text == 0 || text_length <= 0) {
        return false;
    }

    std::string output_text;
    std::vector<TextFormat> output_formats;
    bool in_code_block = false;
    int line_start = 0;

    while (line_start <= text_length) {
        int line_end = find_line_end(text, text_length, line_start);
        bool has_newline = line_end < text_length;

        if (is_fence_line(text, line_start, line_end)) {
            in_code_block = !in_code_block;
        } else if (in_code_block) {
            SemanticStyle code_style;
            code_style.font_size_override = 11;
            append_source_range(
                source,
                line_start,
                line_end,
                code_style,
                output_text,
                output_formats
            );

            if (has_newline) {
                append_character(
                    output_text,
                    output_formats,
                    '\n',
                    TextFormat(false, false, false, 11)
                );
            }
        } else {
            int heading_content_start = line_start;
            int heading_level = get_heading_level(
                text,
                line_start,
                line_end,
                heading_content_start
            );

            int marker_start = line_start;
            int marker_end = line_start;
            int content_start = line_start;

            if (heading_level > 0) {
                SemanticStyle heading_style;
                heading_style.bold = true;
                heading_style.font_size_override =
                    get_heading_font_size(heading_level);

                parse_inline_range(
                    source,
                    heading_content_start,
                    line_end,
                    heading_style,
                    output_text,
                    output_formats
                );
            } else if (get_unordered_list_prefix(
                    text,
                    line_start,
                    line_end,
                    marker_start,
                    content_start
                )) {
                SemanticStyle normal_style;
                append_source_range(
                    source,
                    line_start,
                    content_start,
                    normal_style,
                    output_text,
                    output_formats
                );
                parse_inline_range(
                    source,
                    content_start,
                    line_end,
                    normal_style,
                    output_text,
                    output_formats
                );
            } else if (get_ordered_list_prefix(
                    text,
                    line_start,
                    line_end,
                    marker_start,
                    marker_end,
                    content_start
                )) {
                SemanticStyle normal_style;
                append_source_range(
                    source,
                    line_start,
                    content_start,
                    normal_style,
                    output_text,
                    output_formats
                );
                parse_inline_range(
                    source,
                    content_start,
                    line_end,
                    normal_style,
                    output_text,
                    output_formats
                );
            } else if (get_blockquote_prefix(
                    text,
                    line_start,
                    line_end,
                    marker_start,
                    content_start
                )) {
                SemanticStyle quote_style;
                quote_style.italic = true;

                append_source_range(
                    source,
                    line_start,
                    content_start,
                    quote_style,
                    output_text,
                    output_formats
                );
                parse_inline_range(
                    source,
                    content_start,
                    line_end,
                    quote_style,
                    output_text,
                    output_formats
                );
            } else if (is_horizontal_rule(text, line_start, line_end)) {
                TextFormat rule_format = get_source_format(
                    source,
                    line_start
                );
                append_generated_text(
                    output_text,
                    output_formats,
                    "------------------------",
                    rule_format
                );
            } else {
                SemanticStyle normal_style;
                parse_inline_range(
                    source,
                    line_start,
                    line_end,
                    normal_style,
                    output_text,
                    output_formats
                );
            }

            if (has_newline) {
                TextFormat newline_format = get_source_format(
                    source,
                    line_end
                );
                append_character(
                    output_text,
                    output_formats,
                    '\n',
                    newline_format
                );
            }
        }

        if (!has_newline) {
            break;
        }

        line_start = line_end + 1;
    }

    if (output_text.empty()) {
        return false;
    }

    output.set_formatted_text(
        output_text.c_str(),
        output_formats.empty() ? 0 : &output_formats[0],
        (int)output_formats.size(),
        TextFormat()
    );
    return true;
}

bool MarkdownFormatter::has_block_structure(const char* source_text) {
    if (source_text == 0 || source_text[0] == '\0') {
        return false;
    }

    int text_length = (int)strlen(source_text);

    for (int index = 0; index < text_length; ++index) {
        if (source_text[index] == '\n') {
            return true;
        }
    }

    int line_end = find_line_end(source_text, text_length, 0);
    return line_has_block_marker(source_text, 0, line_end);
}
