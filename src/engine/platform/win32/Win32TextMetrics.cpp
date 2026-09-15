// =================================================================================
// Filename:    engine/platform/win32/Win32TextMetrics.cpp
// Author:      Ebdsaleh
// Description: Implements Win32 text measurement for framework hit-testing.
// =================================================================================

#include "Win32TextMetrics.h"
#include "framework/TextFormat.h"
#include "framework/EmoticonRegistry.h"

namespace {
    HFONT create_text_font(HDC device_context, const TextFormat& format) {
        int font_size = format.font_size;
        if (font_size < 1) {
            font_size = 1;
        }

        int logical_height = -MulDiv(
            font_size,
            GetDeviceCaps(device_context, LOGPIXELSY),
            72
        );

        return CreateFontA(
            logical_height,
            0,
            0,
            0,
            format.bold ? FW_BOLD : FW_NORMAL,
            format.italic ? TRUE : FALSE,
            format.underline ? TRUE : FALSE,
            FALSE,
            DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE,
            "Tahoma"
        );
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

    int measure_character(
        HDC device_context,
        const char* character,
        const TextFormat& format
    ) {
        HFONT font = create_text_font(device_context, format);
        if (font == NULL) {
            return 0;
        }

        HGDIOBJ previous_font = SelectObject(device_context, font);

        SIZE text_size;
        text_size.cx = 0;
        text_size.cy = 0;
        GetTextExtentPoint32A(device_context, character, 1, &text_size);

        if (previous_font != NULL && previous_font != HGDI_ERROR) {
            SelectObject(device_context, previous_font);
        }

        DeleteObject(font);
        return text_size.cx;
    }
}

Win32TextMetrics::Win32TextMetrics(HDC new_device_context)
    : device_context(new_device_context) {
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

    if (!GetTextExtentPoint32A(
            device_context,
            text,
            text_length,
            &text_size
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

    for (int index = 1; index <= text_length; ++index) {
        SIZE text_size;
        text_size.cx = 0;
        text_size.cy = 0;

        if (!GetTextExtentPoint32A(
                device_context,
                text,
                index,
                &text_size
            )) {
            result_index = text_length;
            break;
        }

        int midpoint = previous_width + ((text_size.cx - previous_width) / 2);

        if (pixel_x < midpoint) {
            result_index = index - 1;
            break;
        }

        previous_width = text_size.cx;
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

    int width = 0;
    int position = 0;

    while (position < text_length) {
        EmoticonRegistry::EmoticonId emoticon_id;
        int alias_length = 0;
        TextFormat format = get_format_at(formats, format_count, position);

        if (EmoticonRegistry::match_at(
                text,
                text_length,
                position,
                emoticon_id,
                alias_length
            )) {
            width += EmoticonRegistry::get_visual_size(format.font_size);
            position += alias_length;
            continue;
        }

        width += measure_character(device_context, text + position, format);
        ++position;
    }

    return width;
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
        text_length <= 0 ||
        pixel_x <= 0
    ) {
        return 0;
    }

    int current_x = 0;
    int position = 0;

    while (position < text_length) {
        EmoticonRegistry::EmoticonId emoticon_id;
        int alias_length = 0;
        TextFormat format = get_format_at(formats, format_count, position);

        if (EmoticonRegistry::match_at(
                text,
                text_length,
                position,
                emoticon_id,
                alias_length
            )) {
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

        int character_width = measure_character(
            device_context,
            text + position,
            format
        );
        int midpoint = current_x + character_width / 2;

        if (pixel_x < midpoint) {
            return position;
        }

        current_x += character_width;
        ++position;
    }

    return text_length;
}
