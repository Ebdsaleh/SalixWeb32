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

int APIENTRY WinMain(
    HINSTANCE instance_handle,
    HINSTANCE previous_instance_handle,
    LPSTR command_line,
    int show_command
) {
    ApplicationRuntime application_runtime;
    StatusView status_view(&application_runtime);
    Win32ApplicationHost application_host;

    (void)previous_instance_handle;
    (void)command_line;

    Diagnostics::write_line("SalixWeb32 starting.");

    if (!application_runtime.initialize()) {
        MessageBoxA(
            NULL,
            "Failed to initialize the SalixWeb32 runtime.",
            "SalixWeb32",
            MB_OK | MB_ICONERROR
        );
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
        return 1;
    }

    int exit_code = application_host.run();

    application_host.shutdown();
    application_runtime.shutdown();

    Diagnostics::write_line("SalixWeb32 stopped.");
    return exit_code;
}
