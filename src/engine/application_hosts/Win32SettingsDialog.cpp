// =================================================================================
// Filename:    engine/application_hosts/Win32SettingsDialog.cpp
// Author:      Ebdsaleh
// Description: Implements persistent file-location settings on NT5 Win32.
// =================================================================================

#include <windows.h>
#include <string>

#include "Win32SettingsDialog.h"
#include "app/ApplicationSettings.h"
#include "engine/platform/win32/Win32ApplicationPaths.h"

typedef int (CALLBACK* SalixBrowseCallback)(
    HWND,
    UINT,
    LPARAM,
    LPARAM
);

struct SalixBrowseInfoA {
    HWND owner;
    const void* root;
    LPSTR display_name;
    LPCSTR title;
    UINT flags;
    SalixBrowseCallback callback;
    LPARAM callback_data;
    int image;
};

extern "C" void* WINAPI SHBrowseForFolderA(
    SalixBrowseInfoA* browse_info
);

extern "C" BOOL WINAPI SHGetPathFromIDListA(
    const void* item_id_list,
    LPSTR path
);

extern "C" void WINAPI CoTaskMemFree(LPVOID memory);

namespace {
    const char* settings_dialog_class =
        "SalixWeb32SettingsDialog";

    const int settings_browse_diagnostics = 45201;
    const int settings_browse_attachments = 45202;
    const int settings_restore_defaults = 45203;

    const UINT salix_bif_return_only_file_system_dirs = 0x0001;
    const UINT salix_bif_edit_box = 0x0010;
    const UINT salix_bffm_initialized = 1;
    const UINT salix_bffm_set_selection_a = WM_USER + 102;

    struct SettingsDialogState {
        ApplicationSettings* settings;
        HWND diagnostics_edit;
        HWND attachments_edit;
        bool saved;

        SettingsDialogState()
            : settings(0),
              diagnostics_edit(NULL),
              attachments_edit(NULL),
              saved(false) {
        }
    };

    void set_default_gui_font(HWND control) {
        if (control == NULL) {
            return;
        }

        HFONT font = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
        if (font != NULL) {
            SendMessageA(
                control,
                WM_SETFONT,
                (WPARAM)font,
                TRUE
            );
        }
    }

    std::string get_control_text(HWND control) {
        if (control == NULL) {
            return std::string();
        }

        int length = GetWindowTextLengthA(control);
        if (length < 1) {
            return std::string();
        }

        char* buffer = new char[length + 1];
        if (buffer == 0) {
            return std::string();
        }

        buffer[0] = '\0';
        GetWindowTextA(control, buffer, length + 1);

        std::string result(buffer);
        delete[] buffer;
        return result;
    }

    int CALLBACK browse_callback(
        HWND browser_handle,
        UINT message,
        LPARAM l_param,
        LPARAM callback_data
    ) {
        (void)l_param;

        if (
            message == salix_bffm_initialized &&
            callback_data != 0
        ) {
            const char* initial_directory =
                (const char*)callback_data;

            if (
                initial_directory != 0 &&
                initial_directory[0] != '\0'
            ) {
                SendMessageA(
                    browser_handle,
                    salix_bffm_set_selection_a,
                    TRUE,
                    (LPARAM)initial_directory
                );
            }
        }

        return 0;
    }

    bool browse_for_directory(
        HWND owner_handle,
        const char* title,
        const std::string& initial_directory,
        std::string& selected_directory
    ) {
        selected_directory.clear();

        char display_name[MAX_PATH + 1];
        display_name[0] = '\0';

        SalixBrowseInfoA browse_info;
        ZeroMemory(&browse_info, sizeof(browse_info));

        browse_info.owner = owner_handle;
        browse_info.display_name = display_name;
        browse_info.title = title;
        browse_info.flags =
            salix_bif_return_only_file_system_dirs |
            salix_bif_edit_box;
        browse_info.callback = browse_callback;
        browse_info.callback_data =
            (LPARAM)initial_directory.c_str();

        void* item_id_list =
            SHBrowseForFolderA(&browse_info);

        if (item_id_list == 0) {
            return false;
        }

        char path[MAX_PATH + 1];
        path[0] = '\0';

        BOOL resolved = SHGetPathFromIDListA(
            item_id_list,
            path
        );

        CoTaskMemFree(item_id_list);

        if (!resolved || path[0] == '\0') {
            return false;
        }

        selected_directory = path;
        return true;
    }

    void center_over_owner(
        HWND dialog_handle,
        HWND owner_handle
    ) {
        RECT dialog_rect;
        RECT owner_rect;

        if (
            dialog_handle == NULL ||
            !GetWindowRect(dialog_handle, &dialog_rect)
        ) {
            return;
        }

        int width =
            dialog_rect.right - dialog_rect.left;
        int height =
            dialog_rect.bottom - dialog_rect.top;

        int x =
            (GetSystemMetrics(SM_CXSCREEN) - width) / 2;
        int y =
            (GetSystemMetrics(SM_CYSCREEN) - height) / 2;

        if (
            owner_handle != NULL &&
            IsWindow(owner_handle) &&
            GetWindowRect(owner_handle, &owner_rect)
        ) {
            x = owner_rect.left +
                (
                    owner_rect.right -
                    owner_rect.left -
                    width
                ) / 2;

            y = owner_rect.top +
                (
                    owner_rect.bottom -
                    owner_rect.top -
                    height
                ) / 2;
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

    bool validate_directory(
        HWND owner_handle,
        const char* label,
        const std::string& path
    ) {
        if (
            path.empty() ||
            path.size() >= MAX_PATH ||
            !Win32ApplicationPaths::is_absolute_path(
                path.c_str()
            )
        ) {
            std::string message(label);
            message +=
                " must be an absolute NT5-compatible path.";

            MessageBoxA(
                owner_handle,
                message.c_str(),
                "SalixWeb32 Settings",
                MB_OK | MB_ICONERROR
            );
            return false;
        }

        if (
            !Win32ApplicationPaths::ensure_directory_exists(
                path.c_str()
            )
        ) {
            std::string message(
                "SalixWeb32 could not create or access the "
            );
            message += label;
            message += ".";

            MessageBoxA(
                owner_handle,
                message.c_str(),
                "SalixWeb32 Settings",
                MB_OK | MB_ICONERROR
            );
            return false;
        }

        return true;
    }

    void restore_default_locations(
        SettingsDialogState* state
    ) {
        if (
            state == 0 ||
            state->settings == 0
        ) {
            return;
        }

        std::string diagnostics_directory =
            state->settings->get_default_diagnostics_directory();

        std::string attachments_directory =
            state->settings->get_default_attachment_directory();

        SetWindowTextA(
            state->diagnostics_edit,
            diagnostics_directory.c_str()
        );

        SetWindowTextA(
            state->attachments_edit,
            attachments_directory.c_str()
        );
    }

    bool save_settings(
        HWND dialog_handle,
        SettingsDialogState* state
    ) {
        if (
            state == 0 ||
            state->settings == 0
        ) {
            return false;
        }

        std::string diagnostics_directory =
            get_control_text(state->diagnostics_edit);

        std::string attachments_directory =
            get_control_text(state->attachments_edit);

        if (
            !validate_directory(
                dialog_handle,
                "Diagnostics folder",
                diagnostics_directory
            )
        ) {
            return false;
        }

        if (
            !validate_directory(
                dialog_handle,
                "Attachment browser folder",
                attachments_directory
            )
        ) {
            return false;
        }

        std::string previous_diagnostics =
            state->settings->get_diagnostics_directory();

        std::string previous_attachments =
            state->settings->get_attachment_directory();

        state->settings->set_diagnostics_directory(
            diagnostics_directory.c_str()
        );

        state->settings->set_attachment_directory(
            attachments_directory.c_str()
        );

        if (!state->settings->save_user_preferences()) {
            state->settings->set_diagnostics_directory(
                previous_diagnostics.c_str()
            );
            state->settings->set_attachment_directory(
                previous_attachments.c_str()
            );

            MessageBoxA(
                dialog_handle,
                "SalixWeb32 could not save the settings file.",
                "SalixWeb32 Settings",
                MB_OK | MB_ICONERROR
            );
            return false;
        }

        state->saved = true;
        return true;
    }

    LRESULT CALLBACK settings_window_proc(
        HWND dialog_handle,
        UINT message,
        WPARAM w_param,
        LPARAM l_param
    ) {
        SettingsDialogState* state =
            (SettingsDialogState*)GetWindowLongA(
                dialog_handle,
                GWL_USERDATA
            );

        if (message == WM_NCCREATE) {
            CREATESTRUCTA* create_struct =
                (CREATESTRUCTA*)l_param;

            SetWindowLongA(
                dialog_handle,
                GWL_USERDATA,
                (LONG)create_struct->lpCreateParams
            );
            return TRUE;
        }

        if (message == WM_COMMAND) {
            int command_id = (int)LOWORD(w_param);

            if (
                command_id == settings_browse_diagnostics &&
                state != 0
            ) {
                std::string selected_directory;
                std::string current =
                    get_control_text(
                        state->diagnostics_edit
                    );

                if (browse_for_directory(
                        dialog_handle,
                        "Choose the SalixWeb32 diagnostics folder",
                        current,
                        selected_directory
                    )) {
                    SetWindowTextA(
                        state->diagnostics_edit,
                        selected_directory.c_str()
                    );
                }
                return 0;
            }

            if (
                command_id == settings_browse_attachments &&
                state != 0
            ) {
                std::string selected_directory;
                std::string current =
                    get_control_text(
                        state->attachments_edit
                    );

                if (browse_for_directory(
                        dialog_handle,
                        "Choose the default attachment browser folder",
                        current,
                        selected_directory
                    )) {
                    SetWindowTextA(
                        state->attachments_edit,
                        selected_directory.c_str()
                    );
                }
                return 0;
            }

            if (
                command_id == settings_restore_defaults
            ) {
                restore_default_locations(state);
                return 0;
            }

            if (command_id == IDOK) {
                if (save_settings(dialog_handle, state)) {
                    DestroyWindow(dialog_handle);
                }
                return 0;
            }

            if (command_id == IDCANCEL) {
                DestroyWindow(dialog_handle);
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

    bool register_settings_class(
        HINSTANCE instance_handle
    ) {
        WNDCLASSA window_class;
        ZeroMemory(
            &window_class,
            sizeof(window_class)
        );

        window_class.style =
            CS_HREDRAW | CS_VREDRAW;
        window_class.lpfnWndProc =
            settings_window_proc;
        window_class.hInstance =
            instance_handle;
        window_class.hIcon =
            LoadIconA(NULL, IDI_APPLICATION);
        window_class.hCursor =
            LoadCursorA(NULL, IDC_ARROW);
        window_class.hbrBackground =
            (HBRUSH)(COLOR_BTNFACE + 1);
        window_class.lpszClassName =
            settings_dialog_class;

        if (RegisterClassA(&window_class) != 0) {
            return true;
        }

        return
            GetLastError() ==
            ERROR_CLASS_ALREADY_EXISTS;
    }

    HWND create_label(
        HWND parent,
        HINSTANCE instance_handle,
        const char* text,
        int x,
        int y,
        int width,
        int height
    ) {
        HWND control = CreateWindowExA(
            0,
            "STATIC",
            text,
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            x,
            y,
            width,
            height,
            parent,
            NULL,
            instance_handle,
            NULL
        );

        set_default_gui_font(control);
        return control;
    }

    HWND create_button(
        HWND parent,
        HINSTANCE instance_handle,
        const char* text,
        int command_id,
        int x,
        int y,
        int width,
        bool default_button
    ) {
        HWND control = CreateWindowExA(
            0,
            "BUTTON",
            text,
            WS_CHILD | WS_VISIBLE | WS_TABSTOP |
                (
                    default_button
                        ? BS_DEFPUSHBUTTON
                        : BS_PUSHBUTTON
                ),
            x,
            y,
            width,
            27,
            parent,
            (HMENU)command_id,
            instance_handle,
            NULL
        );

        set_default_gui_font(control);
        return control;
    }
}

bool Win32SettingsDialog::show(
    HWND owner_handle,
    ApplicationSettings* settings
) {
    if (
        owner_handle == NULL ||
        settings == 0
    ) {
        return false;
    }

    HINSTANCE instance_handle =
        (HINSTANCE)GetWindowLongA(
            owner_handle,
            GWL_HINSTANCE
        );

    if (
        instance_handle == NULL ||
        !register_settings_class(instance_handle)
    ) {
        return false;
    }

    SettingsDialogState state;
    state.settings = settings;

    HWND dialog_handle = CreateWindowExA(
        WS_EX_DLGMODALFRAME | WS_EX_CONTROLPARENT,
        settings_dialog_class,
        "SalixWeb32 Settings",
        WS_POPUP | WS_CAPTION | WS_SYSMENU,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        730,
        370,
        owner_handle,
        NULL,
        instance_handle,
        &state
    );

    if (dialog_handle == NULL) {
        return false;
    }

    std::string mode_text("Application mode: ");
    mode_text += settings->get_application_mode_name();

    create_label(
        dialog_handle,
        instance_handle,
        mode_text.c_str(),
        18,
        18,
        680,
        20
    );

    std::string data_root_text(
        settings->is_portable_mode()
            ? "Data root: "
            : "User data: "
    );
    data_root_text += settings->get_user_data_directory();

    create_label(
        dialog_handle,
        instance_handle,
        data_root_text.c_str(),
        18,
        42,
        680,
        34
    );

    create_label(
        dialog_handle,
        instance_handle,
        "File dialogs never determine application storage locations.",
        18,
        80,
        680,
        20
    );

    create_label(
        dialog_handle,
        instance_handle,
        "Diagnostics folder:",
        18,
        112,
        170,
        20
    );

    state.diagnostics_edit = CreateWindowExA(
        WS_EX_CLIENTEDGE,
        "EDIT",
        settings->get_diagnostics_directory(),
        WS_CHILD | WS_VISIBLE | WS_TABSTOP |
            ES_AUTOHSCROLL,
        18,
        134,
        580,
        24,
        dialog_handle,
        NULL,
        instance_handle,
        NULL
    );
    set_default_gui_font(state.diagnostics_edit);

    create_button(
        dialog_handle,
        instance_handle,
        "Browse...",
        settings_browse_diagnostics,
        608,
        133,
        88,
        false
    );

    create_label(
        dialog_handle,
        instance_handle,
        "Attachment browser folder:",
        18,
        172,
        200,
        20
    );

    state.attachments_edit = CreateWindowExA(
        WS_EX_CLIENTEDGE,
        "EDIT",
        settings->get_attachment_directory(),
        WS_CHILD | WS_VISIBLE | WS_TABSTOP |
            ES_AUTOHSCROLL,
        18,
        194,
        580,
        24,
        dialog_handle,
        NULL,
        instance_handle,
        NULL
    );
    set_default_gui_font(state.attachments_edit);

    create_button(
        dialog_handle,
        instance_handle,
        "Browse...",
        settings_browse_attachments,
        608,
        193,
        88,
        false
    );

    std::string preference_text(
        "Preferences file: "
    );
    preference_text +=
        settings->get_user_preferences_path();

    create_label(
        dialog_handle,
        instance_handle,
        preference_text.c_str(),
        18,
        234,
        680,
        38
    );

    create_button(
        dialog_handle,
        instance_handle,
        "Restore Defaults",
        settings_restore_defaults,
        18,
        292,
        120,
        false
    );

    create_button(
        dialog_handle,
        instance_handle,
        "Cancel",
        IDCANCEL,
        516,
        292,
        84,
        false
    );

    create_button(
        dialog_handle,
        instance_handle,
        "Save",
        IDOK,
        612,
        292,
        84,
        true
    );

    center_over_owner(
        dialog_handle,
        owner_handle
    );

    EnableWindow(owner_handle, FALSE);
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

        if (
            !IsDialogMessageA(
                dialog_handle,
                &message_data
            )
        ) {
            TranslateMessage(&message_data);
            DispatchMessageA(&message_data);
        }
    }

    EnableWindow(owner_handle, TRUE);
    SetForegroundWindow(owner_handle);

    if (repost_quit) {
        PostQuitMessage(quit_code);
    }

    return state.saved;
}
