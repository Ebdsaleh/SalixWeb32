// =================================================================================
// Filename:    framework/CodeSyntaxHighlighter.h
// Author:      Ebdsaleh
// Description: Declares lightweight backend-neutral code syntax tokenization.
// =================================================================================
#pragma once

class FormattedText;

class CodeSyntaxHighlighter {
    public:
        static bool highlight(
            const char* language,
            const char* source,
            FormattedText& output
        );

        static bool supports_language(const char* language);
};
