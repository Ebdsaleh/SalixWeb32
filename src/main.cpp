// =================================================================================
// Filename:    main.cpp
// Author:      Ebdsaleh
// Description: Native Win32 entry point for SalixWeb32.
// =================================================================================

#include <windows.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <string>

#include "runtime/ApplicationRuntime.h"
#include "runtime/Diagnostics.h"
#include "app/ApplicationSettings.h"
#include "app/StatusView.h"
#include "conversation/ConversationServiceBackend.h"
#include "conversation/ConversationServiceHost.h"
#include "conversation/backends/PlaceholderConversationBackend.h"
#include "conversation/backends/RemoteConversationBackend.h"
#include "engine/application_hosts/Win32ApplicationHost.h"
#include "engine/application_hosts/Win32MenuController.h"
#include "engine/platform/win32/Win32ApplicationPaths.h"
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

namespace {
    bool command_line_has_flag(
        const char* command_line,
        const char* expected_flag
    ) {
        if (
            command_line == 0 ||
            expected_flag == 0 ||
            expected_flag[0] == '\0'
        ) {
            return false;
        }

        size_t expected_length = strlen(expected_flag);
        const char* cursor = command_line;

        while (*cursor != '\0') {
            while (
                *cursor != '\0' &&
                isspace((unsigned char)*cursor)
            ) {
                ++cursor;
            }

            if (*cursor == '\0') {
                break;
            }

            bool quoted = false;
            if (*cursor == '"') {
                quoted = true;
                ++cursor;
            }

            const char* token_start = cursor;

            while (
                *cursor != '\0' &&
                (
                    quoted
                        ? *cursor != '"'
                        : !isspace((unsigned char)*cursor)
                )
            ) {
                ++cursor;
            }

            size_t token_length =
                (size_t)(cursor - token_start);

            if (
                token_length == expected_length &&
                strncmp(
                    token_start,
                    expected_flag,
                    expected_length
                ) == 0
            ) {
                return true;
            }

            if (quoted && *cursor == '"') {
                ++cursor;
            }
        }

        return false;
    }
}

int APIENTRY WinMain(
    HINSTANCE instance_handle,
    HINSTANCE previous_instance_handle,
    LPSTR command_line,
    int show_command
) {
    std::string launch_directory;
    if (
        !Win32ApplicationPaths::get_launch_directory(
            launch_directory
        )
    ) {
        MessageBoxA(
            NULL,
            "Failed to resolve the SalixWeb32 launch directory.",
            "SalixWeb32",
            MB_OK | MB_ICONERROR
        );
        return 1;
    }

    std::string executable_directory;
    if (
        !Win32ApplicationPaths::get_executable_directory(
            executable_directory
        )
    ) {
        MessageBoxA(
            NULL,
            "Failed to resolve the SalixWeb32 executable directory.",
            "SalixWeb32",
            MB_OK | MB_ICONERROR
        );
        return 1;
    }

    bool portable_mode =
        command_line_has_flag(
            command_line,
            "--portable"
        );

    std::string user_data_directory;
    if (
        !Win32ApplicationPaths::get_user_data_directory(
            portable_mode,
            executable_directory.c_str(),
            user_data_directory
        )
    ) {
        MessageBoxA(
            NULL,
            portable_mode
                ? "Portable mode could not use the executable directory for writable SalixWeb32 state."
                : "Standard mode could not create or access %APPDATA%\\SalixWeb32.",
            "SalixWeb32",
            MB_OK | MB_ICONERROR
        );
        return 1;
    }

    std::string user_preferences_path;
    if (
        !Win32ApplicationPaths::get_user_preferences_file(
            user_data_directory.c_str(),
            user_preferences_path
        )
    ) {
        MessageBoxA(
            NULL,
            "Failed to resolve the SalixWeb32 settings file.",
            "SalixWeb32",
            MB_OK | MB_ICONERROR
        );
        return 1;
    }

    ApplicationSettings settings;
    settings.set_path_context(
        launch_directory.c_str(),
        executable_directory.c_str(),
        user_data_directory.c_str(),
        user_preferences_path.c_str(),
        portable_mode
    );
    settings.load();

    ApplicationRuntime application_runtime;
    PlaceholderConversationBackend placeholder_conversation_backend;
    ConversationServiceHost conversation_service_host;
    PlaceholderWebBackend placeholder_web_backend;

    Win32HttpTransport bridge_transport;
    Win32NetworkRequestExecutor bridge_request_executor(
        &bridge_transport
    );

    Win32HttpTransport conversation_bridge_transport;
    Win32NetworkRequestExecutor conversation_bridge_request_executor(
        &conversation_bridge_transport
    );

    RemoteBridgeWebBackend remote_bridge_web_backend(
        &bridge_request_executor,
        settings.get_bridge_host(),
        settings.get_bridge_port()
    );

    RemoteConversationBackend remote_conversation_backend(
        &conversation_bridge_request_executor,
        settings.get_bridge_host(),
        settings.get_bridge_port()
    );

    bool use_remote_bridge = settings.get_use_remote_bridge();

    // Browser Probe asks the companion to perform a bounded modern HTTPS GET.
    // Keep the request explicit (Go button) and allow enough receive time for
    // that upstream fetch without making the normal placeholder path slower.
    if (use_remote_bridge) {
        bridge_transport.set_timeout_milliseconds(12000);
        conversation_bridge_transport.set_timeout_milliseconds(5000);
    }

    WebPlatformBackend* selected_web_backend = use_remote_bridge
        ? (WebPlatformBackend*)&remote_bridge_web_backend
        : (WebPlatformBackend*)&placeholder_web_backend;

    ConversationServiceBackend* selected_conversation_backend =
        use_remote_bridge
            ? (ConversationServiceBackend*)&remote_conversation_backend
            : (ConversationServiceBackend*)&placeholder_conversation_backend;

    WebPlatformHost web_platform_host;
    Win32GraphicsRuntime graphics_runtime;
    Win32FileDialog file_dialog;
    file_dialog.set_initial_directory(
        settings.get_attachment_directory()
    );

    Win32DesktopServices desktop_services(instance_handle);

    web_platform_host.set_backend(selected_web_backend);
    conversation_service_host.set_backend(
        selected_conversation_backend
    );
    application_runtime.set_web_platform_host(&web_platform_host);
    application_runtime.set_conversation_service_host(
        &conversation_service_host
    );

    StatusView status_view(
        &application_runtime,
        &settings,
        &file_dialog,
        &desktop_services,
        &web_platform_host,
        &conversation_service_host
    );
    Win32ApplicationHost application_host;
    Win32MenuController menu_controller;

    (void)previous_instance_handle;

    Diagnostics::write_line("SalixWeb32 starting.");

    std::string launch_message("Launch folder: ");
    launch_message += settings.get_launch_directory();
    Diagnostics::write_line(launch_message.c_str());

    std::string executable_message("Executable folder: ");
    executable_message += settings.get_executable_directory();
    Diagnostics::write_line(executable_message.c_str());

    std::string mode_message("Application mode: ");
    mode_message += settings.get_application_mode_name();
    Diagnostics::write_line(mode_message.c_str());

    std::string data_root_message("User data root: ");
    data_root_message += settings.get_user_data_directory();
    Diagnostics::write_line(data_root_message.c_str());

    std::string diagnostics_message("Diagnostics folder: ");
    diagnostics_message += settings.get_diagnostics_directory();
    Diagnostics::write_line(diagnostics_message.c_str());

    if (settings.get_user_preferences_path()[0] != '\0') {
        std::string preferences_message("Preferences file: ");
        preferences_message += settings.get_user_preferences_path();
        Diagnostics::write_line(preferences_message.c_str());
    }
    Diagnostics::write_line(
        use_remote_bridge
            ? "Web backend selection: remote Browser Probe bridge."
            : "Web backend selection: placeholder."
    );
    Diagnostics::write_line(
        use_remote_bridge
            ? "Conversation backend selection: remote semantic bridge probe."
            : "Conversation backend selection: local semantic placeholder."
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
            &status_view,
            &settings
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
