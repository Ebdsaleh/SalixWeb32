// =================================================================================
// Filename:    engine/renderers/win32/Win32EmoticonPainter.cpp
// Author:      Ebdsaleh
// Description: Implements original classic-messenger-inspired GDI emoticons.
// =================================================================================

#include "Win32EmoticonPainter.h"

namespace {
    RECT make_square(const RECT& bounds) {
        RECT result = bounds;
        int width = bounds.right - bounds.left;
        int height = bounds.bottom - bounds.top;
        int extent = width < height ? width : height;
        if (extent < 1) {
            extent = 1;
        }

        int center_x = bounds.left + width / 2;
        int center_y = bounds.top + height / 2;
        result.left = center_x - extent / 2;
        result.top = center_y - extent / 2;
        result.right = result.left + extent;
        result.bottom = result.top + extent;
        return result;
    }

    RECT inset_rect(const RECT& rect, int amount) {
        RECT result = rect;
        result.left += amount;
        result.top += amount;
        result.right -= amount;
        result.bottom -= amount;
        return result;
    }

    void ellipse_fill(
        HDC dc,
        const RECT& rect,
        COLORREF fill,
        COLORREF outline
    ) {
        HBRUSH brush = CreateSolidBrush(fill);
        HPEN pen = CreatePen(PS_SOLID, 1, outline);
        HGDIOBJ old_brush = SelectObject(dc, brush);
        HGDIOBJ old_pen = SelectObject(dc, pen);

        Ellipse(dc, rect.left, rect.top, rect.right, rect.bottom);

        if (old_pen != NULL && old_pen != HGDI_ERROR) {
            SelectObject(dc, old_pen);
        }
        if (old_brush != NULL && old_brush != HGDI_ERROR) {
            SelectObject(dc, old_brush);
        }
        DeleteObject(pen);
        DeleteObject(brush);
    }

    void line(
        HDC dc,
        int x1,
        int y1,
        int x2,
        int y2,
        COLORREF color,
        int width
    ) {
        HPEN pen = CreatePen(PS_SOLID, width, color);
        HGDIOBJ old_pen = SelectObject(dc, pen);
        MoveToEx(dc, x1, y1, NULL);
        LineTo(dc, x2, y2);
        if (old_pen != NULL && old_pen != HGDI_ERROR) {
            SelectObject(dc, old_pen);
        }
        DeleteObject(pen);
    }

    void arc_line(
        HDC dc,
        const RECT& rect,
        int start_x,
        int start_y,
        int end_x,
        int end_y,
        COLORREF color,
        int width
    ) {
        HPEN pen = CreatePen(PS_SOLID, width, color);
        HGDIOBJ old_pen = SelectObject(dc, pen);
        HGDIOBJ old_brush = SelectObject(dc, GetStockObject(NULL_BRUSH));

        Arc(
            dc,
            rect.left,
            rect.top,
            rect.right,
            rect.bottom,
            start_x,
            start_y,
            end_x,
            end_y
        );

        if (old_brush != NULL && old_brush != HGDI_ERROR) {
            SelectObject(dc, old_brush);
        }
        if (old_pen != NULL && old_pen != HGDI_ERROR) {
            SelectObject(dc, old_pen);
        }
        DeleteObject(pen);
    }

    void face_base(HDC dc, const RECT& face) {
        ellipse_fill(dc, face, RGB(255, 211, 45), RGB(177, 127, 0));
        RECT inner = inset_rect(face, 2);
        ellipse_fill(dc, inner, RGB(255, 224, 73), RGB(242, 187, 25));

        int width = face.right - face.left;
        int height = face.bottom - face.top;
        RECT highlight;
        highlight.left = face.left + width * 20 / 100;
        highlight.top = face.top + height * 16 / 100;
        highlight.right = highlight.left + width * 24 / 100;
        highlight.bottom = highlight.top + height * 12 / 100;
        ellipse_fill(dc, highlight, RGB(255, 244, 172), RGB(255, 244, 172));
    }

    void eye(HDC dc, const RECT& face, int x_percent, int y_percent) {
        int width = face.right - face.left;
        int height = face.bottom - face.top;
        int size = width / 9;
        if (size < 2) {
            size = 2;
        }

        int center_x = face.left + width * x_percent / 100;
        int center_y = face.top + height * y_percent / 100;
        RECT rect;
        rect.left = center_x - size / 2;
        rect.top = center_y - size / 2;
        rect.right = rect.left + size;
        rect.bottom = rect.top + size + 1;
        ellipse_fill(dc, rect, RGB(55, 43, 22), RGB(55, 43, 22));
    }

    void smile(HDC dc, const RECT& face) {
        int width = face.right - face.left;
        int height = face.bottom - face.top;
        RECT mouth;
        mouth.left = face.left + width * 25 / 100;
        mouth.top = face.top + height * 38 / 100;
        mouth.right = face.right - width * 25 / 100;
        mouth.bottom = face.bottom - height * 16 / 100;
        int middle_y = mouth.top + (mouth.bottom - mouth.top) / 2;
        arc_line(
            dc,
            mouth,
            mouth.left,
            middle_y,
            mouth.right,
            middle_y,
            RGB(110, 55, 25),
            width >= 24 ? 2 : 1
        );
    }

    void smile_face(HDC dc, const RECT& bounds) {
        RECT face = make_square(bounds);
        face_base(dc, face);
        eye(dc, face, 35, 40);
        eye(dc, face, 65, 40);
        smile(dc, face);
    }

    void big_grin(HDC dc, const RECT& bounds) {
        RECT face = make_square(bounds);
        face_base(dc, face);
        eye(dc, face, 35, 38);
        eye(dc, face, 65, 38);

        int width = face.right - face.left;
        int height = face.bottom - face.top;
        RECT mouth;
        mouth.left = face.left + width * 24 / 100;
        mouth.top = face.top + height * 52 / 100;
        mouth.right = face.right - width * 24 / 100;
        mouth.bottom = face.bottom - height * 13 / 100;
        ellipse_fill(dc, mouth, RGB(107, 47, 25), RGB(107, 47, 25));

        RECT teeth = inset_rect(mouth, 2);
        teeth.bottom = teeth.top + (teeth.bottom - teeth.top) / 2 + 1;
        ellipse_fill(dc, teeth, RGB(255, 255, 246), RGB(255, 255, 246));
    }

    void wink(HDC dc, const RECT& bounds) {
        RECT face = make_square(bounds);
        face_base(dc, face);
        eye(dc, face, 66, 40);

        int width = face.right - face.left;
        int height = face.bottom - face.top;
        line(
            dc,
            face.left + width * 22 / 100,
            face.top + height * 40 / 100,
            face.left + width * 42 / 100,
            face.top + height * 40 / 100,
            RGB(55, 43, 22),
            width >= 24 ? 2 : 1
        );
        smile(dc, face);
    }

    void tongue(HDC dc, const RECT& bounds) {
        RECT face = make_square(bounds);
        face_base(dc, face);
        eye(dc, face, 35, 38);
        eye(dc, face, 65, 38);

        int width = face.right - face.left;
        int height = face.bottom - face.top;
        line(
            dc,
            face.left + width * 33 / 100,
            face.top + height * 60 / 100,
            face.right - width * 33 / 100,
            face.top + height * 60 / 100,
            RGB(104, 49, 26),
            width >= 24 ? 2 : 1
        );

        RECT tongue_rect;
        tongue_rect.left = face.left + width * 45 / 100;
        tongue_rect.top = face.top + height * 58 / 100;
        tongue_rect.right = face.left + width * 62 / 100;
        tongue_rect.bottom = face.top + height * 83 / 100;
        ellipse_fill(
            dc,
            tongue_rect,
            RGB(236, 104, 137),
            RGB(164, 63, 89)
        );
    }

    void crying(HDC dc, const RECT& bounds) {
        RECT face = make_square(bounds);
        face_base(dc, face);
        eye(dc, face, 35, 40);
        eye(dc, face, 65, 40);

        int width = face.right - face.left;
        int height = face.bottom - face.top;
        RECT mouth;
        mouth.left = face.left + width * 32 / 100;
        mouth.top = face.top + height * 62 / 100;
        mouth.right = face.right - width * 32 / 100;
        mouth.bottom = face.bottom - height * 8 / 100;
        arc_line(
            dc,
            mouth,
            mouth.right,
            mouth.bottom,
            mouth.left,
            mouth.bottom,
            RGB(110, 55, 25),
            width >= 24 ? 2 : 1
        );

        RECT tear;
        tear.left = face.right - width * 31 / 100;
        tear.top = face.top + height * 46 / 100;
        tear.right = tear.left + width * 10 / 100 + 1;
        tear.bottom = tear.top + height * 24 / 100;
        ellipse_fill(dc, tear, RGB(72, 168, 236), RGB(35, 116, 185));
    }

    void surprised(HDC dc, const RECT& bounds) {
        RECT face = make_square(bounds);
        face_base(dc, face);
        eye(dc, face, 35, 36);
        eye(dc, face, 65, 36);

        int width = face.right - face.left;
        int height = face.bottom - face.top;
        RECT mouth;
        mouth.left = face.left + width * 42 / 100;
        mouth.top = face.top + height * 55 / 100;
        mouth.right = face.left + width * 62 / 100;
        mouth.bottom = face.top + height * 80 / 100;
        ellipse_fill(dc, mouth, RGB(93, 46, 27), RGB(93, 46, 27));
    }

    void heart(HDC dc, const RECT& bounds) {
        RECT rect = make_square(bounds);
        int width = rect.right - rect.left;
        int height = rect.bottom - rect.top;

        HBRUSH brush = CreateSolidBrush(RGB(222, 43, 74));
        HPEN pen = CreatePen(PS_SOLID, 1, RGB(153, 24, 49));
        HGDIOBJ old_brush = SelectObject(dc, brush);
        HGDIOBJ old_pen = SelectObject(dc, pen);

        RECT left_lobe;
        left_lobe.left = rect.left + width * 10 / 100;
        left_lobe.top = rect.top + height * 16 / 100;
        left_lobe.right = rect.left + width * 55 / 100;
        left_lobe.bottom = rect.top + height * 58 / 100;
        RECT right_lobe;
        right_lobe.left = rect.left + width * 45 / 100;
        right_lobe.top = left_lobe.top;
        right_lobe.right = rect.right - width * 10 / 100;
        right_lobe.bottom = left_lobe.bottom;

        Ellipse(dc, left_lobe.left, left_lobe.top, left_lobe.right, left_lobe.bottom);
        Ellipse(dc, right_lobe.left, right_lobe.top, right_lobe.right, right_lobe.bottom);

        POINT points[3];
        points[0].x = rect.left + width * 12 / 100;
        points[0].y = rect.top + height * 42 / 100;
        points[1].x = rect.right - width * 12 / 100;
        points[1].y = points[0].y;
        points[2].x = rect.left + width / 2;
        points[2].y = rect.bottom - height * 10 / 100;
        Polygon(dc, points, 3);

        if (old_pen != NULL && old_pen != HGDI_ERROR) {
            SelectObject(dc, old_pen);
        }
        if (old_brush != NULL && old_brush != HGDI_ERROR) {
            SelectObject(dc, old_brush);
        }
        DeleteObject(pen);
        DeleteObject(brush);

        RECT shine;
        shine.left = rect.left + width * 24 / 100;
        shine.top = rect.top + height * 24 / 100;
        shine.right = shine.left + width * 14 / 100;
        shine.bottom = shine.top + height * 10 / 100;
        ellipse_fill(dc, shine, RGB(255, 162, 177), RGB(255, 162, 177));
    }

    void classic(HDC dc, const RECT& bounds) {
        RECT face = make_square(bounds);
        face_base(dc, face);
        int width = face.right - face.left;
        int height = face.bottom - face.top;

        line(
            dc,
            face.left + width * 22 / 100,
            face.top + height * 35 / 100,
            face.left + width * 42 / 100,
            face.top + height * 42 / 100,
            RGB(55, 43, 22),
            width >= 24 ? 2 : 1
        );
        eye(dc, face, 66, 40);
        line(
            dc,
            face.left + width * 34 / 100,
            face.top + height * 68 / 100,
            face.left + width * 68 / 100,
            face.top + height * 61 / 100,
            RGB(110, 55, 25),
            width >= 24 ? 2 : 1
        );
    }
}

void Win32EmoticonPainter::draw(
    HDC device_context,
    EmoticonRegistry::EmoticonId emoticon_id,
    const RECT& bounds
) {
    if (device_context == NULL) {
        return;
    }

    switch (emoticon_id) {
        case EmoticonRegistry::emoticon_smile:
            smile_face(device_context, bounds);
            break;
        case EmoticonRegistry::emoticon_big_grin:
            big_grin(device_context, bounds);
            break;
        case EmoticonRegistry::emoticon_wink:
            wink(device_context, bounds);
            break;
        case EmoticonRegistry::emoticon_tongue:
            tongue(device_context, bounds);
            break;
        case EmoticonRegistry::emoticon_crying:
            crying(device_context, bounds);
            break;
        case EmoticonRegistry::emoticon_surprised:
            surprised(device_context, bounds);
            break;
        case EmoticonRegistry::emoticon_heart:
            heart(device_context, bounds);
            break;
        case EmoticonRegistry::emoticon_classic:
            classic(device_context, bounds);
            break;
        case EmoticonRegistry::emoticon_none:
        default:
            break;
    }
}
