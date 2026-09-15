// =================================================================================
// Filename:    engine/application_hosts/Win32ApplicationHost.cpp
// Author:      Ebdsaleh
// Description: Implements the native Win32 window and message loop.
// =================================================================================

#include "Win32ApplicationHost.h"
#include "../../runtime/ApplicationRuntime.h"
#include "../../runtime/Diagnostics.h"

namespace {
    const char* window_class_name = "SalixWeb32WindowClass";
    const UINT runtime_timer_id = 1;
    const UINT runtime_timer_interval_ms = 16;
}

Win32ApplicationHost::Win32ApplicationHost()
    : instance_handle(NULL),
      window_handle(NULL),
      application_runtime(0),
      is_initialized(false) {
}

bool Win32ApplicationHost::initialize(
    HINSTANCE new_instance_handle,
    int show_command,
    ApplicationRuntime* new_application_runtime
) {
    if (is_initialized || new_instance_handle == NULL || new_application_runtime == 0) {
        return false;
    }

    instance_handle = new_instance_handle;
    application_runtime = new_application_runtime;

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
        WS_OVERLAPPEDWINDOW,
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

    if (SetTimer(
            window_handle,
            runtime_timer_id,
            runtime_timer_interval_ms,
            NULL
        ) == 0) {
        Diagnostics::write_line("Win32ApplicationHost: SetTimer failed.");
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

    if (window_handle != NULL && IsWindow(window_handle)) {
        KillTimer(window_handle, runtime_timer_id);
        DestroyWindow(window_handle);
    }

    window_handle = NULL;
    UnregisterClassA(window_class_name, instance_handle);

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
            }
            return 0;

        case WM_PAINT:
            paint_window(current_window_handle);
            return 0;

        case WM_CLOSE:
            DestroyWindow(current_window_handle);
            return 0;

        case WM_DESTROY:
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

    HFONT gui_font = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    HFONT old_font = (HFONT)SelectObject(device_context, gui_font);

    SetBkMode(device_context, TRANSPARENT);

    RECT title_rect = client_rect;
    title_rect.bottom = client_rect.bottom / 2;
    title_rect.top = title_rect.bottom - 50;

    DrawTextA(
        device_context,
        "SalixWeb32 runtime operational",
        -1,
        &title_rect,
        DT_CENTER | DT_VCENTER | DT_SINGLELINE
    );

    RECT backend_rect = client_rect;
    backend_rect.top = client_rect.bottom / 2;
    backend_rect.bottom = backend_rect.top + 30;

    DrawTextA(
        device_context,
        "Win32 application host operational",
        -1,
        &backend_rect,
        DT_CENTER | DT_VCENTER | DT_SINGLELINE
    );

    RECT web_rect = client_rect;
    web_rect.top = backend_rect.bottom;
    web_rect.bottom = web_rect.top + 30;

    DrawTextA(
        device_context,
        "Web platform backend: not loaded",
        -1,
        &web_rect,
        DT_CENTER | DT_VCENTER | DT_SINGLELINE
    );

    SelectObject(device_context, old_font);
    EndPaint(current_window_handle, &paint_struct);
}
