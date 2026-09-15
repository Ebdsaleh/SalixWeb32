// =================================================================================
// Filename:    engine/platform/win32/Win32TextMetrics.cpp
// Author:      Ebdsaleh
// Description: Implements Win32 text measurement for framework hit-testing.
// =================================================================================

#include "Win32TextMetrics.h"

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
