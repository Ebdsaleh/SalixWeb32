// =================================================================================
// Filename:    main.cpp
// Author:      Ebdsaleh
// Description: Native Win32 entry point for SalixWeb32.
// =================================================================================

#include <windows.h>
#include <stdlib.h>
#include <string.h>

#include "runtime/ApplicationRuntime.h"
#include "runtime/Diagnostics.h"
#include "app/StatusView.h"
#include "engine/application_hosts/Win32ApplicationHost.h"
#include "engine/application_hosts/Win32MenuController.h"
#include "engine/platform/win32/Win32DesktopServices.h"
#include "engine/platform/win32/Win32FileDialog.h"
#include "engine/platform/win32/Win32GraphicsRuntime.h"
#include "engine/platform/win32/Win32HttpTransport.h"
#include "web/backends/PlaceholderWebBackend.h"
#include "web/backends/RemoteBridgeWebBackend.h"
#include "web/platform/WebNavigationRequest.h"
#include "web/platform/WebPlatformBackend.h"
#include "web/platform/WebPlatformHost.h"

namespace {
    unsigned short get_bridge_port() {
        const char* port_text = getenv("SALIX_BRIDGE_PORT");
        if (port_text == 0 || port_text[0] == '\0') {
            return 8765;
        }

        int port = atoi(port_text);
        if (port < 1 || port > 65535) {
            return 8765;
        }

        return (unsigned short)port;
    }
}

int APIENTRY WinMain(
    HINSTANCE instance_handle,
    HINSTANCE previous_instance_handle,
    LPSTR command_line,
    int show_command
) {
    ApplicationRuntime application_runtime;
    PlaceholderWebBackend placeholder_web_backend;
    Win32HttpTransport bridge_transport;

    const char* bridge_host = getenv("SALIX_BRIDGE_HOST");
    if (bridge_host == 0 || bridge_host[0] == '\0') {
        bridge_host = "127.0.0.1";
    }

    RemoteBridgeWebBackend remote_bridge_web_backend(
        &bridge_transport,
        bridge_host,
        get_bridge_port()
    );

    const char* backend_mode = getenv("SALIX_WEB_BACKEND");
    bool use_remote_bridge =
        backend_mode != 0 && strcmp(backend_mode, "remote") == 0;

    WebPlatformBackend* selected_web_backend = use_remote_bridge
        ? (WebPlatformBackend*)&remote_bridge_web_backend
        : (WebPlatformBackend*)&placeholder_web_backend;

    WebPlatformHost web_platform_host;
    Win32GraphicsRuntime graphics_runtime;
    Win32FileDialog file_dialog;
    Win32DesktopServices desktop_services(instance_handle);

    web_platform_host.set_backend(selected_web_backend);
    application_runtime.set_web_platform_host(&web_platform_host);

    StatusView status_view(
        &application_runtime,
        &file_dialog,
        &desktop_services,
        &web_platform_host
    );
    Win32ApplicationHost application_host;
    Win32MenuController menu_controller;

    (void)previous_instance_handle;
    (void)command_line;

    Diagnostics::write_line("SalixWeb32 starting.");
    Diagnostics::write_line(
        use_remote_bridge
            ? "Web backend selection: remote bridge."
            : "Web backend selection: placeholder."
    );

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

    WebNavigationRequest initial_navigation("https://www.chatgpt.com/");
    if (!web_platform_host.navigate(initial_navigation)) {
        Diagnostics::write_line(
            use_remote_bridge
                ? "Initial remote bridge navigation was not accepted."
                : "Initial placeholder WebView navigation was not accepted."
        );
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
