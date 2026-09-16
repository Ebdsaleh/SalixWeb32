// =================================================================================
// Filename:    engine/platform/win32/Win32DesktopServices.cpp
// Author:      Ebdsaleh
// Description: Implements Win32 file opening, image thumbnails, and image preview.
// =================================================================================

#include <windows.h>
#include <shellapi.h>
#include <gdiplus.h>
#include <string>
#include <vector>

#include "Win32DesktopServices.h"
#include "framework/RasterImage.h"

// Some older Platform SDK header sets used with Visual C++ 7.1 do not expose
// WM_MOUSEWHEEL unless newer target-version macros are enabled. Keep the
// compatibility local to this Win32 implementation rather than raising the
// application's global WINVER/_WIN32_WINNT contract.
#ifndef WM_MOUSEWHEEL
#define WM_MOUSEWHEEL 0x020A
#endif

namespace {
    const char* preview_window_class_name = "SalixWeb32ImagePreviewWindow";
    const double preview_zoom_step = 1.25;
    const double preview_min_zoom = 0.25;
    const double preview_max_zoom = 8.0;

    struct PreviewWindowState {
        PreviewWindowState()
            : image(0),
              zoom_scale(1.0) {
        }

        ~PreviewWindowState() {
            delete image;
            image = 0;
        }

        Gdiplus::Image* image;
        double zoom_scale;
    };

    std::wstring to_wide_path(const char* path) {
        if (path == 0 || path[0] == '\0') {
            return std::wstring();
        }

        int length = MultiByteToWideChar(
            CP_ACP,
            0,
            path,
            -1,
            0,
            0
        );

        if (length <= 0) {
            return std::wstring();
        }

        std::vector<wchar_t> buffer((std::vector<wchar_t>::size_type)length);
        if (MultiByteToWideChar(
                CP_ACP,
                0,
                path,
                -1,
                &buffer[0],
                length
            ) <= 0) {
            return std::wstring();
        }

        return std::wstring(&buffer[0]);
    }

    void calculate_fit_size(
        int source_width,
        int source_height,
        int maximum_width,
        int maximum_height,
        int& output_width,
        int& output_height
    ) {
        output_width = source_width;
        output_height = source_height;

        if (
            source_width <= 0 ||
            source_height <= 0 ||
            maximum_width <= 0 ||
            maximum_height <= 0
        ) {
            output_width = 0;
            output_height = 0;
            return;
        }

        if (
            source_width <= maximum_width &&
            source_height <= maximum_height
        ) {
            return;
        }

        double horizontal_scale =
            (double)maximum_width / (double)source_width;
        double vertical_scale =
            (double)maximum_height / (double)source_height;
        double scale = horizontal_scale < vertical_scale
            ? horizontal_scale
            : vertical_scale;

        output_width = (int)((double)source_width * scale + 0.5);
        output_height = (int)((double)source_height * scale + 0.5);

        if (output_width < 1) {
            output_width = 1;
        }
        if (output_height < 1) {
            output_height = 1;
        }
    }

    void change_preview_zoom(
        PreviewWindowState* state,
        int wheel_delta
    ) {
        if (state == 0 || wheel_delta == 0) {
            return;
        }

        int wheel_steps = wheel_delta / 120;
        if (wheel_steps == 0) {
            wheel_steps = wheel_delta > 0 ? 1 : -1;
        }

        while (wheel_steps > 0) {
            state->zoom_scale *= preview_zoom_step;
            --wheel_steps;
        }

        while (wheel_steps < 0) {
            state->zoom_scale /= preview_zoom_step;
            ++wheel_steps;
        }

        if (state->zoom_scale < preview_min_zoom) {
            state->zoom_scale = preview_min_zoom;
        }

        if (state->zoom_scale > preview_max_zoom) {
            state->zoom_scale = preview_max_zoom;
        }
    }

    void draw_preview_image(
        HDC device_context,
        const RECT& client_rect,
        PreviewWindowState* state
    ) {
        if (
            device_context == NULL ||
            state == 0 ||
            state->image == 0
        ) {
            return;
        }

        int client_width = client_rect.right - client_rect.left;
        int client_height = client_rect.bottom - client_rect.top;
        int available_width = client_width - 24;
        int available_height = client_height - 24;

        int fitted_width = 0;
        int fitted_height = 0;
        calculate_fit_size(
            (int)state->image->GetWidth(),
            (int)state->image->GetHeight(),
            available_width,
            available_height,
            fitted_width,
            fitted_height
        );

        if (fitted_width <= 0 || fitted_height <= 0) {
            return;
        }

        int draw_width = (int)(
            ((double)fitted_width * state->zoom_scale) + 0.5
        );
        int draw_height = (int)(
            ((double)fitted_height * state->zoom_scale) + 0.5
        );

        if (draw_width < 1) {
            draw_width = 1;
        }
        if (draw_height < 1) {
            draw_height = 1;
        }

        int draw_x = (client_width - draw_width) / 2;
        int draw_y = (client_height - draw_height) / 2;

        Gdiplus::Graphics graphics(device_context);
        graphics.SetInterpolationMode(
            Gdiplus::InterpolationModeHighQualityBicubic
        );
        graphics.DrawImage(
            state->image,
            draw_x,
            draw_y,
            draw_width,
            draw_height
        );
    }

    LRESULT CALLBACK preview_window_proc(
        HWND window_handle,
        UINT message,
        WPARAM w_param,
        LPARAM l_param
    ) {
        PreviewWindowState* state = (PreviewWindowState*)GetWindowLongA(
            window_handle,
            GWL_USERDATA
        );

        if (message == WM_NCCREATE) {
            CREATESTRUCTA* create_struct = (CREATESTRUCTA*)l_param;
            state = (PreviewWindowState*)create_struct->lpCreateParams;
            SetWindowLongA(
                window_handle,
                GWL_USERDATA,
                (LONG)state
            );
        }

        switch (message) {
            case WM_ERASEBKGND:
                return 1;

            case WM_PAINT: {
                PAINTSTRUCT paint_struct;
                HDC device_context = BeginPaint(window_handle, &paint_struct);
                RECT client_rect;
                GetClientRect(window_handle, &client_rect);
                FillRect(
                    device_context,
                    &client_rect,
                    GetSysColorBrush(COLOR_WINDOW)
                );

                draw_preview_image(
                    device_context,
                    client_rect,
                    state
                );

                EndPaint(window_handle, &paint_struct);
                return 0;
            }

            case WM_MOUSEWHEEL:
                if (state != 0) {
                    change_preview_zoom(
                        state,
                        (int)(short)HIWORD(w_param)
                    );
                    InvalidateRect(window_handle, NULL, FALSE);
                }
                return 0;

            case WM_SIZE:
                InvalidateRect(window_handle, NULL, FALSE);
                return 0;

            case WM_CLOSE:
                DestroyWindow(window_handle);
                return 0;

            case WM_NCDESTROY:
                SetWindowLongA(window_handle, GWL_USERDATA, 0);
                delete state;
                return DefWindowProcA(
                    window_handle,
                    message,
                    w_param,
                    l_param
                );
        }

        return DefWindowProcA(window_handle, message, w_param, l_param);
    }

    bool ensure_preview_window_class(HINSTANCE instance_handle) {
        WNDCLASSEXA existing_class;
        ZeroMemory(&existing_class, sizeof(existing_class));
        existing_class.cbSize = sizeof(existing_class);

        if (GetClassInfoExA(
                instance_handle,
                preview_window_class_name,
                &existing_class
            )) {
            return true;
        }

        WNDCLASSEXA window_class;
        ZeroMemory(&window_class, sizeof(window_class));
        window_class.cbSize = sizeof(window_class);
        window_class.style = CS_HREDRAW | CS_VREDRAW;
        window_class.lpfnWndProc = preview_window_proc;
        window_class.hInstance = instance_handle;
        window_class.hIcon = LoadIcon(NULL, IDI_APPLICATION);
        window_class.hCursor = LoadCursor(NULL, IDC_ARROW);
        window_class.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        window_class.lpszClassName = preview_window_class_name;
        window_class.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

        return RegisterClassExA(&window_class) != 0;
    }
}

Win32DesktopServices::Win32DesktopServices(HINSTANCE new_instance_handle)
    : instance_handle(new_instance_handle) {
}

bool Win32DesktopServices::load_image_thumbnail(
    const char* path,
    int maximum_width,
    int maximum_height,
    RasterImage& image
) {
    image.clear();

    std::wstring wide_path = to_wide_path(path);
    if (wide_path.empty()) {
        return false;
    }

    Gdiplus::Bitmap source(wide_path.c_str());
    if (source.GetLastStatus() != Gdiplus::Ok) {
        return false;
    }

    int target_width = 0;
    int target_height = 0;
    calculate_fit_size(
        (int)source.GetWidth(),
        (int)source.GetHeight(),
        maximum_width,
        maximum_height,
        target_width,
        target_height
    );

    if (target_width <= 0 || target_height <= 0) {
        return false;
    }

    Gdiplus::Bitmap scaled(
        target_width,
        target_height,
        PixelFormat32bppARGB
    );
    if (scaled.GetLastStatus() != Gdiplus::Ok) {
        return false;
    }

    {
        Gdiplus::Graphics graphics(&scaled);
        graphics.SetInterpolationMode(
            Gdiplus::InterpolationModeHighQualityBicubic
        );
        graphics.DrawImage(
            &source,
            0,
            0,
            target_width,
            target_height
        );
    }

    if (!image.allocate(target_width, target_height)) {
        return false;
    }

    Gdiplus::Rect lock_rect(0, 0, target_width, target_height);
    Gdiplus::BitmapData bitmap_data;
    ZeroMemory(&bitmap_data, sizeof(bitmap_data));

    if (scaled.LockBits(
            &lock_rect,
            Gdiplus::ImageLockModeRead,
            PixelFormat32bppARGB,
            &bitmap_data
        ) != Gdiplus::Ok) {
        image.clear();
        return false;
    }

    unsigned char* destination = image.get_pixels();
    unsigned char* source_bytes = (unsigned char*)bitmap_data.Scan0;
    int source_stride = bitmap_data.Stride;
    int destination_stride = image.get_stride();

    for (int row = 0; row < target_height; ++row) {
        unsigned char* source_row = source_stride >= 0
            ? source_bytes + (row * source_stride)
            : source_bytes + ((target_height - 1 - row) * (-source_stride));
        unsigned char* destination_row =
            destination + (row * destination_stride);

        CopyMemory(
            destination_row,
            source_row,
            (SIZE_T)destination_stride
        );
    }

    scaled.UnlockBits(&bitmap_data);
    return true;
}

bool Win32DesktopServices::open_file(const char* path) {
    if (path == 0 || path[0] == '\0') {
        return false;
    }

    HINSTANCE result = ShellExecuteA(
        NULL,
        "open",
        path,
        NULL,
        NULL,
        SW_SHOWNORMAL
    );

    return (INT_PTR)result > 32;
}

bool Win32DesktopServices::preview_image(
    const char* path,
    const char* title
) {
    if (
        instance_handle == NULL ||
        path == 0 ||
        path[0] == '\0' ||
        !ensure_preview_window_class(instance_handle)
    ) {
        return false;
    }

    std::wstring wide_path = to_wide_path(path);
    if (wide_path.empty()) {
        return false;
    }

    PreviewWindowState* state = new PreviewWindowState();
    if (state == 0) {
        return false;
    }

    state->image = new Gdiplus::Image(wide_path.c_str());
    if (
        state->image == 0 ||
        state->image->GetLastStatus() != Gdiplus::Ok
    ) {
        delete state;
        return false;
    }

    HWND preview_window = CreateWindowExA(
        WS_EX_APPWINDOW,
        preview_window_class_name,
        title == 0 || title[0] == '\0' ? "Image Preview" : title,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        640,
        520,
        NULL,
        NULL,
        instance_handle,
        state
    );

    if (preview_window == NULL) {
        delete state;
        return false;
    }

    ShowWindow(preview_window, SW_SHOWNORMAL);
    UpdateWindow(preview_window);
    return true;
}
