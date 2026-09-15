// =================================================================================
// Filename:    engine/renderers/win32/Win32ComponentRenderer.cpp
// Author:      Ebdsaleh
// Description: Implements Win32 rendering for framework UI components.
// =================================================================================

#include <string>

#include "Win32ComponentRenderer.h"
#include "framework/Panel.h"
#include "framework/Label.h"
#include "framework/Button.h"
#include "framework/TextInput.h"
#include "framework/Style.h"

namespace {
    COLORREF to_color_ref(const Color& color) {
        return RGB(color.red, color.green, color.blue);
    }

    unsigned char adjust_channel(unsigned char channel, int amount) {
        int adjusted = (int)channel + amount;

        if (adjusted < 0) {
            adjusted = 0;
        }

        if (adjusted > 255) {
            adjusted = 255;
        }

        return (unsigned char)adjusted;
    }

    Color adjust_color(const Color& color, int amount) {
        return Color(
            adjust_channel(color.red, amount),
            adjust_channel(color.green, amount),
            adjust_channel(color.blue, amount)
        );
    }

    RECT component_rect(const Component& component) {
        RECT rect;
        rect.left = component.get_x();
        rect.top = component.get_y();
        rect.right = component.get_x() + component.get_width();
        rect.bottom = component.get_y() + component.get_height();
        return rect;
    }

    void fill_rect(HDC device_context, const RECT& rect, const Color& color) {
        HBRUSH brush = CreateSolidBrush(to_color_ref(color));
        FillRect(device_context, &rect, brush);
        DeleteObject(brush);
    }

    void frame_rect(HDC device_context, RECT rect, const Color& color, int border_width) {
        if (border_width <= 0) {
            return;
        }

        HBRUSH brush = CreateSolidBrush(to_color_ref(color));

        for (int index = 0; index < border_width; ++index) {
            FrameRect(device_context, &rect, brush);
            InflateRect(&rect, -1, -1);
        }

        DeleteObject(brush);
    }

    int measure_text_width(HDC device_context, const char* text, int length) {
        if (
            device_context == NULL ||
            text == 0 ||
            length <= 0
        ) {
            return 0;
        }

        SIZE text_size;
        text_size.cx = 0;
        text_size.cy = 0;

        if (!GetTextExtentPoint32A(
                device_context,
                text,
                length,
                &text_size
            )) {
            return 0;
        }

        return text_size.cx;
    }
}

Win32ComponentRenderer::Win32ComponentRenderer(HDC new_device_context)
    : device_context(new_device_context),
      previous_font(NULL),
      previous_background_mode(OPAQUE) {

    if (device_context != NULL) {
        HFONT gui_font = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
        previous_font = (HFONT)SelectObject(device_context, gui_font);
        previous_background_mode = SetBkMode(device_context, TRANSPARENT);
    }
}

Win32ComponentRenderer::~Win32ComponentRenderer() {
    if (device_context == NULL) {
        return;
    }

    if (previous_font != NULL) {
        SelectObject(device_context, previous_font);
    }

    SetBkMode(device_context, previous_background_mode);
}

void Win32ComponentRenderer::render_panel(const Panel& panel) {
    if (device_context == NULL || !panel.get_is_visible()) {
        return;
    }

    if (panel.get_width() <= 0 || panel.get_height() <= 0) {
        return;
    }

    RECT panel_rect = component_rect(panel);

    fill_rect(
        device_context,
        panel_rect,
        panel.get_style().background_color
    );

    frame_rect(
        device_context,
        panel_rect,
        panel.get_style().border_color,
        panel.get_style().border_width
    );
}

void Win32ComponentRenderer::render_label(const Label& label) {
    if (device_context == NULL || !label.get_is_visible()) {
        return;
    }

    if (label.get_width() <= 0 || label.get_height() <= 0) {
        return;
    }

    RECT label_rect = component_rect(label);
    UINT draw_flags = DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX;

    switch (label.get_horizontal_alignment()) {
        case Label::align_center:
            draw_flags |= DT_CENTER;
            break;

        case Label::align_right:
            draw_flags |= DT_RIGHT;
            break;

        case Label::align_left:
        default:
            draw_flags |= DT_LEFT;
            break;
    }

    COLORREF old_text_color = SetTextColor(
        device_context,
        to_color_ref(label.get_style().foreground_color)
    );

    DrawTextA(
        device_context,
        label.get_text(),
        -1,
        &label_rect,
        draw_flags
    );

    SetTextColor(device_context, old_text_color);
}

void Win32ComponentRenderer::render_button(const Button& button) {
    if (device_context == NULL || !button.get_is_visible()) {
        return;
    }

    if (button.get_width() <= 0 || button.get_height() <= 0) {
        return;
    }

    RECT button_rect = component_rect(button);
    Color background_color = button.get_style().background_color;

    if (button.get_is_pressed()) {
        background_color = adjust_color(background_color, -24);
    } else if (button.get_is_hovered()) {
        background_color = adjust_color(background_color, 12);
    }

    fill_rect(device_context, button_rect, background_color);
    frame_rect(
        device_context,
        button_rect,
        button.get_style().border_color,
        button.get_style().border_width
    );

    COLORREF old_text_color = SetTextColor(
        device_context,
        to_color_ref(button.get_style().foreground_color)
    );

    DrawTextA(
        device_context,
        button.get_text(),
        -1,
        &button_rect,
        DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX
    );

    SetTextColor(device_context, old_text_color);
}

void Win32ComponentRenderer::render_text_input(const TextInput& text_input) {
    if (device_context == NULL || !text_input.get_is_visible()) {
        return;
    }

    if (text_input.get_width() <= 0 || text_input.get_height() <= 0) {
        return;
    }

    RECT input_rect = component_rect(text_input);
    fill_rect(device_context, input_rect, text_input.get_style().background_color);

    Color border_color = text_input.get_style().border_color;
    if (text_input.get_is_focused()) {
        border_color = Color(49, 106, 197);
    }

    frame_rect(
        device_context,
        input_rect,
        border_color,
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

    const char* input_text = text_input.get_text();
    std::string display_text = input_text == 0 ? "" : input_text;

    COLORREF old_text_color = SetTextColor(
        device_context,
        to_color_ref(text_input.get_style().foreground_color)
    );

    DrawTextA(
        device_context,
        display_text.c_str(),
        -1,
        &text_rect,
        DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX
    );

    TEXTMETRICA text_metrics;
    ZeroMemory(&text_metrics, sizeof(text_metrics));
    GetTextMetricsA(device_context, &text_metrics);

    int text_y = text_rect.top +
        ((text_rect.bottom - text_rect.top - text_metrics.tmHeight) / 2);

    if (text_y < text_rect.top) {
        text_y = text_rect.top;
    }

    if (text_input.get_is_focused() && text_input.has_selection()) {
        int selection_start = text_input.get_selection_start();
        int selection_end = text_input.get_selection_end();

        if (selection_start < 0) {
            selection_start = 0;
        }

        if (selection_end > (int)display_text.length()) {
            selection_end = (int)display_text.length();
        }

        if (selection_end > selection_start) {
            int prefix_width = measure_text_width(
                device_context,
                display_text.c_str(),
                selection_start
            );

            int selection_width = measure_text_width(
                device_context,
                display_text.c_str() + selection_start,
                selection_end - selection_start
            );

            RECT selection_rect;
            selection_rect.left = text_rect.left + prefix_width;
            selection_rect.top = text_rect.top + 2;
            selection_rect.right = selection_rect.left + selection_width;
            selection_rect.bottom = text_rect.bottom - 2;

            FillRect(
                device_context,
                &selection_rect,
                GetSysColorBrush(COLOR_HIGHLIGHT)
            );

            COLORREF selection_text_color = SetTextColor(
                device_context,
                GetSysColor(COLOR_HIGHLIGHTTEXT)
            );

            std::string selected_text = display_text.substr(
                selection_start,
                selection_end - selection_start
            );

            TextOutA(
                device_context,
                selection_rect.left,
                text_y,
                selected_text.c_str(),
                (int)selected_text.length()
            );

            SetTextColor(device_context, selection_text_color);
        }
    }

    if (text_input.get_is_focused()) {
        int cursor_position = text_input.get_cursor_position();

        if (cursor_position < 0) {
            cursor_position = 0;
        }

        if (cursor_position > (int)display_text.length()) {
            cursor_position = (int)display_text.length();
        }

        int cursor_x = text_rect.left + measure_text_width(
            device_context,
            display_text.c_str(),
            cursor_position
        );

        HPEN caret_pen = CreatePen(
            PS_SOLID,
            1,
            GetSysColor(COLOR_WINDOWTEXT)
        );

        if (caret_pen != NULL) {
            HGDIOBJ previous_pen = SelectObject(device_context, caret_pen);

            MoveToEx(
                device_context,
                cursor_x,
                text_rect.top + 4,
                NULL
            );

            LineTo(
                device_context,
                cursor_x,
                text_rect.bottom - 4
            );

            if (previous_pen != NULL && previous_pen != HGDI_ERROR) {
                SelectObject(device_context, previous_pen);
            }

            DeleteObject(caret_pen);
        }
    }

    SetTextColor(device_context, old_text_color);

    if (saved_state != 0) {
        RestoreDC(device_context, saved_state);
    }
}
