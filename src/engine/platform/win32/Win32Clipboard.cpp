// =================================================================================
// Filename:    engine/platform/win32/Win32Clipboard.cpp
// Author:      Ebdsaleh
// Description: Implements the Win32 clipboard backend for MIME-tagged framework data.
// =================================================================================

#include <string.h>

#include "Win32Clipboard.h"
#include "framework/MimeData.h"

Win32Clipboard::Win32Clipboard(HWND new_owner_window)
    : owner_window(new_owner_window) {
}

bool Win32Clipboard::set_data(const MimeData& data) {
    if (!data.has_format("text/plain")) {
        return false;
    }

    const char* text = data.get_text();
    if (text == 0) {
        return false;
    }

    if (!OpenClipboard(owner_window)) {
        return false;
    }

    bool did_set_data = false;

    if (EmptyClipboard()) {
        SIZE_T text_size = strlen(text) + 1;
        HGLOBAL global_memory = GlobalAlloc(
            GMEM_MOVEABLE,
            text_size
        );

        if (global_memory != NULL) {
            char* destination = (char*)GlobalLock(global_memory);

            if (destination != 0) {
                memcpy(destination, text, text_size);
                GlobalUnlock(global_memory);

                if (SetClipboardData(CF_TEXT, global_memory) != NULL) {
                    did_set_data = true;
                    global_memory = NULL;
                }
            }

            if (global_memory != NULL) {
                GlobalFree(global_memory);
            }
        }
    }

    CloseClipboard();
    return did_set_data;
}

bool Win32Clipboard::get_data(MimeData& data) {
    data.clear();

    if (!IsClipboardFormatAvailable(CF_TEXT)) {
        return false;
    }

    if (!OpenClipboard(owner_window)) {
        return false;
    }

    bool did_get_data = false;
    HANDLE clipboard_handle = GetClipboardData(CF_TEXT);

    if (clipboard_handle != NULL) {
        const char* clipboard_text = (const char*)GlobalLock(clipboard_handle);

        if (clipboard_text != 0) {
            data.set_text(clipboard_text);
            GlobalUnlock(clipboard_handle);
            did_get_data = true;
        }
    }

    CloseClipboard();
    return did_get_data;
}
