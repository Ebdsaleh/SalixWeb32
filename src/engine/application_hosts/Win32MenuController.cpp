// =================================================================================
// Filename:    engine/application_hosts/Win32MenuController.cpp
// Author:      Ebdsaleh
// Description: Implements native Win32 menu bar creation and command routing.
// =================================================================================

#include "Win32MenuController.h"
#include "framework/ApplicationCommand.h"
#include "framework/UIEvent.h"
#include "framework/View.h"
#include "engine/platform/win32/Win32Clipboard.h"

namespace {
    const char* menu_controller_property = "SalixWeb32MenuController";
    const int control_character_z = 26;

    enum NativeMenuCommand {
        menu_file_attach = 43001,
        menu_file_exit,

        menu_edit_undo,
        menu_edit_cut,
        menu_edit_copy,
        menu_edit_paste,
        menu_edit_select_all,

        menu_options_conversation,
        menu_options_runtime,

        menu_help_about
    };

    void append_menu_item(
        HMENU menu,
        UINT command_id,
        const char* text
    ) {
        AppendMenuA(menu, MF_STRING, command_id, text);
    }
}

Win32MenuController::Win32MenuController()
    : window_handle(NULL),
      application_view(0),
      menu_handle(NULL),
      previous_window_proc(0),
      is_initialized(false) {
}

Win32MenuController::~Win32MenuController() {
    shutdown();
}

bool Win32MenuController::initialize(
    HWND new_window_handle,
    View* new_application_view
) {
    if (
        is_initialized ||
        new_window_handle == NULL ||
        new_application_view == 0
    ) {
        return false;
    }

    window_handle = new_window_handle;
    application_view = new_application_view;

    if (!create_menu_bar()) {
        window_handle = NULL;
        application_view = 0;
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
        menu_options_conversation,
        "&Conversation"
    );
    append_menu_item(
        options_menu,
        menu_options_runtime,
        "&Runtime Diagnostics"
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
