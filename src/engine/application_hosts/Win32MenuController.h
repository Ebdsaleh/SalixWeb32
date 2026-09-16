// =================================================================================
// Filename:    engine/application_hosts/Win32MenuController.h
// Author:      Ebdsaleh
// Description: Declares the native Win32 menu bar and generic command routing.
// =================================================================================
#pragma once

#include <windows.h>

class View;

class Win32MenuController {
    public:
        Win32MenuController();
        ~Win32MenuController();

        bool initialize(HWND window_handle, View* application_view);
        void shutdown();

    private:
        static LRESULT CALLBACK menu_window_proc(
            HWND window_handle,
            UINT message,
            WPARAM w_param,
            LPARAM l_param
        );

        bool create_menu_bar();
        bool handle_menu_command(int command_id);
        bool dispatch_application_command(int command_id);
        bool dispatch_edit_key(int key_code);

        HWND window_handle;
        View* application_view;
        HMENU menu_handle;
        WNDPROC previous_window_proc;
        bool is_initialized;
};
