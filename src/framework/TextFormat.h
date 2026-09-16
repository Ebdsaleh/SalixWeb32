// =================================================================================
// Filename:    framework/TextFormat.h
// Author:      Ebdsaleh
// Description: Declares backend-neutral inline text formatting state.
// =================================================================================
#pragma once

struct TextFormat {
    enum CodeStyle {
        code_none = 0,
        code_inline,
        code_block
    };

    TextFormat()
        : bold(false),
          italic(false),
          underline(false),
          font_size(12),
          code_style(code_none) {
    }

    TextFormat(
        bool new_bold,
        bool new_italic,
        bool new_underline,
        int new_font_size,
        CodeStyle new_code_style = code_none
    ) : bold(new_bold),
        italic(new_italic),
        underline(new_underline),
        font_size(new_font_size),
        code_style(new_code_style) {
    }

    bool bold;
    bool italic;
    bool underline;
    int font_size;
    CodeStyle code_style;
};
