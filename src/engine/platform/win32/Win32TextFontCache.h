// =================================================================================
// Filename:    engine/platform/win32/Win32TextFontCache.h
// Author:      Ebdsaleh
// Description: Reuses formatted Win32 fonts during one text measure/render pass.
// =================================================================================
#pragma once

#include <windows.h>
#include <vector>

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
            for (int index = 0; index < (int)entries.size(); ++index) {
                if (matches(entries[index], format)) {
                    return entries[index].font;
                }
            }

            Entry entry;
            entry.bold = format.bold;
            entry.italic = format.italic;
            entry.underline = format.underline;
            entry.font_size = format.font_size < 1 ? 1 : format.font_size;
            entry.code_style = format.code_style;
            entry.font = create_font(entry);

            entries.push_back(entry);
            return entry.font;
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
            }

            bool bold;
            bool italic;
            bool underline;
            int font_size;
            TextFormat::CodeStyle code_style;
            HFONT font;
        };

        bool matches(
            const Entry& entry,
            const TextFormat& format
        ) const {
            int font_size = format.font_size < 1 ? 1 : format.font_size;

            return
                entry.bold == format.bold &&
                entry.italic == format.italic &&
                entry.underline == format.underline &&
                entry.font_size == font_size &&
                entry.code_style == format.code_style;
        }

        HFONT create_font(const Entry& entry) const {
            if (device_context == NULL) {
                return NULL;
            }

            int logical_height = -MulDiv(
                entry.font_size,
                GetDeviceCaps(device_context, LOGPIXELSY),
                72
            );

            const char* font_name =
                entry.code_style == TextFormat::code_none
                    ? "Tahoma"
                    : "Courier New";

            return CreateFontA(
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
