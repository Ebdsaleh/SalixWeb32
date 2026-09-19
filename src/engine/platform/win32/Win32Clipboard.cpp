// =================================================================================
// Filename:    engine/platform/win32/Win32Clipboard.cpp
// Author:      Ebdsaleh
// Description: Implements the Win32 clipboard backend for MIME-tagged framework data.
// =================================================================================

#include <string.h>
#include <wchar.h>
#include <string>

#include "Win32Clipboard.h"
#include "Win32Utf8Text.h"
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

    bool set_clipboard_text(
        UINT format,
        const char* text,
        int text_length
    ) {
        if (format == 0 || text == 0 || text_length < 0) {
            return false;
        }

        SIZE_T text_size = (SIZE_T)text_length + 1;
        HGLOBAL global_memory = GlobalAlloc(GMEM_MOVEABLE, text_size);

        if (global_memory == NULL) {
            return false;
        }

        char* destination = (char*)GlobalLock(global_memory);
        if (destination == 0) {
            GlobalFree(global_memory);
            return false;
        }

        if (text_length > 0) {
            memcpy(destination, text, (SIZE_T)text_length);
        }
        destination[text_length] = '\0';
        GlobalUnlock(global_memory);

        if (SetClipboardData(format, global_memory) == NULL) {
            GlobalFree(global_memory);
            return false;
        }

        return true;
    }

    bool set_clipboard_unicode_text(
        const char* utf8_text,
        int text_length
    ) {
        if (utf8_text == 0 || text_length < 0) {
            return false;
        }

        std::vector<WCHAR> wide;
        if (!Win32Utf8Text::to_wide(
                utf8_text,
                text_length,
                wide
            )) {
            return false;
        }

        SIZE_T character_count =
            (SIZE_T)wide.size() + 1;
        SIZE_T byte_count =
            character_count * sizeof(WCHAR);

        HGLOBAL global_memory = GlobalAlloc(
            GMEM_MOVEABLE,
            byte_count
        );

        if (global_memory == NULL) {
            return false;
        }

        WCHAR* destination =
            (WCHAR*)GlobalLock(global_memory);

        if (destination == 0) {
            GlobalFree(global_memory);
            return false;
        }

        if (!wide.empty()) {
            memcpy(
                destination,
                &wide[0],
                wide.size() * sizeof(WCHAR)
            );
        }

        destination[wide.size()] = L'\0';
        GlobalUnlock(global_memory);

        if (
            SetClipboardData(
                CF_UNICODETEXT,
                global_memory
            ) == NULL
        ) {
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

    bool read_clipboard_unicode_text(std::string& text) {
        if (!IsClipboardFormatAvailable(CF_UNICODETEXT)) {
            return false;
        }

        HANDLE clipboard_handle =
            GetClipboardData(CF_UNICODETEXT);

        if (clipboard_handle == NULL) {
            return false;
        }

        const WCHAR* clipboard_text =
            (const WCHAR*)GlobalLock(clipboard_handle);

        if (clipboard_text == 0) {
            return false;
        }

        int text_length = (int)wcslen(clipboard_text);
        bool converted = Win32Utf8Text::from_wide(
            clipboard_text,
            text_length,
            text
        );

        GlobalUnlock(clipboard_handle);
        return converted;
    }

    bool read_clipboard_ansi_text(std::string& text) {
        if (!IsClipboardFormatAvailable(CF_TEXT)) {
            return false;
        }

        HANDLE clipboard_handle =
            GetClipboardData(CF_TEXT);

        if (clipboard_handle == NULL) {
            return false;
        }

        const char* clipboard_text =
            (const char*)GlobalLock(clipboard_handle);

        if (clipboard_text == 0) {
            return false;
        }

        int text_length = (int)strlen(clipboard_text);
        bool converted = Win32Utf8Text::ansi_to_utf8(
            clipboard_text,
            text_length,
            text
        );

        GlobalUnlock(clipboard_handle);
        return converted;
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
        int text_length =
            data.get_data_size(MimeTypes::text_plain());

        did_set_data = set_clipboard_unicode_text(
            text,
            text_length
        );

        // Keep a legacy ANSI text flavor available for old external
        // applications, but never reinterpret UTF-8 bytes as CF_TEXT.
        std::string ansi_text;
        if (Win32Utf8Text::utf8_to_ansi(
                text,
                text_length,
                ansi_text
            )) {
            bool ansi_set = set_clipboard_text(
                CF_TEXT,
                ansi_text.c_str(),
                (int)ansi_text.length()
            );

            if (!did_set_data) {
                did_set_data = ansi_set;
            }
        }

        if (data.has_format(MimeTypes::salix_selection_preserved())) {
            UINT preserved_format = get_preserved_selection_format();
            const char* preserved_text = data.get_data(
                MimeTypes::salix_selection_preserved()
            );

            // The standard text format is the interoperability requirement.
            // The private format is an optional richer payload for Salix paste
            // options and must not make an otherwise valid copy fail.
            set_clipboard_text(
                preserved_format,
                preserved_text,
                data.get_data_size(
                    MimeTypes::salix_selection_preserved()
                )
            );
        }
    }

    CloseClipboard();
    return did_set_data;
}

bool Win32Clipboard::get_data(MimeData& data) {
    data.clear();

    UINT preserved_format = get_preserved_selection_format();
    bool has_plain_text =
        IsClipboardFormatAvailable(CF_UNICODETEXT) != 0 ||
        IsClipboardFormatAvailable(CF_TEXT) != 0;
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

    if (
        read_clipboard_unicode_text(clipboard_text) ||
        read_clipboard_ansi_text(clipboard_text)
    ) {
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
