// =================================================================================
// Filename:    framework/TextFormat.h
// Author:      Ebdsaleh
// Description: Declares backend-neutral inline text formatting state.
// =================================================================================
#pragma once

struct TextFormat {
    TextFormat()
        : bold(false),
          italic(false),
          underline(false),
          font_size(12) {
    }

    TextFormat(
        bool new_bold,
        bool new_italic,
        bool new_underline,
        int new_font_size
    ) : bold(new_bold),
        italic(new_italic),
        underline(new_underline),
        font_size(new_font_size) {
    }

    bool bold;
    bool italic;
    bool underline;
    int font_size;
};
