// =================================================================================
// Filename:    framework/MarkdownFormatter.h
// Author:      Ebdsaleh
// Description: Declares backend-neutral Markdown-to-FormattedText presentation.
// =================================================================================
#pragma once

#include "FormattedText.h"

class MarkdownFormatter {
    public:
        static bool format(
            const char* source_text,
            FormattedText& output
        );

        static bool format(
            const FormattedText& source_text,
            FormattedText& output
        );

        static bool has_block_structure(const char* source_text);
};
