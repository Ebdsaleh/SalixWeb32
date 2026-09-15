// =================================================================================
// Filename:    engine/renderers/win32/Win32TextPainter.cpp
// Author:      Ebdsaleh
// Description: Implements formatted text and inline-emoticon painting on Win32.
// =================================================================================

#include <string>

#include "Win32TextPainter.h"
#include "Win32EmoticonPainter.h"
#include "framework/EmoticonRegistry.h"
#include "framework/Label.h"
#include "framework/TextInput.h"
#include "framework/TextFormat.h"
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

        SIZE text_size;
        text_size.cx = 0;
        text_size.cy = 0;
        GetTextExtentPoint32A(device_context, &character, 1, &text_size);

        if (character_height != 0) {
            *character_height = text_size.cy;
        }

        if (previous_font != NULL && previous_font != HGDI_ERROR) {
            SelectObject(device_context, previous_font);
        }

        DeleteObject(font);
        return text_size.cx;
    }

    typedef TextFormat (*FormatGetter)(const void* context, int index);
    typedef bool (*SelectionGetter)(const void* context, int index);

    TextFormat get_label_format(const void* context, int index) {
        const Label* label = (const Label*)context;
        return label == 0 ? TextFormat() : label->get_character_format(index);
    }

    TextFormat get_input_format(const void* context, int index) {
        const TextInput* input = (const TextInput*)context;
        return input == 0 ? TextFormat() : input->get_character_format(index);
    }

    bool is_label_character_selected(const void* context, int character_index) {
        const Label* label = (const Label*)context;
        if (label == 0 || !label->has_selection()) {
            return false;
        }

        int range_count = label->get_selection_range_count();
        for (int index = 0; index < range_count; ++index) {
            int start = 0;
            int end = 0;

            if (!label->get_selection_range(index, start, end)) {
                continue;
            }

            if (character_index >= start && character_index < end) {
                return true;
            }
        }

        return false;
    }

    bool is_input_character_selected(const void* context, int character_index) {
        const TextInput* input = (const TextInput*)context;
        return input != 0 && input->is_character_selected(character_index);
    }

    bool is_source_span_selected(
        SelectionGetter selection_getter,
        const void* context,
        int start,
        int length
    ) {
        if (selection_getter == 0 || length <= 0) {
            return false;
        }

        for (int index = 0; index < length; ++index) {
            if (selection_getter(context, start + index)) {
                return true;
            }
        }

        return false;
    }

    int measure_visual_width(
        HDC device_context,
        const char* text,
        int text_length,
        const void* context,
        FormatGetter format_getter,
        int end_index
    ) {
        if (text == 0 || text_length <= 0 || end_index <= 0) {
            return 0;
        }

        if (end_index > text_length) {
            end_index = text_length;
        }

        int width = 0;
        int position = 0;

        while (position < end_index) {
            EmoticonRegistry::EmoticonId emoticon_id;
            int alias_length = 0;

            if (EmoticonRegistry::match_at(
                    text,
                    text_length,
                    position,
                    emoticon_id,
                    alias_length
                )) {
                TextFormat format = format_getter(context, position);
                int visual_size = EmoticonRegistry::get_visual_size(
                    format.font_size
                );

                if (position + alias_length <= end_index) {
                    width += visual_size;
                } else {
                    int consumed = end_index - position;
                    width += (visual_size * consumed) / alias_length;
                }

                position += alias_length;
                continue;
            }

            width += measure_character(
                device_context,
                text[position],
                format_getter(context, position),
                0
            );
            ++position;
        }

        return width;
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

    void draw_formatted_line(
        HDC device_context,
        const char* text,
        int text_length,
        const RECT& line_rect,
        int text_x,
        COLORREF normal_text_color,
        const void* context,
        FormatGetter format_getter,
        SelectionGetter selection_getter,
        bool draw_selection,
        bool draw_text_caret,
        int caret_position
    ) {
        if (text == 0) {
            text = "";
            text_length = 0;
        }

        int x = text_x;
        int position = 0;

        while (position < text_length) {
            TextFormat format = format_getter(context, position);
            EmoticonRegistry::EmoticonId emoticon_id;
            int alias_length = 0;

            if (EmoticonRegistry::match_at(
                    text,
                    text_length,
                    position,
                    emoticon_id,
                    alias_length
                )) {
                int visual_size = EmoticonRegistry::get_visual_size(
                    format.font_size
                );

                RECT visual_rect;
                visual_rect.left = x;
                visual_rect.right = x + visual_size;
                visual_rect.top = line_rect.top +
                    ((line_rect.bottom - line_rect.top - visual_size) / 2);
                visual_rect.bottom = visual_rect.top + visual_size;

                bool selected = draw_selection && is_source_span_selected(
                    selection_getter,
                    context,
                    position,
                    alias_length
                );

                if (selected) {
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

            bool selected = draw_selection &&
                selection_getter != 0 &&
                selection_getter(context, position);

            if (selected) {
                RECT selection_rect;
                selection_rect.left = x;
                selection_rect.top = line_rect.top + 2;
                selection_rect.right = x + character_size.cx;
                selection_rect.bottom = line_rect.bottom - 2;

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

        if (draw_text_caret) {
            if (caret_position < 0) {
                caret_position = 0;
            }

            if (caret_position > text_length) {
                caret_position = text_length;
            }

            int caret_x = text_x + measure_visual_width(
                device_context,
                text,
                text_length,
                context,
                format_getter,
                caret_position
            );

            draw_caret(
                device_context,
                caret_x,
                line_rect.top + 3,
                line_rect.bottom - 3
            );
        }
    }
}

void Win32TextPainter::render_label(
    HDC device_context,
    const Label& label
) {
    if (
        device_context == NULL ||
        !label.get_is_visible() ||
        label.get_width() <= 0 ||
        label.get_height() <= 0
    ) {
        return;
    }

    RECT label_rect;
    label_rect.left = label.get_x();
    label_rect.top = label.get_y();
    label_rect.right = label.get_x() + label.get_width();
    label_rect.bottom = label.get_y() + label.get_height();

    int saved_state = SaveDC(device_context);
    IntersectClipRect(
        device_context,
        label_rect.left,
        label_rect.top,
        label_rect.right,
        label_rect.bottom
    );

    const char* text = label.get_text();
    int text_length = text == 0 ? 0 : (int)strlen(text);

    int text_width = measure_visual_width(
        device_context,
        text,
        text_length,
        &label,
        get_label_format,
        text_length
    );

    int text_x = label_rect.left;

    switch (label.get_horizontal_alignment()) {
        case Label::align_center:
            text_x += (label.get_width() - text_width) / 2;
            break;

        case Label::align_right:
            text_x += label.get_width() - text_width;
            break;

        case Label::align_left:
        default:
            break;
    }

    COLORREF old_text_color = SetTextColor(
        device_context,
        to_color_ref(label.get_style().foreground_color)
    );

    draw_formatted_line(
        device_context,
        text,
        text_length,
        label_rect,
        text_x,
        to_color_ref(label.get_style().foreground_color),
        &label,
        get_label_format,
        is_label_character_selected,
        label.has_selection(),
        label.get_is_selectable() && label.get_is_focused(),
        label.get_cursor_position()
    );

    SetTextColor(device_context, old_text_color);

    if (saved_state != 0) {
        RestoreDC(device_context, saved_state);
    }
}

void Win32TextPainter::render_text_input(
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

    RECT text_rect = input_rect;
    text_rect.left += text_input.get_text_padding();
    text_rect.right -= text_input.get_text_padding();

    if (text_rect.right < text_rect.left) {
        text_rect.right = text_rect.left;
    }

    int saved_state = SaveDC(device_context);
    IntersectClipRect(
        device_context,
        text_rect.left,
        text_rect.top,
        text_rect.right,
        text_rect.bottom
    );

    const char* text = text_input.get_text();
    int text_length = text == 0 ? 0 : (int)strlen(text);

    COLORREF old_text_color = SetTextColor(
        device_context,
        to_color_ref(text_input.get_style().foreground_color)
    );

    draw_formatted_line(
        device_context,
        text,
        text_length,
        text_rect,
        text_rect.left,
        to_color_ref(text_input.get_style().foreground_color),
        &text_input,
        get_input_format,
        is_input_character_selected,
        text_input.get_is_focused() && text_input.has_selection(),
        text_input.get_is_focused(),
        text_input.get_cursor_position()
    );

    SetTextColor(device_context, old_text_color);

    if (saved_state != 0) {
        RestoreDC(device_context, saved_state);
    }
}
