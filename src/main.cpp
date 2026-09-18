// =================================================================================
// Filename:    main.cpp
// Author:      Ebdsaleh
// Description: Native Win32 entry point for SalixWeb32.
// =================================================================================

#include <windows.h>
#include <stdio.h>
#include <string>

#include "runtime/ApplicationRuntime.h"
#include "runtime/Diagnostics.h"
#include "app/ApplicationSettings.h"
#include "app/StatusView.h"
#include "engine/application_hosts/Win32ApplicationHost.h"
#include "engine/application_hosts/Win32MenuController.h"
#include "engine/platform/win32/Win32DesktopServices.h"
#include "engine/platform/win32/Win32FileDialog.h"
#include "engine/platform/win32/Win32GraphicsRuntime.h"
#include "engine/platform/win32/Win32HttpTransport.h"
#include "engine/platform/win32/Win32NetworkRequestExecutor.h"
#include "web/backends/PlaceholderWebBackend.h"
#include "web/backends/RemoteBridgeWebBackend.h"
#include "web/platform/WebNavigationRequest.h"
#include "web/platform/WebPlatformBackend.h"
#include "web/platform/WebPlatformHost.h"

int APIENTRY WinMain(
    HINSTANCE instance_handle,
    HINSTANCE previous_instance_handle,
    LPSTR command_line,
    int show_command
) {
    ApplicationSettings settings;
    settings.load();

    ApplicationRuntime application_runtime;
    PlaceholderWebBackend placeholder_web_backend;
    Win32HttpTransport bridge_transport;
    Win32NetworkRequestExecutor bridge_request_executor(
        &bridge_transport
    );

    RemoteBridgeWebBackend remote_bridge_web_backend(
        &bridge_request_executor,
        settings.get_bridge_host(),
        settings.get_bridge_port()
    );

    bool use_remote_bridge = settings.get_use_remote_bridge();

    // Browser Probe asks the companion to perform a bounded modern HTTPS GET.
    // Keep the request explicit (Go button) and allow enough receive time for
    // that upstream fetch without making the normal placeholder path slower.
    if (use_remote_bridge) {
        bridge_transport.set_timeout_milliseconds(12000);
    }

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
            ? "Web backend selection: remote Browser Probe bridge."
            : "Web backend selection: placeholder."
    );

    if (settings.get_source_path()[0] != '\0') {
        std::string settings_message("Startup settings: ");
        settings_message += settings.get_source_path();
        Diagnostics::write_line(settings_message.c_str());
    }

    if (use_remote_bridge) {
        char endpoint_message[512];
        sprintf(
            endpoint_message,
            "Bridge endpoint: %s:%u",
            settings.get_bridge_host(),
            (unsigned int)settings.get_bridge_port()
        );
        Diagnostics::write_line(endpoint_message);
    }

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

    // Keep the historical placeholder proof initialized to the ChatGPT target,
    // but do not make a real remote fetch before the application window exists.
    // Browser Probe remote requests are user-triggered from the Browser/Web tab.
    if (!use_remote_bridge) {
        WebNavigationRequest initial_navigation("https://www.chatgpt.com/");
        if (!web_platform_host.navigate(initial_navigation)) {
            Diagnostics::write_line(
                "Initial placeholder WebView navigation was not accepted."
            );
        }
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
