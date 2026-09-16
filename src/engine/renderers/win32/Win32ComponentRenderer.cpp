// =================================================================================
// Filename:    engine/renderers/win32/Win32ComponentRenderer.cpp
// Author:      Ebdsaleh
// Description: Implements Win32 rendering for framework UI components.
// =================================================================================

#include "Win32ComponentRenderer.h"
#include "Win32EmoticonPainter.h"
#include "Win32TextPainter.h"
#include "Win32ScrollableTextPainter.h"
#include "framework/Panel.h"
#include "framework/Label.h"
#include "framework/Button.h"
#include "framework/TextInput.h"
#include "framework/ImageView.h"
#include "framework/RasterImage.h"
#include "framework/EmoticonRegistry.h"
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

    void frame_rect(
        HDC device_context,
        RECT rect,
        const Color& color,
        int border_width
    ) {
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

void Win32ComponentRenderer::push_clip_rect(
    int x,
    int y,
    int width,
    int height
) {
    if (device_context == NULL) {
        return;
    }

    if (width < 0) {
        width = 0;
    }
    if (height < 0) {
        height = 0;
    }

    SaveDC(device_context);
    IntersectClipRect(
        device_context,
        x,
        y,
        x + width,
        y + height
    );
}

void Win32ComponentRenderer::pop_clip_rect() {
    if (device_context == NULL) {
        return;
    }

    RestoreDC(device_context, -1);
}

void Win32ComponentRenderer::render_panel(const Panel& panel) {
    if (
        device_context == NULL ||
        !panel.get_is_visible() ||
        panel.get_width() <= 0 ||
        panel.get_height() <= 0
    ) {
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
    Win32TextPainter::render_label(device_context, label);
}

void Win32ComponentRenderer::render_button(const Button& button) {
    if (
        device_context == NULL ||
        !button.get_is_visible() ||
        button.get_width() <= 0 ||
        button.get_height() <= 0
    ) {
        return;
    }

    RECT button_rect = component_rect(button);
    Color background_color = button.get_style().background_color;

    if (!button.get_is_enabled()) {
        background_color = adjust_color(background_color, 8);
    } else if (button.get_is_pressed()) {
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

    const char* button_text = button.get_text();
    EmoticonRegistry::EmoticonId emoticon_id;

    if (
        button_text != 0 &&
        EmoticonRegistry::find_exact_alias(button_text, emoticon_id)
    ) {
        RECT icon_rect = button_rect;
        int width = icon_rect.right - icon_rect.left;
        int height = icon_rect.bottom - icon_rect.top;
        int extent = width < height ? width : height;

        extent -= 2;
        if (extent < 10) {
            extent = 10;
        }

        int center_x = button_rect.left + width / 2;
        int center_y = button_rect.top + height / 2;

        icon_rect.left = center_x - extent / 2;
        icon_rect.top = center_y - extent / 2;
        icon_rect.right = icon_rect.left + extent;
        icon_rect.bottom = icon_rect.top + extent;

        Win32EmoticonPainter::draw(
            device_context,
            emoticon_id,
            icon_rect
        );
        return;
    }

    Color text_color = button.get_style().foreground_color;
    if (!button.get_is_enabled()) {
        text_color = Color(145, 145, 145);
    }

    COLORREF old_text_color = SetTextColor(
        device_context,
        to_color_ref(text_color)
    );

    DrawTextA(
        device_context,
        button_text == 0 ? "" : button_text,
        -1,
        &button_rect,
        DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX
    );

    SetTextColor(device_context, old_text_color);
}

void Win32ComponentRenderer::render_text_input(const TextInput& text_input) {
    Win32ScrollableTextPainter::render_text_input(
        device_context,
        text_input
    );
}

void Win32ComponentRenderer::render_image_view(const ImageView& image_view) {
    if (
        device_context == NULL ||
        !image_view.get_is_visible() ||
        image_view.get_width() <= 0 ||
        image_view.get_height() <= 0
    ) {
        return;
    }

    RECT image_rect = component_rect(image_view);
    fill_rect(
        device_context,
        image_rect,
        image_view.get_style().background_color
    );

    const RasterImage& image = image_view.get_image();
    const unsigned char* pixels = image.get_pixels();

    if (!image.empty() && pixels != 0) {
        BITMAPINFO bitmap_info;
        ZeroMemory(&bitmap_info, sizeof(bitmap_info));
        bitmap_info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bitmap_info.bmiHeader.biWidth = image.get_width();
        bitmap_info.bmiHeader.biHeight = -image.get_height();
        bitmap_info.bmiHeader.biPlanes = 1;
        bitmap_info.bmiHeader.biBitCount = 32;
        bitmap_info.bmiHeader.biCompression = BI_RGB;

        int old_stretch_mode = SetStretchBltMode(device_context, HALFTONE);
        SetBrushOrgEx(device_context, 0, 0, NULL);

        StretchDIBits(
            device_context,
            image_view.get_x(),
            image_view.get_y(),
            image_view.get_width(),
            image_view.get_height(),
            0,
            0,
            image.get_width(),
            image.get_height(),
            pixels,
            &bitmap_info,
            DIB_RGB_COLORS,
            SRCCOPY
        );

        SetStretchBltMode(device_context, old_stretch_mode);
    }

    frame_rect(
        device_context,
        image_rect,
        image_view.get_style().border_color,
        image_view.get_style().border_width
    );
}
