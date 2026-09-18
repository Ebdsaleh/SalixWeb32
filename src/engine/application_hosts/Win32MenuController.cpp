// =================================================================================
// Filename:    engine/application_hosts/Win32MenuController.cpp
// Author:      Ebdsaleh
// Description: Implements native Win32 menu bar creation and command routing.
// =================================================================================

#include <shellapi.h>
#include <string>

#include "Win32MenuController.h"
#include "Win32DiagnosticCapture.h"
#include "Win32SettingsDialog.h"
#include "app/ApplicationSettings.h"
#include "framework/ApplicationCommand.h"
#include "framework/UIEvent.h"
#include "framework/View.h"
#include "engine/platform/win32/Win32Clipboard.h"

namespace {
    const char* menu_controller_property = "SalixWeb32MenuController";
    const char* diagnostic_dialog_class = "SalixWeb32DiagnosticCaptureDialog";
    const int control_character_z = 26;
    const int diagnostic_dialog_ok = 44101;
    const int diagnostic_dialog_open_folder = 44102;

    enum NativeMenuCommand {
        menu_file_attach = 43001,
        menu_file_exit,

        menu_edit_undo,
        menu_edit_cut,
        menu_edit_copy,
        menu_edit_paste,
        menu_edit_select_all,

        menu_options_settings,
        menu_options_conversation,
        menu_options_runtime,
        menu_options_browser_diagnostic_report,
        menu_options_diagnostic_screenshot,

        menu_help_about
    };

    struct DiagnosticDialogState {
        std::string diagnostics_directory;
        std::string dialog_title;
    };

    void append_menu_item(
        HMENU menu,
        UINT command_id,
        const char* text
    ) {
        AppendMenuA(menu, MF_STRING, command_id, text);
    }

    std::string get_parent_directory(const std::string& path) {
        std::string::size_type separator = path.find_last_of("\\/");
        if (separator == std::string::npos) {
            return std::string();
        }

        return path.substr(0, separator);
    }

    void set_default_gui_font(HWND control) {
        if (control == NULL) {
            return;
        }

        HFONT font = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
        if (font != NULL) {
            SendMessageA(control, WM_SETFONT, (WPARAM)font, TRUE);
        }
    }

    LRESULT CALLBACK diagnostic_dialog_window_proc(
        HWND dialog_handle,
        UINT message,
        WPARAM w_param,
        LPARAM l_param
    ) {
        DiagnosticDialogState* state =
            (DiagnosticDialogState*)GetWindowLongA(
                dialog_handle,
                GWL_USERDATA
            );

        if (message == WM_NCCREATE) {
            CREATESTRUCTA* create_struct = (CREATESTRUCTA*)l_param;
            SetWindowLongA(
                dialog_handle,
                GWL_USERDATA,
                (LONG)create_struct->lpCreateParams
            );
            return TRUE;
        }

        if (message == WM_COMMAND) {
            int command_id = (int)LOWORD(w_param);

            if (command_id == diagnostic_dialog_ok) {
                DestroyWindow(dialog_handle);
                return 0;
            }

            if (
                command_id == diagnostic_dialog_open_folder &&
                state != 0 &&
                !state->diagnostics_directory.empty()
            ) {
                HINSTANCE result = ShellExecuteA(
                    dialog_handle,
                    "open",
                    state->diagnostics_directory.c_str(),
                    NULL,
                    NULL,
                    SW_SHOWNORMAL
                );

                if ((INT_PTR)result <= 32) {
                    MessageBoxA(
                        dialog_handle,
                        "Windows could not open the diagnostics folder.",
                        state->dialog_title.empty()
                            ? "SalixWeb32 Diagnostics"
                            : state->dialog_title.c_str(),
                        MB_OK | MB_ICONERROR
                    );
                } else {
                    DestroyWindow(dialog_handle);
                }
                return 0;
            }
        }

        if (message == WM_CLOSE) {
            DestroyWindow(dialog_handle);
            return 0;
        }

        return DefWindowProcA(
            dialog_handle,
            message,
            w_param,
            l_param
        );
    }

    bool register_diagnostic_dialog_class(HINSTANCE instance_handle) {
        WNDCLASSA window_class;
        ZeroMemory(&window_class, sizeof(window_class));
        window_class.style = CS_HREDRAW | CS_VREDRAW;
        window_class.lpfnWndProc = diagnostic_dialog_window_proc;
        window_class.hInstance = instance_handle;
        window_class.hIcon = LoadIconA(NULL, IDI_INFORMATION);
        window_class.hCursor = LoadCursorA(NULL, IDC_ARROW);
        window_class.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
        window_class.lpszClassName = diagnostic_dialog_class;

        if (RegisterClassA(&window_class) != 0) {
            return true;
        }

        return GetLastError() == ERROR_CLASS_ALREADY_EXISTS;
    }

    void center_window_over_owner(HWND dialog_handle, HWND owner_handle) {
        RECT dialog_rect;
        RECT owner_rect;

        if (
            dialog_handle == NULL ||
            !GetWindowRect(dialog_handle, &dialog_rect)
        ) {
            return;
        }

        int width = dialog_rect.right - dialog_rect.left;
        int height = dialog_rect.bottom - dialog_rect.top;
        int x = (GetSystemMetrics(SM_CXSCREEN) - width) / 2;
        int y = (GetSystemMetrics(SM_CYSCREEN) - height) / 2;

        if (
            owner_handle != NULL &&
            IsWindow(owner_handle) &&
            GetWindowRect(owner_handle, &owner_rect)
        ) {
            x = owner_rect.left +
                ((owner_rect.right - owner_rect.left - width) / 2);
            y = owner_rect.top +
                ((owner_rect.bottom - owner_rect.top - height) / 2);
        }

        if (x < 0) {
            x = 0;
        }
        if (y < 0) {
            y = 0;
        }

        SetWindowPos(
            dialog_handle,
            HWND_TOP,
            x,
            y,
            0,
            0,
            SWP_NOSIZE | SWP_NOACTIVATE
        );
    }

    void show_diagnostic_file_success(
        HWND owner_handle,
        const char* dialog_title,
        const std::string& message,
        const std::string& file_path
    ) {
        HINSTANCE instance_handle = NULL;
        if (owner_handle != NULL) {
            instance_handle = (HINSTANCE)GetWindowLongA(
                owner_handle,
                GWL_HINSTANCE
            );
        }

        if (
            instance_handle == NULL ||
            !register_diagnostic_dialog_class(instance_handle)
        ) {
            MessageBoxA(
                owner_handle,
                message.c_str(),
                dialog_title,
                MB_OK | MB_ICONINFORMATION
            );
            return;
        }

        DiagnosticDialogState state;
        state.diagnostics_directory = get_parent_directory(file_path);
        state.dialog_title = dialog_title == 0
            ? "SalixWeb32 Diagnostics"
            : dialog_title;

        HWND dialog_handle = CreateWindowExA(
            WS_EX_DLGMODALFRAME,
            diagnostic_dialog_class,
            dialog_title,
            WS_POPUP | WS_CAPTION | WS_SYSMENU,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            650,
            220,
            owner_handle,
            NULL,
            instance_handle,
            &state
        );

        if (dialog_handle == NULL) {
            MessageBoxA(
                owner_handle,
                message.c_str(),
                dialog_title,
                MB_OK | MB_ICONINFORMATION
            );
            return;
        }

        HWND icon_control = CreateWindowExA(
            0,
            "STATIC",
            "",
            WS_CHILD | WS_VISIBLE | SS_ICON,
            18,
            20,
            34,
            34,
            dialog_handle,
            NULL,
            instance_handle,
            NULL
        );

        if (icon_control != NULL) {
            HICON information_icon = LoadIconA(NULL, IDI_INFORMATION);
            SendMessageA(
                icon_control,
                STM_SETICON,
                (WPARAM)information_icon,
                0
            );
        }

        HWND message_control = CreateWindowExA(
            0,
            "STATIC",
            message.c_str(),
            WS_CHILD | WS_VISIBLE | SS_LEFT | SS_NOPREFIX,
            66,
            18,
            558,
            124,
            dialog_handle,
            NULL,
            instance_handle,
            NULL
        );

        HWND open_button = CreateWindowExA(
            0,
            "BUTTON",
            "Go to Files",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
            424,
            154,
            105,
            27,
            dialog_handle,
            (HMENU)diagnostic_dialog_open_folder,
            instance_handle,
            NULL
        );

        HWND ok_button = CreateWindowExA(
            0,
            "BUTTON",
            "OK",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
            540,
            154,
            84,
            27,
            dialog_handle,
            (HMENU)diagnostic_dialog_ok,
            instance_handle,
            NULL
        );

        set_default_gui_font(message_control);
        set_default_gui_font(open_button);
        set_default_gui_font(ok_button);

        center_window_over_owner(dialog_handle, owner_handle);

        if (owner_handle != NULL && IsWindow(owner_handle)) {
            EnableWindow(owner_handle, FALSE);
        }

        ShowWindow(dialog_handle, SW_SHOW);
        UpdateWindow(dialog_handle);
        SetForegroundWindow(dialog_handle);

        MSG message_data;
        bool repost_quit = false;
        int quit_code = 0;

        while (IsWindow(dialog_handle)) {
            BOOL get_result = GetMessageA(
                &message_data,
                NULL,
                0,
                0
            );

            if (get_result <= 0) {
                if (get_result == 0) {
                    repost_quit = true;
                    quit_code = (int)message_data.wParam;
                }
                break;
            }

            if (!IsDialogMessageA(dialog_handle, &message_data)) {
                TranslateMessage(&message_data);
                DispatchMessageA(&message_data);
            }
        }

        if (owner_handle != NULL && IsWindow(owner_handle)) {
            EnableWindow(owner_handle, TRUE);
            SetForegroundWindow(owner_handle);
        }

        if (repost_quit) {
            PostQuitMessage(quit_code);
        }
    }

    void show_diagnostic_capture_success(
        HWND owner_handle,
        const std::string& screenshot_path,
        const std::string& report_path
    ) {
        std::string message(
            "Diagnostic capture saved successfully.\r\n\r\nScreenshot: "
        );
        message += screenshot_path;
        message += "\r\nReport: ";
        message += report_path;

        show_diagnostic_file_success(
            owner_handle,
            "SalixWeb32 Diagnostic Capture",
            message,
            report_path
        );
    }

    void show_browser_report_success(
        HWND owner_handle,
        const std::string& report_path
    ) {
        std::string message(
            "Browser diagnostic report saved successfully.\r\n\r\nReport: "
        );
        message += report_path;

        show_diagnostic_file_success(
            owner_handle,
            "SalixWeb32 Browser Diagnostic Report",
            message,
            report_path
        );
    }
}

Win32MenuController::Win32MenuController()
    : window_handle(NULL),
      application_view(0),
      application_settings(0),
      menu_handle(NULL),
      previous_window_proc(0),
      is_initialized(false) {
}

Win32MenuController::~Win32MenuController() {
    shutdown();
}

bool Win32MenuController::initialize(
    HWND new_window_handle,
    View* new_application_view,
    ApplicationSettings* new_application_settings
) {
    if (
        is_initialized ||
        new_window_handle == NULL ||
        new_application_view == 0 ||
        new_application_settings == 0
    ) {
        return false;
    }

    window_handle = new_window_handle;
    application_view = new_application_view;
    application_settings = new_application_settings;

    if (!create_menu_bar()) {
        window_handle = NULL;
        application_view = 0;
        application_settings = 0;
        return false;
    }

    if (!SetPropA(
            window_handle,
            menu_controller_property,
            (HANDLE)this
        )) {
        SetMenu(window_handle, NULL);
        DestroyMenu(menu_handle);
        menu_handle = NULL;
        window_handle = NULL;
        application_view = 0;
        application_settings = 0;
        return false;
    }

    previous_window_proc = (WNDPROC)SetWindowLongA(
        window_handle,
        GWL_WNDPROC,
        (LONG)Win32MenuController::menu_window_proc
    );

    if (previous_window_proc == 0) {
        RemovePropA(window_handle, menu_controller_property);
        SetMenu(window_handle, NULL);
        DestroyMenu(menu_handle);
        menu_handle = NULL;
        window_handle = NULL;
        application_view = 0;
        application_settings = 0;
        return false;
    }

    DrawMenuBar(window_handle);
    is_initialized = true;
    return true;
}

void Win32MenuController::shutdown() {
    if (!is_initialized) {
        return;
    }

    if (window_handle != NULL && IsWindow(window_handle)) {
        if (previous_window_proc != 0) {
            SetWindowLongA(
                window_handle,
                GWL_WNDPROC,
                (LONG)previous_window_proc
            );
        }

        RemovePropA(window_handle, menu_controller_property);
        SetMenu(window_handle, NULL);
        DrawMenuBar(window_handle);
    }

    if (menu_handle != NULL) {
        DestroyMenu(menu_handle);
    }

    menu_handle = NULL;
    previous_window_proc = 0;
    application_view = 0;
    application_settings = 0;
    window_handle = NULL;
    is_initialized = false;
}

bool Win32MenuController::create_menu_bar() {
    menu_handle = CreateMenu();
    HMENU file_menu = CreatePopupMenu();
    HMENU edit_menu = CreatePopupMenu();
    HMENU options_menu = CreatePopupMenu();
    HMENU help_menu = CreatePopupMenu();

    if (
        menu_handle == NULL ||
        file_menu == NULL ||
        edit_menu == NULL ||
        options_menu == NULL ||
        help_menu == NULL
    ) {
        if (menu_handle != NULL) {
            DestroyMenu(menu_handle);
        }
        if (file_menu != NULL) {
            DestroyMenu(file_menu);
        }
        if (edit_menu != NULL) {
            DestroyMenu(edit_menu);
        }
        if (options_menu != NULL) {
            DestroyMenu(options_menu);
        }
        if (help_menu != NULL) {
            DestroyMenu(help_menu);
        }
        menu_handle = NULL;
        return false;
    }

    append_menu_item(file_menu, menu_file_attach, "&Attach File...\tCtrl+O");
    AppendMenuA(file_menu, MF_SEPARATOR, 0, NULL);
    append_menu_item(file_menu, menu_file_exit, "E&xit");

    append_menu_item(edit_menu, menu_edit_undo, "&Undo\tCtrl+Z");
    AppendMenuA(edit_menu, MF_SEPARATOR, 0, NULL);
    append_menu_item(edit_menu, menu_edit_cut, "Cu&t\tCtrl+X");
    append_menu_item(edit_menu, menu_edit_copy, "&Copy\tCtrl+C");
    append_menu_item(edit_menu, menu_edit_paste, "&Paste\tCtrl+V");
    AppendMenuA(edit_menu, MF_SEPARATOR, 0, NULL);
    append_menu_item(edit_menu, menu_edit_select_all, "Select &All\tCtrl+A");

    append_menu_item(
        options_menu,
        menu_options_settings,
        "&Settings..."
    );
    AppendMenuA(options_menu, MF_SEPARATOR, 0, NULL);
    append_menu_item(
        options_menu,
        menu_options_conversation,
        "&Conversation"
    );
    append_menu_item(
        options_menu,
        menu_options_runtime,
        "&Runtime Diagnostics"
    );
    AppendMenuA(options_menu, MF_SEPARATOR, 0, NULL);
    append_menu_item(
        options_menu,
        menu_options_browser_diagnostic_report,
        "Export &Browser Diagnostic Report..."
    );
    append_menu_item(
        options_menu,
        menu_options_diagnostic_screenshot,
        "Take &Diagnostic Screenshot"
    );

    append_menu_item(help_menu, menu_help_about, "&About SalixWeb32");

    AppendMenuA(menu_handle, MF_POPUP, (UINT_PTR)file_menu, "&File");
    AppendMenuA(menu_handle, MF_POPUP, (UINT_PTR)edit_menu, "&Edit");
    AppendMenuA(menu_handle, MF_POPUP, (UINT_PTR)options_menu, "&Options");
    AppendMenuA(menu_handle, MF_POPUP, (UINT_PTR)help_menu, "&Help");

    if (!SetMenu(window_handle, menu_handle)) {
        DestroyMenu(menu_handle);
        menu_handle = NULL;
        return false;
    }

    return true;
}

LRESULT CALLBACK Win32MenuController::menu_window_proc(
    HWND current_window_handle,
    UINT message,
    WPARAM w_param,
    LPARAM l_param
) {
    Win32MenuController* controller =
        (Win32MenuController*)GetPropA(
            current_window_handle,
            menu_controller_property
        );

    if (
        controller != 0 &&
        message == WM_KEYDOWN &&
        (int)w_param == 'O' &&
        (GetKeyState(VK_CONTROL) & 0x8000) != 0 &&
        controller->handle_menu_command(menu_file_attach)
    ) {
        InvalidateRect(current_window_handle, NULL, FALSE);
        return 0;
    }

    if (
        controller != 0 &&
        message == WM_COMMAND &&
        l_param == 0 &&
        HIWORD(w_param) == 0 &&
        controller->handle_menu_command((int)LOWORD(w_param))
    ) {
        InvalidateRect(current_window_handle, NULL, FALSE);
        return 0;
    }

    if (controller != 0 && controller->previous_window_proc != 0) {
        return CallWindowProcA(
            controller->previous_window_proc,
            current_window_handle,
            message,
            w_param,
            l_param
        );
    }

    return DefWindowProcA(
        current_window_handle,
        message,
        w_param,
        l_param
    );
}

bool Win32MenuController::handle_menu_command(int command_id) {
    switch (command_id) {
        case menu_file_attach:
            dispatch_application_command(application_command_attach_file);
            return true;

        case menu_file_exit:
            SendMessageA(window_handle, WM_CLOSE, 0, 0);
            return true;

        case menu_edit_undo: {
            if (application_view != 0) {
                UIEvent event(UIEvent::event_character);
                event.character_code = control_character_z;
                event.control_down = true;
                application_view->handle_event(event);
            }
            return true;
        }

        case menu_edit_cut:
            dispatch_edit_key(UIEvent::key_x);
            return true;

        case menu_edit_copy:
            dispatch_edit_key(UIEvent::key_c);
            return true;

        case menu_edit_paste:
            dispatch_edit_key(UIEvent::key_v);
            return true;

        case menu_edit_select_all:
            dispatch_edit_key(UIEvent::key_a);
            return true;

        case menu_options_settings:
            if (
                application_settings != 0 &&
                !Win32SettingsDialog::show(
                    window_handle,
                    application_settings
                )
            ) {
                // Cancel is a normal no-op.  The dialog itself reports
                // validation/persistence failures before it closes.
            }
            return true;

        case menu_options_conversation:
            dispatch_application_command(
                application_command_show_conversation
            );
            return true;

        case menu_options_runtime:
            dispatch_application_command(
                application_command_show_runtime
            );
            return true;

        case menu_options_browser_diagnostic_report: {
            std::string report_path;
            std::string error_text;

            if (Win32DiagnosticCapture::export_browser_report(
                    application_view,
                    application_settings == 0
                        ? 0
                        : application_settings->get_diagnostics_directory(),
                    report_path,
                    error_text
                )) {
                show_browser_report_success(
                    window_handle,
                    report_path
                );
            } else {
                std::string message(
                    "Browser diagnostic report export failed."
                );
                if (!error_text.empty()) {
                    message += "\r\n\r\n";
                    message += error_text;
                }

                MessageBoxA(
                    window_handle,
                    message.c_str(),
                    "SalixWeb32 Browser Diagnostic Report",
                    MB_OK | MB_ICONERROR
                );
            }
            return true;
        }

        case menu_options_diagnostic_screenshot: {
            std::string screenshot_path;
            std::string report_path;
            std::string error_text;

            if (Win32DiagnosticCapture::capture(
                    window_handle,
                    application_view,
                    application_settings == 0
                        ? 0
                        : application_settings->get_diagnostics_directory(),
                    screenshot_path,
                    report_path,
                    error_text
                )) {
                show_diagnostic_capture_success(
                    window_handle,
                    screenshot_path,
                    report_path
                );
            } else {
                std::string message("Diagnostic capture failed.");
                if (!error_text.empty()) {
                    message += "\r\n\r\n";
                    message += error_text;
                }

                MessageBoxA(
                    window_handle,
                    message.c_str(),
                    "SalixWeb32 Diagnostic Capture",
                    MB_OK | MB_ICONERROR
                );
            }
            return true;
        }

        case menu_help_about:
            MessageBoxA(
                window_handle,
                "SalixWeb32\r\n\r\nNative legacy-web application framework\r\nby Ebdsaleh",
                "About SalixWeb32",
                MB_OK | MB_ICONINFORMATION
            );
            return true;
    }

    return false;
}

bool Win32MenuController::dispatch_application_command(int command_id) {
    if (application_view == 0) {
        return false;
    }

    UIEvent event(UIEvent::event_command);
    event.command_id = command_id;
    return application_view->handle_event(event);
}

bool Win32MenuController::dispatch_edit_key(int key_code) {
    if (application_view == 0 || window_handle == NULL) {
        return false;
    }

    UIEvent event(UIEvent::event_key_down);
    event.key_code = (UIEvent::KeyCode)key_code;
    event.control_down = true;

    Win32Clipboard clipboard(window_handle);
    event.clipboard = &clipboard;

    return application_view->handle_event(event);
}
