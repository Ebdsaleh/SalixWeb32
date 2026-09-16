// =================================================================================
// Filename:    engine/application_hosts/Win32ApplicationHost.h
// Author:      Ebdsaleh
// Description: Declares the native Win32 application host.
// =================================================================================
#pragma once

#include <windows.h>

#ifndef WM_MOUSEWHEEL
#define WM_MOUSEWHEEL 0x020A
#endif

#include "engine/platform/win32/Win32NativeControlHost.h"

class ApplicationRuntime;
class View;

class Win32ApplicationHost {
    public:
        Win32ApplicationHost();

        bool initialize(
            HINSTANCE instance_handle,
            int show_command,
            ApplicationRuntime* application_runtime,
            View* application_view
        );

        int run();
        void shutdown();

    private:
        static LRESULT CALLBACK window_proc(
            HWND window_handle,
            UINT message,
            WPARAM w_param,
            LPARAM l_param
        );

        LRESULT handle_message(
            HWND window_handle,
            UINT message,
            WPARAM w_param,
            LPARAM l_param
        );

        void paint_window(HWND window_handle);
        void update_client_size(HWND window_handle);
        void layout_application_view();

        HINSTANCE instance_handle;
        HWND window_handle;
        ApplicationRuntime* application_runtime;
        View* application_view;
        Win32NativeControlHost native_control_host;
        int client_width;
        int client_height;
        bool is_initialized;
};
