// =================================================================================
// Filename:    framework/CodeSyntaxHighlighter.cpp
// Author:      Ebdsaleh
// Description: Implements lightweight backend-neutral code syntax tokenization.
// =================================================================================

#include <ctype.h>
#include <string.h>
#include <string>

#include "CodeSyntaxHighlighter.h"
#include "CodeLanguageRegistry.h"
#include "FormattedText.h"
#include "TextFormat.h"

namespace {
    enum LanguageFamily {
        family_plain = 0,
        family_c_like,
        family_python,
        family_json,
        family_shell,
        family_lua,
        family_markup,
        family_css
    };

    bool is_identifier_start(char value) {
        return
            value == '_' ||
            (value >= 'A' && value <= 'Z') ||
            (value >= 'a' && value <= 'z');
    }

    bool is_identifier_character(char value) {
        return
            is_identifier_start(value) ||
            (value >= '0' && value <= '9');
    }

    bool token_in_list(
        const std::string& token,
        const char* const* values,
        int value_count
    ) {
        for (int index = 0; index < value_count; ++index) {
            if (token == values[index]) {
                return true;
            }
        }

        return false;
    }

    LanguageFamily get_family(const char* canonical_language) {
        if (canonical_language == 0 || canonical_language[0] == '\0') {
            return family_plain;
        }

        if (
            strcmp(canonical_language, "c") == 0 ||
            strcmp(canonical_language, "cpp") == 0 ||
            strcmp(canonical_language, "csharp") == 0 ||
            strcmp(canonical_language, "java") == 0 ||
            strcmp(canonical_language, "javascript") == 0 ||
            strcmp(canonical_language, "typescript") == 0 ||
            strcmp(canonical_language, "rust") == 0
        ) {
            return family_c_like;
        }

        if (strcmp(canonical_language, "python") == 0) {
            return family_python;
        }

        if (strcmp(canonical_language, "json") == 0) {
            return family_json;
        }

        if (strcmp(canonical_language, "bash") == 0) {
            return family_shell;
        }

        if (strcmp(canonical_language, "lua") == 0) {
            return family_lua;
        }

        if (
            strcmp(canonical_language, "html") == 0 ||
            strcmp(canonical_language, "xml") == 0
        ) {
            return family_markup;
        }

        if (strcmp(canonical_language, "css") == 0) {
            return family_css;
        }

        return family_plain;
    }

    bool language_supports_preprocessor(const char* canonical_language) {
        return
            canonical_language != 0 &&
            (
                strcmp(canonical_language, "c") == 0 ||
                strcmp(canonical_language, "cpp") == 0 ||
                strcmp(canonical_language, "csharp") == 0
            );
    }

    bool is_literal_token(const std::string& token) {
        static const char* const literals[] = {
            "true", "false", "null", "nullptr", "undefined",
            "None", "True", "False", "nil"
        };

        return token_in_list(
            token,
            literals,
            (int)(sizeof(literals) / sizeof(literals[0]))
        );
    }

    bool is_keyword_token(
        LanguageFamily family,
        const std::string& token
    ) {
        static const char* const c_like_keywords[] = {
            "alignas", "alignof", "asm", "auto", "bool", "break", "case",
            "catch", "char", "class", "const", "constexpr", "continue",
            "default", "delete", "do", "double", "else", "enum", "explicit",
            "export", "extern", "float", "for", "friend", "goto", "if",
            "inline", "int", "interface", "let", "long", "namespace", "new",
            "operator", "private", "protected", "public", "register", "return",
            "short", "signed", "sizeof", "static", "struct", "switch",
            "template", "this", "throw", "try", "typedef", "typename", "typeof",
            "union", "unsigned", "using", "var", "virtual", "void", "volatile",
            "while", "with", "yield", "async", "await", "fn", "impl", "match",
            "mod", "move", "mut", "pub", "ref", "self", "Self", "trait", "use",
            "where", "dyn", "crate", "super", "package", "extends", "implements",
            "instanceof", "synchronized", "throws", "abstract", "final", "native",
            "strictfp"
        };

        static const char* const python_keywords[] = {
            "and", "as", "assert", "async", "await", "break", "class", "continue",
            "def", "del", "elif", "else", "except", "finally", "for", "from",
            "global", "if", "import", "in", "is", "lambda", "nonlocal", "not",
            "or", "pass", "raise", "return", "try", "while", "with", "yield"
        };

        static const char* const shell_keywords[] = {
            "case", "do", "done", "elif", "else", "esac", "fi", "for", "function",
            "if", "in", "select", "then", "time", "until", "while"
        };

        static const char* const lua_keywords[] = {
            "and", "break", "do", "else", "elseif", "end", "false", "for",
            "function", "goto", "if", "in", "local", "nil", "not", "or", "repeat",
            "return", "then", "true", "until", "while"
        };

        switch (family) {
            case family_c_like:
                return token_in_list(
                    token,
                    c_like_keywords,
                    (int)(sizeof(c_like_keywords) / sizeof(c_like_keywords[0]))
                );

            case family_python:
                return token_in_list(
                    token,
                    python_keywords,
                    (int)(sizeof(python_keywords) / sizeof(python_keywords[0]))
                );

            case family_shell:
                return token_in_list(
                    token,
                    shell_keywords,
                    (int)(sizeof(shell_keywords) / sizeof(shell_keywords[0]))
                );

            case family_lua:
                return token_in_list(
                    token,
                    lua_keywords,
                    (int)(sizeof(lua_keywords) / sizeof(lua_keywords[0]))
                );

            default:
                return false;
        }
    }

    void mark_range(
        FormattedText& output,
        int start,
        int end,
        TextFormat::SyntaxStyle syntax_style
    ) {
        if (start < 0) {
            start = 0;
        }
        if (end > output.get_length()) {
            end = output.get_length();
        }
        if (end <= start) {
            return;
        }

        for (int index = start; index < end; ++index) {
            TextFormat format = output.get_character_format(index);
            format.code_style = TextFormat::code_block;
            format.syntax_style = syntax_style;

            if (
                syntax_style == TextFormat::syntax_keyword ||
                syntax_style == TextFormat::syntax_preprocessor ||
                syntax_style == TextFormat::syntax_tag
            ) {
                format.bold = true;
            }

            if (syntax_style == TextFormat::syntax_comment) {
                format.italic = true;
            }

            output.set_character_format(index, format);
        }
    }

    int find_line_end(const char* source, int length, int start) {
        int position = start;
        while (position < length && source[position] != '\n') {
            ++position;
        }
        return position;
    }

    bool only_whitespace_since_line_start(
        const char* source,
        int position
    ) {
        int index = position - 1;

        while (index >= 0 && source[index] != '\n') {
            if (source[index] != ' ' && source[index] != '\t') {
                return false;
            }
            --index;
        }

        return true;
    }

    int scan_string(
        const char* source,
        int length,
        int start,
        char quote
    ) {
        int position = start + 1;

        while (position < length) {
            if (source[position] == '\\' && position + 1 < length) {
                position += 2;
                continue;
            }

            if (source[position] == quote) {
                return position + 1;
            }

            ++position;
        }

        return length;
    }

    int scan_number(const char* source, int length, int start) {
        int position = start;

        while (position < length) {
            char value = source[position];
            if (
                (value >= '0' && value <= '9') ||
                value == '.' ||
                value == 'x' || value == 'X' ||
                value == 'a' || value == 'A' ||
                value == 'b' || value == 'B' ||
                value == 'c' || value == 'C' ||
                value == 'd' || value == 'D' ||
                value == 'e' || value == 'E' ||
                value == 'f' || value == 'F' ||
                value == '_' || value == '+' || value == '-'
            ) {
                ++position;
                continue;
            }

            break;
        }

        return position;
    }

    void highlight_markup(
        const char* source,
        int length,
        FormattedText& output
    ) {
        int position = 0;

        while (position < length) {
            if (
                position + 3 < length &&
                source[position] == '<' &&
                source[position + 1] == '!' &&
                source[position + 2] == '-' &&
                source[position + 3] == '-'
            ) {
                int end = position + 4;
                while (
                    end + 2 < length &&
                    !(
                        source[end] == '-' &&
                        source[end + 1] == '-' &&
                        source[end + 2] == '>'
                    )
                ) {
                    ++end;
                }

                if (end + 2 < length) {
                    end += 3;
                } else {
                    end = length;
                }

                mark_range(output, position, end, TextFormat::syntax_comment);
                position = end;
                continue;
            }

            if (source[position] == '<') {
                int tag_end = position + 1;
                while (tag_end < length && source[tag_end] != '>') {
                    ++tag_end;
                }
                if (tag_end < length) {
                    ++tag_end;
                }

                int cursor = position;
                mark_range(output, cursor, cursor + 1, TextFormat::syntax_tag);
                ++cursor;

                if (cursor < tag_end && source[cursor] == '/') {
                    mark_range(output, cursor, cursor + 1, TextFormat::syntax_tag);
                    ++cursor;
                }

                while (cursor < tag_end) {
                    if (source[cursor] == '"' || source[cursor] == '\'') {
                        int string_end = scan_string(
                            source,
                            tag_end,
                            cursor,
                            source[cursor]
                        );
                        mark_range(
                            output,
                            cursor,
                            string_end,
                            TextFormat::syntax_string
                        );
                        cursor = string_end;
                        continue;
                    }

                    if (is_identifier_start(source[cursor])) {
                        int identifier_end = cursor + 1;
                        while (
                            identifier_end < tag_end &&
                            (
                                is_identifier_character(source[identifier_end]) ||
                                source[identifier_end] == '-' ||
                                source[identifier_end] == ':'
                            )
                        ) {
                            ++identifier_end;
                        }

                        TextFormat::SyntaxStyle style =
                            cursor == position + 1 ||
                            (
                                position + 1 < tag_end &&
                                source[position + 1] == '/' &&
                                cursor == position + 2
                            )
                                ? TextFormat::syntax_tag
                                : TextFormat::syntax_keyword;

                        mark_range(output, cursor, identifier_end, style);
                        cursor = identifier_end;
                        continue;
                    }

                    if (source[cursor] == '>') {
                        mark_range(
                            output,
                            cursor,
                            cursor + 1,
                            TextFormat::syntax_tag
                        );
                    }

                    ++cursor;
                }

                position = tag_end;
                continue;
            }

            ++position;
        }
    }

    void highlight_general(
        const char* canonical_language,
        LanguageFamily family,
        const char* source,
        int length,
        FormattedText& output
    ) {
        int position = 0;

        while (position < length) {
            if (
                language_supports_preprocessor(canonical_language) &&
                source[position] == '#' &&
                only_whitespace_since_line_start(source, position)
            ) {
                int end = find_line_end(source, length, position);
                mark_range(
                    output,
                    position,
                    end,
                    TextFormat::syntax_preprocessor
                );
                position = end;
                continue;
            }

            if (
                (family == family_c_like || family == family_css) &&
                position + 1 < length &&
                source[position] == '/' &&
                source[position + 1] == '*'
            ) {
                int end = position + 2;
                while (
                    end + 1 < length &&
                    !(source[end] == '*' && source[end + 1] == '/')
                ) {
                    ++end;
                }

                if (end + 1 < length) {
                    end += 2;
                } else {
                    end = length;
                }

                mark_range(output, position, end, TextFormat::syntax_comment);
                position = end;
                continue;
            }

            if (
                family == family_lua &&
                position + 3 < length &&
                source[position] == '-' &&
                source[position + 1] == '-' &&
                source[position + 2] == '[' &&
                source[position + 3] == '['
            ) {
                int end = position + 4;
                while (
                    end + 1 < length &&
                    !(source[end] == ']' && source[end + 1] == ']')
                ) {
                    ++end;
                }

                if (end + 1 < length) {
                    end += 2;
                } else {
                    end = length;
                }

                mark_range(output, position, end, TextFormat::syntax_comment);
                position = end;
                continue;
            }

            bool line_comment = false;

            if (
                family == family_c_like &&
                position + 1 < length &&
                source[position] == '/' &&
                source[position + 1] == '/'
            ) {
                line_comment = true;
            } else if (
                (family == family_python || family == family_shell) &&
                source[position] == '#'
            ) {
                line_comment = true;
            } else if (
                family == family_lua &&
                position + 1 < length &&
                source[position] == '-' &&
                source[position + 1] == '-'
            ) {
                line_comment = true;
            }

            if (line_comment) {
                int end = find_line_end(source, length, position);
                mark_range(output, position, end, TextFormat::syntax_comment);
                position = end;
                continue;
            }

            if (
                source[position] == '"' ||
                source[position] == '\'' ||
                (
                    (family == family_c_like || family == family_shell) &&
                    source[position] == '`'
                )
            ) {
                int end = scan_string(
                    source,
                    length,
                    position,
                    source[position]
                );
                mark_range(output, position, end, TextFormat::syntax_string);
                position = end;
                continue;
            }

            if (
                source[position] >= '0' &&
                source[position] <= '9'
            ) {
                int end = scan_number(source, length, position);
                mark_range(output, position, end, TextFormat::syntax_number);
                position = end;
                continue;
            }

            if (is_identifier_start(source[position])) {
                int end = position + 1;
                while (
                    end < length &&
                    is_identifier_character(source[end])
                ) {
                    ++end;
                }

                std::string token(source + position, end - position);

                if (is_literal_token(token)) {
                    mark_range(
                        output,
                        position,
                        end,
                        TextFormat::syntax_literal
                    );
                } else if (is_keyword_token(family, token)) {
                    mark_range(
                        output,
                        position,
                        end,
                        TextFormat::syntax_keyword
                    );
                } else if (
                    family == family_css &&
                    end < length &&
                    source[end] == ':'
                ) {
                    mark_range(
                        output,
                        position,
                        end,
                        TextFormat::syntax_keyword
                    );
                }

                position = end;
                continue;
            }

            ++position;
        }
    }
}

bool CodeSyntaxHighlighter::highlight(
    const char* language,
    const char* source,
    FormattedText& output
) {
    const char* canonical = CodeLanguageRegistry::canonicalize(language);
    LanguageFamily family = get_family(canonical);

    TextFormat base_format(
        false,
        false,
        false,
        11,
        TextFormat::code_block,
        TextFormat::syntax_none
    );

    output.set_plain_text(source == 0 ? "" : source, base_format);

    if (source == 0 || source[0] == '\0' || family == family_plain) {
        return true;
    }

    int length = (int)strlen(source);

    if (family == family_markup) {
        highlight_markup(source, length, output);
    } else {
        highlight_general(canonical, family, source, length, output);
    }

    return true;
}

bool CodeSyntaxHighlighter::supports_language(const char* language) {
    const char* canonical = CodeLanguageRegistry::canonicalize(language);
    return get_family(canonical) != family_plain;
}
