// =================================================================================
// Filename:    engine/platform/win32/Win32Clipboard.cpp
// Author:      Ebdsaleh
// Description: Implements the Win32 clipboard backend for MIME-tagged framework data.
// =================================================================================

#include <string.h>
#include <string>

#include "Win32Clipboard.h"
#include "framework/MimeData.h"
#include "framework/MimeTypes.h"

namespace {
    UINT get_preserved_selection_format() {
        static UINT format_id = 0;

        if (format_id == 0) {
            format_id = RegisterClipboardFormatA(
                "SalixWeb32.Selection.Preserved"
            );
        }

        return format_id;
    }

    bool set_clipboard_text(UINT format, const char* text) {
        if (format == 0 || text == 0) {
            return false;
        }

        SIZE_T text_size = strlen(text) + 1;
        HGLOBAL global_memory = GlobalAlloc(GMEM_MOVEABLE, text_size);

        if (global_memory == NULL) {
            return false;
        }

        char* destination = (char*)GlobalLock(global_memory);
        if (destination == 0) {
            GlobalFree(global_memory);
            return false;
        }

        memcpy(destination, text, text_size);
        GlobalUnlock(global_memory);

        if (SetClipboardData(format, global_memory) == NULL) {
            GlobalFree(global_memory);
            return false;
        }

        return true;
    }

    bool read_clipboard_text(UINT format, std::string& text) {
        if (format == 0 || !IsClipboardFormatAvailable(format)) {
            return false;
        }

        HANDLE clipboard_handle = GetClipboardData(format);
        if (clipboard_handle == NULL) {
            return false;
        }

        const char* clipboard_text = (const char*)GlobalLock(clipboard_handle);
        if (clipboard_text == 0) {
            return false;
        }

        text = clipboard_text;
        GlobalUnlock(clipboard_handle);
        return true;
    }
}

Win32Clipboard::Win32Clipboard(HWND new_owner_window)
    : owner_window(new_owner_window) {
}

bool Win32Clipboard::set_data(const MimeData& data) {
    if (!data.has_format(MimeTypes::text_plain())) {
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
        did_set_data = set_clipboard_text(CF_TEXT, text);

        if (data.has_format(MimeTypes::salix_selection_preserved())) {
            UINT preserved_format = get_preserved_selection_format();
            const char* preserved_text = data.get_data(
                MimeTypes::salix_selection_preserved()
            );

            // The standard text format is the interoperability requirement.
            // The private format is an optional richer payload for Salix paste
            // options and must not make an otherwise valid copy fail.
            set_clipboard_text(preserved_format, preserved_text);
        }
    }

    CloseClipboard();
    return did_set_data;
}

bool Win32Clipboard::get_data(MimeData& data) {
    data.clear();

    UINT preserved_format = get_preserved_selection_format();
    bool has_plain_text = IsClipboardFormatAvailable(CF_TEXT) != 0;
    bool has_preserved_text =
        preserved_format != 0 &&
        IsClipboardFormatAvailable(preserved_format) != 0;

    if (!has_plain_text && !has_preserved_text) {
        return false;
    }

    if (!OpenClipboard(owner_window)) {
        return false;
    }

    bool did_get_data = false;
    std::string clipboard_text;

    if (read_clipboard_text(CF_TEXT, clipboard_text)) {
        data.set_text(clipboard_text.c_str());
        did_get_data = true;
    }

    if (read_clipboard_text(preserved_format, clipboard_text)) {
        data.set_data(
            MimeTypes::salix_selection_preserved(),
            clipboard_text.c_str(),
            (int)clipboard_text.length()
        );
        did_get_data = true;
    }

    CloseClipboard();
    return did_get_data;
}
