// =================================================================================
// Filename:    engine/platform/win32/Win32TextMetrics.cpp
// Author:      Ebdsaleh
// Description: Implements Win32 text measurement for framework hit-testing.
// =================================================================================

#include "Win32TextMetrics.h"
#include "Win32Utf8Text.h"
#include "framework/TextFormat.h"
#include "framework/EmoticonRegistry.h"
#include "framework/Utf8Text.h"

namespace {
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

    bool formats_equal(
        const TextFormat& left,
        const TextFormat& right
    ) {
        return
            left.bold == right.bold &&
            left.italic == right.italic &&
            left.underline == right.underline &&
            left.font_size == right.font_size &&
            left.code_style == right.code_style &&
            left.syntax_style == right.syntax_style;
    }

    void measure_span(
        HDC device_context,
        Win32TextFontCache& font_cache,
        const char* text,
        int text_length,
        const TextFormat& format,
        int& width,
        int& height
    ) {
        width = 0;
        height = 0;

        if (
            device_context == NULL ||
            text == 0 ||
            text_length <= 0
        ) {
            return;
        }

        HFONT font = font_cache.get_font_for_text(
            format,
            text,
            text_length
        );
        if (font == NULL) {
            return;
        }

        HGDIOBJ previous_font = SelectObject(device_context, font);

        SIZE text_size;
        text_size.cx = 0;
        text_size.cy = 0;
        if (Win32Utf8Text::get_text_extent(
                device_context,
                text,
                text_length,
                text_size
            )) {
            width = text_size.cx;
            height = text_size.cy;
        }

        if (previous_font != NULL && previous_font != HGDI_ERROR) {
            SelectObject(device_context, previous_font);
        }
    }

    void measure_character(
        HDC device_context,
        Win32TextFontCache& font_cache,
        const char* character,
        const TextFormat& format,
        int& width,
        int& height
    ) {
        measure_span(
            device_context,
            font_cache,
            character,
            1,
            format,
            width,
            height
        );
    }

    int find_format_run_end(
        const char* text,
        int text_length,
        const TextFormat* formats,
        int format_count,
        int start,
        int line_end
    ) {
        TextFormat format = get_format_at(
            formats,
            format_count,
            start
        );

        int position = Utf8Text::next_index(
            text,
            text_length,
            start
        );

        while (position < line_end && position < text_length) {
            TextFormat next_format = get_format_at(
                formats,
                format_count,
                position
            );

            if (!formats_equal(format, next_format)) {
                break;
            }

            if (next_format.code_style == TextFormat::code_none) {
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
                    position + alias_length <= line_end
                ) {
                    break;
                }
            }

            int next = Utf8Text::next_index(
                text,
                text_length,
                position
            );

            if (next <= position) {
                ++position;
            } else {
                position = next;
            }
        }

        return position;
    }

    int get_default_line_height(
        HDC device_context,
        Win32TextFontCache& font_cache,
        const TextFormat* formats,
        int format_count,
        int source_index
    ) {
        TextFormat format = get_format_at(formats, format_count, source_index);
        int width = 0;
        int height = 0;
        char sample = 'M';
        measure_character(
            device_context,
            font_cache,
            &sample,
            format,
            width,
            height
        );

        if (height <= 0) {
            height = format.font_size + 6;
        }

        if (height < 16) {
            height = 16;
        }

        return height;
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

    int measure_line_height(
        HDC device_context,
        Win32TextFontCache& font_cache,
        const char* text,
        int text_length,
        const TextFormat* formats,
        int format_count,
        int line_start,
        int line_end
    ) {
        int height = get_default_line_height(
            device_context,
            font_cache,
            formats,
            format_count,
            line_start
        );

        int position = line_start;

        while (position < line_end && position < text_length) {
            TextFormat format = get_format_at(formats, format_count, position);
            EmoticonRegistry::EmoticonId emoticon_id;
            int alias_length = 0;

            if (
                format.code_style == TextFormat::code_none &&
                EmoticonRegistry::match_at(
                    text,
                    text_length,
                    position,
                    emoticon_id,
                    alias_length
                ) &&
                position + alias_length <= line_end
            ) {
                int visual_size = EmoticonRegistry::get_visual_size(
                    format.font_size
                );
                if (visual_size > height) {
                    height = visual_size;
                }
                position += alias_length;
                continue;
            }

            int run_end = find_format_run_end(
                text,
                text_length,
                formats,
                format_count,
                position,
                line_end
            );

            int run_width = 0;
            int run_height = 0;
            measure_span(
                device_context,
                font_cache,
                text + position,
                run_end - position,
                format,
                run_width,
                run_height
            );

            if (run_height > height) {
                height = run_height;
            }

            position = run_end;
        }

        return height;
    }

    int measure_line_width(
        HDC device_context,
        Win32TextFontCache& font_cache,
        const char* text,
        int text_length,
        const TextFormat* formats,
        int format_count,
        int line_start,
        int line_end
    ) {
        int width = 0;
        int position = line_start;

        while (position < line_end && position < text_length) {
            TextFormat format = get_format_at(formats, format_count, position);
            EmoticonRegistry::EmoticonId emoticon_id;
            int alias_length = 0;

            if (
                format.code_style == TextFormat::code_none &&
                EmoticonRegistry::match_at(
                    text,
                    text_length,
                    position,
                    emoticon_id,
                    alias_length
                ) &&
                position + alias_length <= line_end
            ) {
                width += EmoticonRegistry::get_visual_size(format.font_size);
                position += alias_length;
                continue;
            }

            int run_end = find_format_run_end(
                text,
                text_length,
                formats,
                format_count,
                position,
                line_end
            );

            int run_width = 0;
            int run_height = 0;
            measure_span(
                device_context,
                font_cache,
                text + position,
                run_end - position,
                format,
                run_width,
                run_height
            );

            width += run_width;
            position = run_end;
        }

        return width;
    }

    int get_index_at_x_range(
        HDC device_context,
        Win32TextFontCache& font_cache,
        const char* text,
        int text_length,
        const TextFormat* formats,
        int format_count,
        int line_start,
        int line_end,
        int pixel_x
    ) {
        if (pixel_x <= 0) {
            return line_start;
        }

        int current_x = 0;
        int position = line_start;

        while (position < line_end && position < text_length) {
            TextFormat format = get_format_at(formats, format_count, position);
            EmoticonRegistry::EmoticonId emoticon_id;
            int alias_length = 0;

            if (
                format.code_style == TextFormat::code_none &&
                EmoticonRegistry::match_at(
                    text,
                    text_length,
                    position,
                    emoticon_id,
                    alias_length
                ) &&
                position + alias_length <= line_end
            ) {
                int visual_width = EmoticonRegistry::get_visual_size(
                    format.font_size
                );
                int midpoint = current_x + visual_width / 2;
                int right = current_x + visual_width;

                if (pixel_x < right) {
                    return pixel_x < midpoint
                        ? position
                        : position + alias_length;
                }

                current_x = right;
                position += alias_length;
                continue;
            }

            int run_end = find_format_run_end(
                text,
                text_length,
                formats,
                format_count,
                position,
                line_end
            );

            int run_width = 0;
            int run_height = 0;
            measure_span(
                device_context,
                font_cache,
                text + position,
                run_end - position,
                format,
                run_width,
                run_height
            );

            if (pixel_x >= current_x + run_width) {
                current_x += run_width;
                position = run_end;
                continue;
            }

            while (position < run_end) {
                int next_position = Utf8Text::next_index(
                    text,
                    text_length,
                    position
                );

                if (
                    next_position <= position ||
                    next_position > run_end
                ) {
                    next_position = run_end;
                }

                int character_width = 0;
                int character_height = 0;
                measure_span(
                    device_context,
                    font_cache,
                    text + position,
                    next_position - position,
                    format,
                    character_width,
                    character_height
                );

                int midpoint = current_x + character_width / 2;
                if (pixel_x < midpoint) {
                    return position;
                }

                current_x += character_width;
                position = next_position;
            }
        }

        return line_end;
    }
}

Win32TextMetrics::Win32TextMetrics(HDC new_device_context)
    : device_context(new_device_context),
      font_cache(new_device_context) {
}

int Win32TextMetrics::measure_text_width(
    const char* text,
    int text_length
) {
    if (
        device_context == NULL ||
        text == 0 ||
        text_length <= 0
    ) {
        return 0;
    }

    HFONT gui_font = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    HGDIOBJ previous_font = SelectObject(device_context, gui_font);

    SIZE text_size;
    text_size.cx = 0;
    text_size.cy = 0;

    if (!Win32Utf8Text::get_text_extent(
            device_context,
            text,
            text_length,
            text_size
        )) {
        text_size.cx = 0;
    }

    if (previous_font != NULL && previous_font != HGDI_ERROR) {
        SelectObject(device_context, previous_font);
    }

    return text_size.cx;
}

int Win32TextMetrics::get_character_index_at_x(
    const char* text,
    int text_length,
    int pixel_x
) {
    if (
        device_context == NULL ||
        text == 0 ||
        text_length <= 0 ||
        pixel_x <= 0
    ) {
        return 0;
    }

    HFONT gui_font = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    HGDIOBJ previous_font = SelectObject(device_context, gui_font);

    int previous_width = 0;
    int result_index = text_length;
    int position = 0;

    while (position < text_length) {
        int next_position = Utf8Text::next_index(
            text,
            text_length,
            position
        );

        if (next_position <= position) {
            next_position = position + 1;
        }

        SIZE text_size;
        text_size.cx = 0;
        text_size.cy = 0;

        if (!Win32Utf8Text::get_text_extent(
                device_context,
                text,
                next_position,
                text_size
            )) {
            result_index = text_length;
            break;
        }

        int midpoint =
            previous_width +
            ((text_size.cx - previous_width) / 2);

        if (pixel_x < midpoint) {
            result_index = position;
            break;
        }

        previous_width = text_size.cx;
        position = next_position;
    }

    if (previous_font != NULL && previous_font != HGDI_ERROR) {
        SelectObject(device_context, previous_font);
    }

    return result_index;
}

int Win32TextMetrics::measure_formatted_text_width(
    const char* text,
    int text_length,
    const TextFormat* formats,
    int format_count
) {
    if (
        device_context == NULL ||
        text == 0 ||
        text_length <= 0
    ) {
        return 0;
    }

    int maximum_width = 0;
    int line_start = 0;

    while (line_start <= text_length) {
        int line_end = find_line_end(text, text_length, line_start);
        int width = measure_line_width(
            device_context,
            font_cache,
            text,
            text_length,
            formats,
            format_count,
            line_start,
            line_end
        );

        if (width > maximum_width) {
            maximum_width = width;
        }

        if (line_end >= text_length) {
            break;
        }

        line_start = line_end + 1;
    }

    return maximum_width;
}

int Win32TextMetrics::get_formatted_character_index_at_x(
    const char* text,
    int text_length,
    const TextFormat* formats,
    int format_count,
    int pixel_x
) {
    if (
        device_context == NULL ||
        text == 0 ||
        text_length <= 0
    ) {
        return 0;
    }

    int line_end = find_line_end(text, text_length, 0);
    return get_index_at_x_range(
        device_context,
        font_cache,
        text,
        text_length,
        formats,
        format_count,
        0,
        line_end,
        pixel_x
    );
}

int Win32TextMetrics::measure_formatted_text_height(
    const char* text,
    int text_length,
    const TextFormat* formats,
    int format_count,
    int line_spacing
) {
    if (device_context == NULL) {
        return 0;
    }

    if (line_spacing < 0) {
        line_spacing = 0;
    }

    if (text == 0) {
        text = "";
        text_length = 0;
    }

    int total_height = 0;
    int line_start = 0;
    bool first_line = true;

    while (line_start <= text_length) {
        int line_end = find_line_end(text, text_length, line_start);
        int line_height = measure_line_height(
            device_context,
            font_cache,
            text,
            text_length,
            formats,
            format_count,
            line_start,
            line_end
        );

        if (!first_line) {
            total_height += line_spacing;
        }

        total_height += line_height;
        first_line = false;

        if (line_end >= text_length) {
            break;
        }

        line_start = line_end + 1;
    }

    return total_height;
}

int Win32TextMetrics::get_formatted_character_index_at_point(
    const char* text,
    int text_length,
    const TextFormat* formats,
    int format_count,
    int pixel_x,
    int pixel_y,
    int line_spacing
) {
    if (device_context == NULL || text == 0 || text_length < 0) {
        return 0;
    }

    if (line_spacing < 0) {
        line_spacing = 0;
    }

    int line_start = 0;
    int current_y = 0;

    while (line_start <= text_length) {
        int line_end = find_line_end(text, text_length, line_start);
        int line_height = measure_line_height(
            device_context,
            font_cache,
            text,
            text_length,
            formats,
            format_count,
            line_start,
            line_end
        );

        int line_bottom = current_y + line_height;

        if (pixel_y < line_bottom + line_spacing || line_end >= text_length) {
            return get_index_at_x_range(
                device_context,
                font_cache,
                text,
                text_length,
                formats,
                format_count,
                line_start,
                line_end,
                pixel_x
            );
        }

        current_y = line_bottom + line_spacing;
        line_start = line_end + 1;
    }

    return text_length;
}
