// =================================================================================
// Filename:    engine/renderers/win32/Win32ScrollableTextPainter.cpp
// Author:      Ebdsaleh
// Description: Implements scroll-aware Win32 painting for custom TextInput widgets.
// =================================================================================

#include <string.h>

#include "Win32ScrollableTextPainter.h"
#include "Win32EmoticonPainter.h"
#include "framework/EmoticonRegistry.h"
#include "framework/TextInput.h"
#include "framework/TextFormat.h"
#include "framework/TextViewportState.h"
#include "framework/Style.h"

namespace {
    COLORREF to_color_ref(const Color& color) {
        return RGB(color.red, color.green, color.blue);
    }

    void fill_rect(HDC device_context, const RECT& rect, COLORREF color) {
        HBRUSH brush = CreateSolidBrush(color);
        FillRect(device_context, &rect, brush);
        DeleteObject(brush);
    }

    void frame_rect(
        HDC device_context,
        RECT rect,
        COLORREF color,
        int border_width
    ) {
        if (border_width <= 0) {
            return;
        }

        HBRUSH brush = CreateSolidBrush(color);
        for (int index = 0; index < border_width; ++index) {
            FrameRect(device_context, &rect, brush);
            InflateRect(&rect, -1, -1);
        }
        DeleteObject(brush);
    }

    HFONT create_formatted_font(
        HDC device_context,
        const TextFormat& format
    ) {
        int font_size = format.font_size;
        if (font_size < 1) {
            font_size = 1;
        }

        int logical_height = -MulDiv(
            font_size,
            GetDeviceCaps(device_context, LOGPIXELSY),
            72
        );

        const char* font_name = format.code_style == TextFormat::code_none
            ? "Tahoma"
            : "Courier New";

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
            font_name
        );
    }

    int measure_character(
        HDC device_context,
        char character,
        const TextFormat& format,
        int* character_height
    ) {
        if (character_height != 0) {
            *character_height = 0;
        }

        HFONT font = create_formatted_font(device_context, format);
        if (font == NULL) {
            return 0;
        }

        HGDIOBJ previous_font = SelectObject(device_context, font);
        SIZE size;
        size.cx = 0;
        size.cy = 0;
        GetTextExtentPoint32A(device_context, &character, 1, &size);

        if (character_height != 0) {
            *character_height = size.cy;
        }

        if (previous_font != NULL && previous_font != HGDI_ERROR) {
            SelectObject(device_context, previous_font);
        }
        DeleteObject(font);
        return size.cx;
    }

    bool is_source_span_selected(
        const TextInput& text_input,
        int start,
        int length
    ) {
        for (int index = 0; index < length; ++index) {
            if (text_input.is_character_selected(start + index)) {
                return true;
            }
        }
        return false;
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

    int measure_line_width(
        HDC device_context,
        const TextInput& text_input,
        const char* text,
        int text_length,
        int line_start,
        int line_end
    ) {
        int width = 0;
        int position = line_start;

        while (position < line_end) {
            TextFormat format = text_input.get_character_format(position);
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

            width += measure_character(
                device_context,
                text[position],
                format,
                0
            );
            ++position;
        }

        return width;
    }

    int measure_line_height(
        HDC device_context,
        const TextInput& text_input,
        const char* text,
        int text_length,
        int line_start,
        int line_end
    ) {
        TextFormat base_format = text_input.get_character_format(line_start);
        int sample_height = 0;
        measure_character(device_context, 'M', base_format, &sample_height);

        int height = sample_height;
        if (height < 16) {
            height = 16;
        }

        int position = line_start;
        while (position < line_end) {
            TextFormat format = text_input.get_character_format(position);
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

            int character_height = 0;
            measure_character(
                device_context,
                text[position],
                format,
                &character_height
            );
            if (character_height > height) {
                height = character_height;
            }
            ++position;
        }

        return height;
    }

    int measure_prefix_width(
        HDC device_context,
        const TextInput& text_input,
        const char* text,
        int text_length,
        int line_start,
        int end_index
    ) {
        if (end_index < line_start) {
            end_index = line_start;
        }
        if (end_index > text_length) {
            end_index = text_length;
        }

        return measure_line_width(
            device_context,
            text_input,
            text,
            text_length,
            line_start,
            end_index
        );
    }

    void draw_caret(
        HDC device_context,
        int x,
        int top,
        int bottom
    ) {
        HPEN pen = CreatePen(
            PS_SOLID,
            1,
            GetSysColor(COLOR_WINDOWTEXT)
        );
        if (pen == NULL) {
            return;
        }

        HGDIOBJ old_pen = SelectObject(device_context, pen);
        MoveToEx(device_context, x, top, NULL);
        LineTo(device_context, x, bottom);
        if (old_pen != NULL && old_pen != HGDI_ERROR) {
            SelectObject(device_context, old_pen);
        }
        DeleteObject(pen);
    }

    void draw_line(
        HDC device_context,
        const TextInput& text_input,
        const char* text,
        int text_length,
        const RECT& line_rect,
        int text_x,
        COLORREF normal_text_color,
        int line_start,
        int line_end
    ) {
        int x = text_x;
        int position = line_start;

        while (position < line_end) {
            TextFormat format = text_input.get_character_format(position);
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

                RECT visual_rect;
                visual_rect.left = x;
                visual_rect.right = x + visual_size;
                visual_rect.top = line_rect.top +
                    ((line_rect.bottom - line_rect.top - visual_size) / 2);
                visual_rect.bottom = visual_rect.top + visual_size;

                if (
                    text_input.get_is_focused() &&
                    text_input.has_selection() &&
                    is_source_span_selected(
                        text_input,
                        position,
                        alias_length
                    )
                ) {
                    RECT selection_rect = visual_rect;
                    selection_rect.left -= 1;
                    selection_rect.right += 1;
                    fill_rect(
                        device_context,
                        selection_rect,
                        GetSysColor(COLOR_HIGHLIGHT)
                    );
                }

                Win32EmoticonPainter::draw(
                    device_context,
                    emoticon_id,
                    visual_rect
                );

                x += visual_size;
                position += alias_length;
                continue;
            }

            HFONT font = create_formatted_font(device_context, format);
            HGDIOBJ previous_font = NULL;
            if (font != NULL) {
                previous_font = SelectObject(device_context, font);
            }

            SIZE character_size;
            character_size.cx = 0;
            character_size.cy = 0;
            GetTextExtentPoint32A(
                device_context,
                text + position,
                1,
                &character_size
            );

            int text_y = line_rect.top +
                ((line_rect.bottom - line_rect.top - character_size.cy) / 2);

            bool selected =
                text_input.get_is_focused() &&
                text_input.is_character_selected(position);

            if (selected) {
                RECT selection_rect;
                selection_rect.left = x;
                selection_rect.top = line_rect.top + 1;
                selection_rect.right = x + character_size.cx;
                selection_rect.bottom = line_rect.bottom - 1;
                if (selection_rect.right <= selection_rect.left) {
                    selection_rect.right = selection_rect.left + 1;
                }
                fill_rect(
                    device_context,
                    selection_rect,
                    GetSysColor(COLOR_HIGHLIGHT)
                );
                SetTextColor(
                    device_context,
                    GetSysColor(COLOR_HIGHLIGHTTEXT)
                );
            } else {
                SetTextColor(device_context, normal_text_color);
            }

            TextOutA(
                device_context,
                x,
                text_y,
                text + position,
                1
            );
            x += character_size.cx;

            if (
                font != NULL &&
                previous_font != NULL &&
                previous_font != HGDI_ERROR
            ) {
                SelectObject(device_context, previous_font);
            }
            if (font != NULL) {
                DeleteObject(font);
            }
            ++position;
        }

        int caret_position = text_input.get_cursor_position();
        if (
            text_input.get_is_focused() &&
            caret_position >= line_start &&
            caret_position <= line_end
        ) {
            int caret_x = text_x + measure_prefix_width(
                device_context,
                text_input,
                text,
                text_length,
                line_start,
                caret_position
            );

            draw_caret(
                device_context,
                caret_x,
                line_rect.top + 2,
                line_rect.bottom - 2
            );
        }
    }
}

void Win32ScrollableTextPainter::render_text_input(
    HDC device_context,
    const TextInput& text_input
) {
    if (
        device_context == NULL ||
        !text_input.get_is_visible() ||
        text_input.get_width() <= 0 ||
        text_input.get_height() <= 0
    ) {
        return;
    }

    RECT input_rect;
    input_rect.left = text_input.get_x();
    input_rect.top = text_input.get_y();
    input_rect.right = text_input.get_x() + text_input.get_width();
    input_rect.bottom = text_input.get_y() + text_input.get_height();

    fill_rect(
        device_context,
        input_rect,
        to_color_ref(text_input.get_style().background_color)
    );

    Color border_color = text_input.get_style().border_color;
    if (text_input.get_is_focused()) {
        border_color = Color(49, 106, 197);
    }

    frame_rect(
        device_context,
        input_rect,
        to_color_ref(border_color),
        text_input.get_style().border_width
    );

    RECT viewport_rect = input_rect;
    viewport_rect.left += text_input.get_text_padding();
    viewport_rect.right -= text_input.get_text_padding();

    if (text_input.get_is_multiline()) {
        viewport_rect.top += text_input.get_text_padding();
        viewport_rect.bottom -= text_input.get_text_padding();
    }

    if (viewport_rect.right < viewport_rect.left) {
        viewport_rect.right = viewport_rect.left;
    }
    if (viewport_rect.bottom < viewport_rect.top) {
        viewport_rect.bottom = viewport_rect.top;
    }

    int scroll_x = 0;
    int scroll_y = 0;
    TextViewportState::get_scroll(&text_input, scroll_x, scroll_y);

    int saved_state = SaveDC(device_context);
    IntersectClipRect(
        device_context,
        viewport_rect.left,
        viewport_rect.top,
        viewport_rect.right,
        viewport_rect.bottom
    );

    const char* text = text_input.get_text();
    if (text == 0) {
        text = "";
    }
    int text_length = (int)strlen(text);

    COLORREF normal_text_color = to_color_ref(
        text_input.get_style().foreground_color
    );
    COLORREF old_text_color = SetTextColor(
        device_context,
        normal_text_color
    );

    int line_start = 0;
    int y = viewport_rect.top - scroll_y;

    while (line_start <= text_length) {
        int line_end = find_line_end(text, text_length, line_start);
        int line_height = measure_line_height(
            device_context,
            text_input,
            text,
            text_length,
            line_start,
            line_end
        );

        RECT line_rect;
        line_rect.left = viewport_rect.left;
        line_rect.right = viewport_rect.right;
        line_rect.top = y;
        line_rect.bottom = y + line_height;

        if (
            line_rect.bottom >= viewport_rect.top &&
            line_rect.top <= viewport_rect.bottom
        ) {
            draw_line(
                device_context,
                text_input,
                text,
                text_length,
                line_rect,
                viewport_rect.left - scroll_x,
                normal_text_color,
                line_start,
                line_end
            );
        }

        y += line_height + text_input.get_line_spacing();

        if (line_end >= text_length) {
            break;
        }
        line_start = line_end + 1;
    }

    SetTextColor(device_context, old_text_color);
    if (saved_state != 0) {
        RestoreDC(device_context, saved_state);
    }
}
