// =================================================================================
// Filename:    engine/application_hosts/Win32ApplicationHost.cpp
// Author:      Ebdsaleh
// Description: Implements the native Win32 window and message loop.
// =================================================================================

#include "Win32ApplicationHost.h"
#include "runtime/ApplicationRuntime.h"
#include "runtime/Diagnostics.h"
#include "framework/View.h"
#include "framework/UIEvent.h"
#include "engine/platform/win32/Win32Clipboard.h"
#include "engine/platform/win32/Win32TextMetrics.h"
#include "engine/renderers/win32/Win32ComponentRenderer.h"

namespace {
    const char* window_class_name = "SalixWeb32WindowClass";
    const UINT runtime_timer_id = 1;
    const UINT runtime_timer_interval_ms = 16;

    struct LeftClickSequence {
        LeftClickSequence()
            : last_click_time(0),
              last_x(0),
              last_y(0),
              click_count(0) {
        }

        DWORD last_click_time;
        int last_x;
        int last_y;
        int click_count;
    };

    int absolute_distance(int first_value, int second_value) {
        int difference = first_value - second_value;
        return difference < 0 ? -difference : difference;
    }

    int get_left_click_count(int x, int y) {
        static LeftClickSequence sequence;

        DWORD current_time = GetTickCount();
        DWORD elapsed_time = current_time - sequence.last_click_time;
        UINT double_click_time = GetDoubleClickTime();

        int horizontal_tolerance = GetSystemMetrics(SM_CXDOUBLECLK) / 2;
        int vertical_tolerance = GetSystemMetrics(SM_CYDOUBLECLK) / 2;

        if (horizontal_tolerance < 1) {
            horizontal_tolerance = 1;
        }

        if (vertical_tolerance < 1) {
            vertical_tolerance = 1;
        }

        bool continues_sequence =
            sequence.last_click_time != 0 &&
            elapsed_time <= double_click_time &&
            absolute_distance(x, sequence.last_x) <= horizontal_tolerance &&
            absolute_distance(y, sequence.last_y) <= vertical_tolerance;

        if (continues_sequence) {
            ++sequence.click_count;

            if (sequence.click_count > 3) {
                sequence.click_count = 1;
            }
        } else {
            sequence.click_count = 1;
        }

        sequence.last_click_time = current_time;
        sequence.last_x = x;
        sequence.last_y = y;

        return sequence.click_count;
    }

    UIEvent::KeyCode translate_key_code(WPARAM key_code) {
        switch ((int)key_code) {
            case VK_LEFT:
                return UIEvent::key_left;

            case VK_RIGHT:
                return UIEvent::key_right;

            case VK_UP:
                return UIEvent::key_up;

            case VK_DOWN:
                return UIEvent::key_down;

            case VK_HOME:
                return UIEvent::key_home;

            case VK_END:
                return UIEvent::key_end;

            case VK_DELETE:
                return UIEvent::key_delete;

            case VK_BACK:
                return UIEvent::key_backspace;

            case VK_RETURN:
                return UIEvent::key_enter;

            case VK_TAB:
                return UIEvent::key_tab;

            case VK_ESCAPE:
                return UIEvent::key_escape;

            case 'A':
                return UIEvent::key_a;

            case 'C':
                return UIEvent::key_c;

            case 'V':
                return UIEvent::key_v;

            case 'X':
                return UIEvent::key_x;

            case VK_OEM_1:
                return UIEvent::key_semicolon;
        }

        return UIEvent::key_none;
    }

    void populate_modifier_state(UIEvent& event) {
        event.shift_down = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
        event.control_down = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
        event.alt_down = (GetKeyState(VK_MENU) & 0x8000) != 0;
    }
}

Win32ApplicationHost::Win32ApplicationHost()
    : instance_handle(NULL),
      window_handle(NULL),
      application_runtime(0),
      application_view(0),
      client_width(0),
      client_height(0),
      is_initialized(false) {
}

bool Win32ApplicationHost::initialize(
    HINSTANCE new_instance_handle,
    int show_command,
    ApplicationRuntime* new_application_runtime,
    View* new_application_view
) {
    if (
        is_initialized ||
        new_instance_handle == NULL ||
        new_application_runtime == 0 ||
        new_application_view == 0
    ) {
        return false;
    }

    instance_handle = new_instance_handle;
    application_runtime = new_application_runtime;
    application_view = new_application_view;

    WNDCLASSEXA window_class;
    ZeroMemory(&window_class, sizeof(window_class));

    window_class.cbSize = sizeof(window_class);
    window_class.style = CS_HREDRAW | CS_VREDRAW;
    window_class.lpfnWndProc = Win32ApplicationHost::window_proc;
    window_class.hInstance = instance_handle;
    window_class.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    window_class.hCursor = LoadCursor(NULL, IDC_ARROW);
    window_class.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    window_class.lpszClassName = window_class_name;
    window_class.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

    if (RegisterClassExA(&window_class) == 0) {
        Diagnostics::write_line("Win32ApplicationHost: RegisterClassExA failed.");
        return false;
    }

    window_handle = CreateWindowExA(
        0,
        window_class_name,
        "SalixWeb32",
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        800,
        600,
        NULL,
        NULL,
        instance_handle,
        this
    );

    if (window_handle == NULL) {
        Diagnostics::write_line("Win32ApplicationHost: CreateWindowExA failed.");
        UnregisterClassA(window_class_name, instance_handle);
        return false;
    }

    if (!native_control_host.initialize(window_handle, instance_handle)) {
        Diagnostics::write_line(
            "Win32ApplicationHost: native control host initialization failed."
        );
        DestroyWindow(window_handle);
        window_handle = NULL;
        UnregisterClassA(window_class_name, instance_handle);
        return false;
    }

    application_view->attach_native_control_host(&native_control_host);

    update_client_size(window_handle);
    layout_application_view();

    if (SetTimer(
            window_handle,
            runtime_timer_id,
            runtime_timer_interval_ms,
            NULL
        ) == 0) {
        Diagnostics::write_line("Win32ApplicationHost: SetTimer failed.");
        application_view->detach_native_control_host();
        native_control_host.shutdown();
        DestroyWindow(window_handle);
        window_handle = NULL;
        UnregisterClassA(window_class_name, instance_handle);
        return false;
    }

    ShowWindow(window_handle, show_command);
    UpdateWindow(window_handle);

    is_initialized = true;
    Diagnostics::write_line("Win32ApplicationHost initialized.");
    return true;
}

int Win32ApplicationHost::run() {
    if (!is_initialized) {
        return 1;
    }

    MSG message;
    int get_message_result;

    while ((get_message_result = GetMessageA(&message, NULL, 0, 0)) > 0) {
        TranslateMessage(&message);
        DispatchMessageA(&message);
    }

    if (get_message_result == -1) {
        Diagnostics::write_line("Win32ApplicationHost: GetMessageA failed.");
        return 1;
    }

    return (int)message.wParam;
}

void Win32ApplicationHost::shutdown() {
    if (!is_initialized) {
        return;
    }

    if (application_view != 0) {
        application_view->detach_native_control_host();
    }

    native_control_host.shutdown();

    if (window_handle != NULL && IsWindow(window_handle)) {
        KillTimer(window_handle, runtime_timer_id);
        DestroyWindow(window_handle);
    }

    window_handle = NULL;
    client_width = 0;
    client_height = 0;

    UnregisterClassA(window_class_name, instance_handle);

    application_view = 0;
    application_runtime = 0;
    instance_handle = NULL;
    is_initialized = false;

    Diagnostics::write_line("Win32ApplicationHost shut down.");
}

LRESULT CALLBACK Win32ApplicationHost::window_proc(
    HWND current_window_handle,
    UINT message,
    WPARAM w_param,
    LPARAM l_param
) {
    Win32ApplicationHost* application_host = 0;

    if (message == WM_NCCREATE) {
        CREATESTRUCTA* create_struct = (CREATESTRUCTA*)l_param;
        application_host = (Win32ApplicationHost*)create_struct->lpCreateParams;

        SetWindowLongA(
            current_window_handle,
            GWL_USERDATA,
            (LONG)application_host
        );

        application_host->window_handle = current_window_handle;
    } else {
        application_host = (Win32ApplicationHost*)GetWindowLongA(
            current_window_handle,
            GWL_USERDATA
        );
    }

    if (application_host != 0) {
        return application_host->handle_message(
            current_window_handle,
            message,
            w_param,
            l_param
        );
    }

    return DefWindowProcA(current_window_handle, message, w_param, l_param);
}

LRESULT Win32ApplicationHost::handle_message(
    HWND current_window_handle,
    UINT message,
    WPARAM w_param,
    LPARAM l_param
) {
    switch (message) {
        case WM_TIMER:
            if (w_param == runtime_timer_id && application_runtime != 0) {
                application_runtime->update();

                if ((application_runtime->get_update_count() % 60UL) == 0UL) {
                    InvalidateRect(current_window_handle, NULL, FALSE);
                }
            }
            return 0;

        case WM_SIZE:
            client_width = (int)LOWORD(l_param);
            client_height = (int)HIWORD(l_param);
            layout_application_view();
            InvalidateRect(current_window_handle, NULL, FALSE);
            return 0;

        case WM_COMMAND:
            if (native_control_host.handle_command(w_param, l_param)) {
                InvalidateRect(current_window_handle, NULL, FALSE);
                return 0;
            }
            break;

        case WM_HSCROLL:
        case WM_VSCROLL:
            if (native_control_host.handle_scroll(message, w_param, l_param)) {
                InvalidateRect(current_window_handle, NULL, FALSE);
                return 0;
            }
            break;

        case WM_MOUSEMOVE: {
            UIEvent event(UIEvent::event_mouse_move);
            event.x = (int)(short)LOWORD(l_param);
            event.y = (int)(short)HIWORD(l_param);
            event.left_button_down = (w_param & MK_LBUTTON) != 0;
            populate_modifier_state(event);

            HDC metrics_context = GetDC(current_window_handle);
            Win32TextMetrics text_metrics(metrics_context);
            event.text_metrics = metrics_context != NULL ? &text_metrics : 0;

            bool was_handled = application_view != 0 &&
                application_view->handle_event(event);

            if (metrics_context != NULL) {
                ReleaseDC(current_window_handle, metrics_context);
            }

            if (was_handled) {
                InvalidateRect(current_window_handle, NULL, FALSE);
            }
            return 0;
        }

        case WM_LBUTTONDOWN: {
            SetFocus(current_window_handle);
            SetCapture(current_window_handle);

            UIEvent event(UIEvent::event_mouse_down);
            event.x = (int)(short)LOWORD(l_param);
            event.y = (int)(short)HIWORD(l_param);
            event.left_button_down = true;
            event.click_count = get_left_click_count(event.x, event.y);
            populate_modifier_state(event);

            HDC metrics_context = GetDC(current_window_handle);
            Win32TextMetrics text_metrics(metrics_context);
            event.text_metrics = metrics_context != NULL ? &text_metrics : 0;

            bool was_handled = application_view != 0 &&
                application_view->handle_event(event);

            if (metrics_context != NULL) {
                ReleaseDC(current_window_handle, metrics_context);
            }

            if (was_handled) {
                InvalidateRect(current_window_handle, NULL, FALSE);
            }
            return 0;
        }

        case WM_LBUTTONUP: {
            UIEvent event(UIEvent::event_mouse_up);
            event.x = (int)(short)LOWORD(l_param);
            event.y = (int)(short)HIWORD(l_param);
            event.left_button_down = false;
            populate_modifier_state(event);

            HDC metrics_context = GetDC(current_window_handle);
            Win32TextMetrics text_metrics(metrics_context);
            event.text_metrics = metrics_context != NULL ? &text_metrics : 0;

            bool was_handled = application_view != 0 &&
                application_view->handle_event(event);

            if (metrics_context != NULL) {
                ReleaseDC(current_window_handle, metrics_context);
            }

            if (was_handled) {
                InvalidateRect(current_window_handle, NULL, FALSE);
            }

            if (GetCapture() == current_window_handle) {
                ReleaseCapture();
            }
            return 0;
        }

        case WM_KEYDOWN: {
            UIEvent event(UIEvent::event_key_down);
            event.key_code = translate_key_code(w_param);
            populate_modifier_state(event);

            Win32Clipboard clipboard(current_window_handle);
            event.clipboard = &clipboard;

            if (application_view != 0 && application_view->handle_event(event)) {
                InvalidateRect(current_window_handle, NULL, FALSE);
            }
            return 0;
        }

        case WM_KEYUP: {
            UIEvent event(UIEvent::event_key_up);
            event.key_code = translate_key_code(w_param);
            populate_modifier_state(event);

            if (application_view != 0 && application_view->handle_event(event)) {
                InvalidateRect(current_window_handle, NULL, FALSE);
            }
            return 0;
        }

        case WM_CHAR: {
            UIEvent event(UIEvent::event_character);
            event.character_code = (int)w_param;
            populate_modifier_state(event);

            if (application_view != 0 && application_view->handle_event(event)) {
                InvalidateRect(current_window_handle, NULL, FALSE);
            }
            return 0;
        }

        case WM_ERASEBKGND:
            return 1;

        case WM_PAINT:
            paint_window(current_window_handle);
            return 0;

        case WM_CLOSE:
            Diagnostics::write_line("Win32ApplicationHost: close requested.");
            DestroyWindow(current_window_handle);
            return 0;

        case WM_DESTROY:
            Diagnostics::write_line("Win32ApplicationHost: window destroyed.");
            KillTimer(current_window_handle, runtime_timer_id);
            window_handle = NULL;
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProcA(current_window_handle, message, w_param, l_param);
}

void Win32ApplicationHost::paint_window(HWND current_window_handle) {
    PAINTSTRUCT paint_struct;
    HDC device_context = BeginPaint(current_window_handle, &paint_struct);

    RECT client_rect;
    GetClientRect(current_window_handle, &client_rect);

    int paint_width = client_rect.right - client_rect.left;
    int paint_height = client_rect.bottom - client_rect.top;

    if (paint_width <= 0 || paint_height <= 0) {
        EndPaint(current_window_handle, &paint_struct);
        return;
    }

    HDC back_buffer_context = CreateCompatibleDC(device_context);
    HBITMAP back_buffer_bitmap = NULL;
    HGDIOBJ previous_bitmap = NULL;

    if (back_buffer_context != NULL) {
        back_buffer_bitmap = CreateCompatibleBitmap(
            device_context,
            paint_width,
            paint_height
        );
    }

    if (back_buffer_context != NULL && back_buffer_bitmap != NULL) {
        previous_bitmap = SelectObject(
            back_buffer_context,
            back_buffer_bitmap
        );

        FillRect(
            back_buffer_context,
            &client_rect,
            GetSysColorBrush(COLOR_WINDOW)
        );

        if (application_view != 0) {
            Win32ComponentRenderer component_renderer(back_buffer_context);
            application_view->render(component_renderer);
        }

        BitBlt(
            device_context,
            0,
            0,
            paint_width,
            paint_height,
            back_buffer_context,
            0,
            0,
            SRCCOPY
        );

        if (previous_bitmap != NULL && previous_bitmap != HGDI_ERROR) {
            SelectObject(back_buffer_context, previous_bitmap);
        }
    } else {
        FillRect(
            device_context,
            &client_rect,
            GetSysColorBrush(COLOR_WINDOW)
        );

        if (application_view != 0) {
            Win32ComponentRenderer component_renderer(device_context);
            application_view->render(component_renderer);
        }
    }

    if (back_buffer_bitmap != NULL) {
        DeleteObject(back_buffer_bitmap);
    }

    if (back_buffer_context != NULL) {
        DeleteDC(back_buffer_context);
    }

    EndPaint(current_window_handle, &paint_struct);
}

void Win32ApplicationHost::update_client_size(HWND current_window_handle) {
    RECT client_rect;

    if (!GetClientRect(current_window_handle, &client_rect)) {
        client_width = 0;
        client_height = 0;
        return;
    }

    client_width = client_rect.right - client_rect.left;
    client_height = client_rect.bottom - client_rect.top;
}

void Win32ApplicationHost::layout_application_view() {
    if (application_view == 0) {
        return;
    }

    application_view->layout(client_width, client_height);
}
