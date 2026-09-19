// =================================================================================
// Filename:    engine/platform/win32/Win32Utf8Text.h
// Author:      Ebdsaleh
// Description: Converts UTF-8 framework text at the Win32 UTF-16 presentation boundary.
// =================================================================================
#pragma once

#include <windows.h>
#include <string>
#include <vector>

#include "framework/Utf8Text.h"

namespace Win32Utf8Text {
    inline bool to_wide(
        const char* text,
        int text_length,
        std::vector<WCHAR>& output
    ) {
        output.clear();

        if (text == 0 || text_length <= 0) {
            return true;
        }

        UINT code_page = Utf8Text::is_valid(
            text,
            text_length
        ) ? CP_UTF8 : CP_ACP;

        int required = MultiByteToWideChar(
            code_page,
            0,
            text,
            text_length,
            0,
            0
        );

        if (required <= 0) {
            return false;
        }

        output.resize(required);

        int converted = MultiByteToWideChar(
            code_page,
            0,
            text,
            text_length,
            &output[0],
            required
        );

        if (converted != required) {
            output.clear();
            return false;
        }

        return true;
    }

    inline bool from_wide(
        const WCHAR* text,
        int text_length,
        std::string& output
    ) {
        output.clear();

        if (text == 0 || text_length <= 0) {
            return true;
        }

        int required = WideCharToMultiByte(
            CP_UTF8,
            0,
            text,
            text_length,
            0,
            0,
            0,
            0
        );

        if (required <= 0) {
            return false;
        }

        std::vector<char> bytes(required);

        int converted = WideCharToMultiByte(
            CP_UTF8,
            0,
            text,
            text_length,
            &bytes[0],
            required,
            0,
            0
        );

        if (converted != required) {
            return false;
        }

        output.assign(&bytes[0], required);
        return true;
    }

    inline bool ansi_to_utf8(
        const char* text,
        int text_length,
        std::string& output
    ) {
        output.clear();

        if (text == 0 || text_length <= 0) {
            return true;
        }

        int required = MultiByteToWideChar(
            CP_ACP,
            0,
            text,
            text_length,
            0,
            0
        );

        if (required <= 0) {
            return false;
        }

        std::vector<WCHAR> wide(required);

        int converted = MultiByteToWideChar(
            CP_ACP,
            0,
            text,
            text_length,
            &wide[0],
            required
        );

        if (converted != required) {
            return false;
        }

        return from_wide(
            &wide[0],
            required,
            output
        );
    }

    inline bool utf8_to_ansi(
        const char* text,
        int text_length,
        std::string& output
    ) {
        output.clear();

        std::vector<WCHAR> wide;
        if (!to_wide(text, text_length, wide)) {
            return false;
        }

        if (wide.empty()) {
            return true;
        }

        int required = WideCharToMultiByte(
            CP_ACP,
            0,
            &wide[0],
            (int)wide.size(),
            0,
            0,
            "?",
            0
        );

        if (required <= 0) {
            return false;
        }

        std::vector<char> bytes(required);

        int converted = WideCharToMultiByte(
            CP_ACP,
            0,
            &wide[0],
            (int)wide.size(),
            &bytes[0],
            required,
            "?",
            0
        );

        if (converted != required) {
            return false;
        }

        output.assign(&bytes[0], required);
        return true;
    }

    inline bool get_text_extent(
        HDC device_context,
        const char* text,
        int text_length,
        SIZE& size
    ) {
        size.cx = 0;
        size.cy = 0;

        if (
            device_context == NULL ||
            text == 0 ||
            text_length <= 0
        ) {
            return true;
        }

        std::vector<WCHAR> wide;
        if (!to_wide(text, text_length, wide) || wide.empty()) {
            return false;
        }

        return GetTextExtentPoint32W(
            device_context,
            &wide[0],
            (int)wide.size(),
            &size
        ) != 0;
    }

    inline bool text_out(
        HDC device_context,
        int x,
        int y,
        const char* text,
        int text_length
    ) {
        if (
            device_context == NULL ||
            text == 0 ||
            text_length <= 0
        ) {
            return true;
        }

        std::vector<WCHAR> wide;
        if (!to_wide(text, text_length, wide) || wide.empty()) {
            return false;
        }

        return TextOutW(
            device_context,
            x,
            y,
            &wide[0],
            (int)wide.size()
        ) != 0;
    }
}
