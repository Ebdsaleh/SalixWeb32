// =================================================================================
// Filename:    engine/renderers/win32/Win32ComponentRenderer.cpp
// Author:      Ebdsaleh
// Description: Implements Win32 rendering for framework UI components.
// =================================================================================

#include "Win32ComponentRenderer.h"
#include "framework/Label.h"

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

void Win32ComponentRenderer::render_label(const Label& label) {
    if (device_context == NULL || !label.get_is_visible()) {
        return;
    }

    if (label.get_width() <= 0 || label.get_height() <= 0) {
        return;
    }

    RECT label_rect;
    label_rect.left = label.get_x();
    label_rect.top = label.get_y();
    label_rect.right = label.get_x() + label.get_width();
    label_rect.bottom = label.get_y() + label.get_height();

    UINT draw_flags = DT_VCENTER | DT_SINGLELINE;

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

    DrawTextA(
        device_context,
        label.get_text(),
        -1,
        &label_rect,
        draw_flags
    );
}
