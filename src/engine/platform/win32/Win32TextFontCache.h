// =================================================================================
// Filename:    engine/platform/win32/Win32TextFontCache.h
// Author:      Ebdsaleh
// Description: Reuses formatted Win32 fonts during one text measure/render pass.
// =================================================================================
#pragma once

#include <windows.h>
#include <string.h>
#include <wchar.h>
#include <vector>

#include "Win32Utf8Text.h"
#include "framework/TextFormat.h"

class Win32TextFontCache {
    public:
        explicit Win32TextFontCache(HDC new_device_context)
            : device_context(new_device_context) {
        }

        ~Win32TextFontCache() {
            for (int index = 0; index < (int)entries.size(); ++index) {
                if (entries[index].font != NULL) {
                    DeleteObject(entries[index].font);
                }
            }
        }

        HFONT get_font(const TextFormat& format) {
            return get_named_font(
                format,
                get_primary_face(format)
            );
        }

        HFONT get_font_for_text(
            const TextFormat& format,
            const char* text,
            int text_length
        ) {
            HFONT primary = get_font(format);

            if (
                primary == NULL ||
                text == 0 ||
                text_length <= 0 ||
                is_ascii_only(text, text_length) ||
                font_supports_text(primary, text, text_length)
            ) {
                return primary;
            }

            if (format.code_style != TextFormat::code_none) {
                const WCHAR* code_candidates[] = {
                    L"Lucida Console",
                    L"Lucida Sans Unicode",
                    L"Arial Unicode MS",
                    L"Courier New"
                };

                for (int index = 0; index < 4; ++index) {
                    HFONT candidate = get_named_font(
                        format,
                        code_candidates[index]
                    );

                    if (
                        candidate != NULL &&
                        font_supports_text(
                            candidate,
                            text,
                            text_length
                        )
                    ) {
                        return candidate;
                    }
                }
            } else {
                const WCHAR* text_candidates[] = {
                    L"Lucida Sans Unicode",
                    L"Lucida Console",
                    L"Arial Unicode MS",
                    L"Arial",
                    L"Tahoma"
                };

                for (int index = 0; index < 5; ++index) {
                    HFONT candidate = get_named_font(
                        format,
                        text_candidates[index]
                    );

                    if (
                        candidate != NULL &&
                        font_supports_text(
                            candidate,
                            text,
                            text_length
                        )
                    ) {
                        return candidate;
                    }
                }
            }

            return primary;
        }

    private:
        struct Entry {
            Entry()
                : bold(false),
                  italic(false),
                  underline(false),
                  font_size(12),
                  code_style(TextFormat::code_none),
                  font(NULL) {
                face_name[0] = L'\0';
            }

            bool bold;
            bool italic;
            bool underline;
            int font_size;
            TextFormat::CodeStyle code_style;
            WCHAR face_name[LF_FACESIZE];
            HFONT font;
        };

        const WCHAR* get_primary_face(
            const TextFormat& format
        ) const {
            return format.code_style == TextFormat::code_none
                ? L"Tahoma"
                : L"Courier New";
        }

        bool is_ascii_only(
            const char* text,
            int text_length
        ) const {
            for (int index = 0; index < text_length; ++index) {
                if ((unsigned char)text[index] >= 0x80) {
                    return false;
                }
            }

            return true;
        }

        bool matches(
            const Entry& entry,
            const TextFormat& format,
            const WCHAR* face_name
        ) const {
            int font_size = format.font_size < 1 ? 1 : format.font_size;

            return
                entry.bold == format.bold &&
                entry.italic == format.italic &&
                entry.underline == format.underline &&
                entry.font_size == font_size &&
                entry.code_style == format.code_style &&
                face_name != 0 &&
                wcscmp(entry.face_name, face_name) == 0;
        }

        HFONT get_named_font(
            const TextFormat& format,
            const WCHAR* face_name
        ) {
            if (face_name == 0 || face_name[0] == L'\0') {
                return NULL;
            }

            for (int index = 0; index < (int)entries.size(); ++index) {
                if (matches(entries[index], format, face_name)) {
                    return entries[index].font;
                }
            }

            Entry entry;
            entry.bold = format.bold;
            entry.italic = format.italic;
            entry.underline = format.underline;
            entry.font_size = format.font_size < 1 ? 1 : format.font_size;
            entry.code_style = format.code_style;

            wcsncpy(
                entry.face_name,
                face_name,
                LF_FACESIZE - 1
            );
            entry.face_name[LF_FACESIZE - 1] = L'\0';

            entry.font = create_font(
                entry,
                entry.face_name
            );

            entries.push_back(entry);
            return entry.font;
        }

        bool font_supports_text(
            HFONT font,
            const char* text,
            int text_length
        ) const {
            if (
                device_context == NULL ||
                font == NULL ||
                text == 0 ||
                text_length <= 0
            ) {
                return false;
            }

            std::vector<WCHAR> wide;
            if (
                !Win32Utf8Text::to_wide(
                    text,
                    text_length,
                    wide
                ) ||
                wide.empty()
            ) {
                return false;
            }

            typedef DWORD (WINAPI *GetGlyphIndicesWProc)(
                HDC,
                LPCWSTR,
                int,
                LPWORD,
                DWORD
            );

            HMODULE gdi_module = GetModuleHandleA("gdi32.dll");
            if (gdi_module == NULL) {
                return false;
            }

            GetGlyphIndicesWProc get_glyph_indices =
                (GetGlyphIndicesWProc)GetProcAddress(
                    gdi_module,
                    "GetGlyphIndicesW"
                );

            if (get_glyph_indices == 0) {
                return false;
            }

            const DWORD mark_nonexisting_glyphs = 0x0001;
            std::vector<WORD> glyphs(wide.size());

            HGDIOBJ previous_font = SelectObject(
                device_context,
                font
            );

            DWORD result = get_glyph_indices(
                device_context,
                &wide[0],
                (int)wide.size(),
                &glyphs[0],
                mark_nonexisting_glyphs
            );

            if (
                previous_font != NULL &&
                previous_font != HGDI_ERROR
            ) {
                SelectObject(
                    device_context,
                    previous_font
                );
            }

            if (result == GDI_ERROR) {
                return false;
            }

            for (int index = 0; index < (int)glyphs.size(); ++index) {
                if (glyphs[index] == 0xFFFF) {
                    return false;
                }
            }

            return true;
        }

        HFONT create_font(
            const Entry& entry,
            const WCHAR* font_name
        ) const {
            if (
                device_context == NULL ||
                font_name == 0 ||
                font_name[0] == L'\0'
            ) {
                return NULL;
            }

            int logical_height = -MulDiv(
                entry.font_size,
                GetDeviceCaps(device_context, LOGPIXELSY),
                72
            );

            return CreateFontW(
                logical_height,
                0,
                0,
                0,
                entry.bold ? FW_BOLD : FW_NORMAL,
                entry.italic ? TRUE : FALSE,
                entry.underline ? TRUE : FALSE,
                FALSE,
                DEFAULT_CHARSET,
                OUT_DEFAULT_PRECIS,
                CLIP_DEFAULT_PRECIS,
                DEFAULT_QUALITY,
                DEFAULT_PITCH | FF_DONTCARE,
                font_name
            );
        }

        Win32TextFontCache(const Win32TextFontCache&);
        Win32TextFontCache& operator=(const Win32TextFontCache&);

        HDC device_context;
        std::vector<Entry> entries;
};
