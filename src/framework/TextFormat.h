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

    enum SyntaxStyle {
        syntax_none = 0,
        syntax_keyword,
        syntax_string,
        syntax_comment,
        syntax_number,
        syntax_preprocessor,
        syntax_literal,
        syntax_tag
    };

    TextFormat()
        : bold(false),
          italic(false),
          underline(false),
          font_size(12),
          code_style(code_none),
          syntax_style(syntax_none) {
    }

    TextFormat(
        bool new_bold,
        bool new_italic,
        bool new_underline,
        int new_font_size,
        CodeStyle new_code_style = code_none,
        SyntaxStyle new_syntax_style = syntax_none
    ) : bold(new_bold),
        italic(new_italic),
        underline(new_underline),
        font_size(new_font_size),
        code_style(new_code_style),
        syntax_style(new_syntax_style) {
    }

    bool bold;
    bool italic;
    bool underline;
    int font_size;
    CodeStyle code_style;
    SyntaxStyle syntax_style;
};
