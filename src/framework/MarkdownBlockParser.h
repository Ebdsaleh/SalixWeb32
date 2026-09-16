// =================================================================================
// Filename:    framework/MarkdownBlockParser.h
// Author:      Ebdsaleh
// Description: Declares block-level Markdown segmentation for conversation content.
// =================================================================================
#pragma once

#include <string>
#include <vector>

#include "FormattedText.h"

struct MarkdownBlock {
    enum Type {
        block_text = 0,
        block_code
    };

    MarkdownBlock()
        : type(block_text) {
    }

    Type type;
    FormattedText content;
    std::string language;
};

class MarkdownBlockParser {
    public:
        static bool parse(
            const FormattedText& source,
            std::vector<MarkdownBlock>& blocks
        );
};
