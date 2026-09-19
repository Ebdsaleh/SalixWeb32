// =================================================================================
// Filename:    engine/renderers/win32/Win32ScrollableTextPainter.cpp
// Author:      Ebdsaleh
// Description: Implements scroll-aware Win32 painting for custom TextInput widgets.
// =================================================================================

#include <string.h>

#include "Win32ScrollableTextPainter.h"
#include "Win32EmoticonPainter.h"
#include "engine/platform/win32/Win32TextFontCache.h"
#include "engine/platform/win32/Win32Utf8Text.h"
#include "framework/EmoticonRegistry.h"
#include "framework/TextInput.h"
#include "framework/TextFormat.h"
#include "framework/TextViewportState.h"
#include "framework/Utf8Text.h"
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

    int measure_span(
        HDC device_context,
        Win32TextFontCache& font_cache,
        const char* text,
        int text_length,
        const TextFormat& format,
        int* text_height
    ) {
        if (text_height != 0) {
            *text_height = 0;
        }

        if (
            device_context == NULL ||
            text == 0 ||
            text_length <= 0
        ) {
            return 0;
        }

        HFONT font = font_cache.get_font_for_text(
            format,
            text,
            text_length
        );
        if (font == NULL) {
            return 0;
        }

        HGDIOBJ previous_font = SelectObject(device_context, font);

        SIZE size;
        size.cx = 0;
        size.cy = 0;
        Win32Utf8Text::get_text_extent(
            device_context,
            text,
            text_length,
            size
        );

        if (text_height != 0) {
            *text_height = size.cy;
        }

        if (previous_font != NULL && previous_font != HGDI_ERROR) {
            SelectObject(device_context, previous_font);
        }

        return size.cx;
    }

    int measure_character(
        HDC device_context,
        Win32TextFontCache& font_cache,
        char character,
        const TextFormat& format,
        int* character_height
    ) {
        return measure_span(
            device_context,
            font_cache,
            &character,
            1,
            format,
            character_height
        );
    }

    int find_format_run_end(
        const TextInput& text_input,
        const char* text,
        int text_length,
        int start,
        int line_end
    ) {
        TextFormat format = text_input.get_character_format(start);
        int position = Utf8Text::next_index(
            text,
            text_length,
            start
        );

        while (position < line_end) {
            TextFormat next_format =
                text_input.get_character_format(position);

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
        Win32TextFontCache& font_cache,
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

            int run_end = find_format_run_end(
                text_input,
                text,
                text_length,
                position,
                line_end
            );

            width += measure_span(
                device_context,
                font_cache,
                text + position,
                run_end - position,
                format,
                0
            );
            position = run_end;
        }

        return width;
    }

    int measure_line_height(
        HDC device_context,
        Win32TextFontCache& font_cache,
        const TextInput& text_input,
        const char* text,
        int text_length,
        int line_start,
        int line_end
    ) {
        TextFormat base_format = text_input.get_character_format(line_start);
        int sample_height = 0;
        measure_character(
            device_context,
            font_cache,
            'M',
            base_format,
            &sample_height
        );

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

            int run_end = find_format_run_end(
                text_input,
                text,
                text_length,
                position,
                line_end
            );

            int run_height = 0;
            measure_span(
                device_context,
                font_cache,
                text + position,
                run_end - position,
                format,
                &run_height
            );

            if (run_height > height) {
                height = run_height;
            }
            position = run_end;
        }

        return height;
    }

    int measure_prefix_width(
        HDC device_context,
        Win32TextFontCache& font_cache,
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
            font_cache,
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
        Win32TextFontCache& font_cache,
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

            bool selected =
                text_input.get_is_focused() &&
                text_input.is_character_selected(position);

            int run_end = find_format_run_end(
                text_input,
                text,
                text_length,
                position,
                line_end
            );

            if (text_input.get_is_focused()) {
                int selection_index = Utf8Text::next_index(
                    text,
                    text_length,
                    position
                );

                while (selection_index < run_end) {
                    if (
                        text_input.is_character_selected(selection_index) !=
                        selected
                    ) {
                        run_end = selection_index;
                        break;
                    }

                    int next_selection_index = Utf8Text::next_index(
                        text,
                        text_length,
                        selection_index
                    );

                    if (next_selection_index <= selection_index) {
                        ++selection_index;
                    } else {
                        selection_index = next_selection_index;
                    }
                }
            }

            HFONT font = font_cache.get_font_for_text(
                format,
                text + position,
                run_end - position
            );
            HGDIOBJ previous_font = NULL;
            if (font != NULL) {
                previous_font = SelectObject(device_context, font);
            }

            SIZE run_size;
            run_size.cx = 0;
            run_size.cy = 0;
            Win32Utf8Text::get_text_extent(
                device_context,
                text + position,
                run_end - position,
                run_size
            );

            int text_y = line_rect.top +
                ((line_rect.bottom - line_rect.top - run_size.cy) / 2);

            if (selected) {
                RECT selection_rect;
                selection_rect.left = x;
                selection_rect.top = line_rect.top + 1;
                selection_rect.right = x + run_size.cx;
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

            Win32Utf8Text::text_out(
                device_context,
                x,
                text_y,
                text + position,
                run_end - position
            );
            x += run_size.cx;

            if (
                font != NULL &&
                previous_font != NULL &&
                previous_font != HGDI_ERROR
            ) {
                SelectObject(device_context, previous_font);
            }
            position = run_end;
        }

        int caret_position = text_input.get_cursor_position();
        if (
            text_input.get_is_focused() &&
            caret_position >= line_start &&
            caret_position <= line_end
        ) {
            int caret_x = text_x + measure_prefix_width(
                device_context,
                font_cache,
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
    Win32TextFontCache font_cache(device_context);

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
            font_cache,
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
                font_cache,
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
