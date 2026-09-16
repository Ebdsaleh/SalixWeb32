// =================================================================================
// Filename:    main.cpp
// Author:      Ebdsaleh
// Description: Native Win32 entry point for SalixWeb32.
// =================================================================================

#include <windows.h>

#include "runtime/ApplicationRuntime.h"
#include "runtime/Diagnostics.h"
#include "app/StatusView.h"
#include "engine/application_hosts/Win32ApplicationHost.h"
#include "engine/application_hosts/Win32MenuController.h"
#include "engine/platform/win32/Win32DesktopServices.h"
#include "engine/platform/win32/Win32FileDialog.h"
#include "engine/platform/win32/Win32GraphicsRuntime.h"

int APIENTRY WinMain(
    HINSTANCE instance_handle,
    HINSTANCE previous_instance_handle,
    LPSTR command_line,
    int show_command
) {
    ApplicationRuntime application_runtime;
    Win32GraphicsRuntime graphics_runtime;
    Win32FileDialog file_dialog;
    Win32DesktopServices desktop_services(instance_handle);
    StatusView status_view(
        &application_runtime,
        &file_dialog,
        &desktop_services
    );
    Win32ApplicationHost application_host;
    Win32MenuController menu_controller;

    (void)previous_instance_handle;
    (void)command_line;

    Diagnostics::write_line("SalixWeb32 starting.");

    if (!graphics_runtime.initialize()) {
        MessageBoxA(
            NULL,
            "Failed to initialize the Win32 image runtime.",
            "SalixWeb32",
            MB_OK | MB_ICONERROR
        );
        return 1;
    }

    if (!application_runtime.initialize()) {
        MessageBoxA(
            NULL,
            "Failed to initialize the SalixWeb32 runtime.",
            "SalixWeb32",
            MB_OK | MB_ICONERROR
        );
        graphics_runtime.shutdown();
        return 1;
    }

    if (!application_host.initialize(
            instance_handle,
            show_command,
            &application_runtime,
            &status_view
        )) {
        MessageBoxA(
            NULL,
            "Failed to initialize the Win32 application host.",
            "SalixWeb32",
            MB_OK | MB_ICONERROR
        );
        application_runtime.shutdown();
        graphics_runtime.shutdown();
        return 1;
    }

    if (!menu_controller.initialize(
            application_host.get_window_handle(),
            &status_view
        )) {
        MessageBoxA(
            application_host.get_window_handle(),
            "Failed to initialize the native application menu.",
            "SalixWeb32",
            MB_OK | MB_ICONERROR
        );
        application_host.shutdown();
        application_runtime.shutdown();
        graphics_runtime.shutdown();
        return 1;
    }

    int exit_code = application_host.run();

    menu_controller.shutdown();
    application_host.shutdown();
    application_runtime.shutdown();
    graphics_runtime.shutdown();

    Diagnostics::write_line("SalixWeb32 stopped.");
    return exit_code;
}
