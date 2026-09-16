// =================================================================================
// Filename:    framework/MarkdownBlockParser.cpp
// Author:      Ebdsaleh
// Description: Implements block-level Markdown segmentation for conversation content.
// =================================================================================

#include "MarkdownBlockParser.h"

namespace {
    int find_line_end(const char* text, int length, int line_start) {
        int position = line_start;
        while (position < length && text[position] != '\n') {
            ++position;
        }
        return position;
    }

    int find_first_non_space(const char* text, int start, int end) {
        int position = start;
        while (
            position < end &&
            (text[position] == ' ' || text[position] == '\t')
        ) {
            ++position;
        }
        return position;
    }

    bool get_fence_info(
        const char* text,
        int line_start,
        int line_end,
        int& fence_start,
        int& info_start,
        int& info_end
    ) {
        fence_start = find_first_non_space(text, line_start, line_end);
        if (
            fence_start + 3 > line_end ||
            text[fence_start] != '`' ||
            text[fence_start + 1] != '`' ||
            text[fence_start + 2] != '`'
        ) {
            return false;
        }

        int position = fence_start + 3;
        while (
            position < line_end &&
            (text[position] == ' ' || text[position] == '\t')
        ) {
            ++position;
        }

        info_start = position;
        while (
            position < line_end &&
            text[position] != ' ' &&
            text[position] != '\t' &&
            text[position] != '\r'
        ) {
            ++position;
        }
        info_end = position;
        return true;
    }

    void append_text_block(
        const FormattedText& source,
        int start,
        int end,
        std::vector<MarkdownBlock>& blocks
    ) {
        const char* text = source.get_text();
        if (text == 0) {
            return;
        }

        while (
            start < end &&
            (text[start] == '\n' || text[start] == '\r')
        ) {
            ++start;
        }

        while (
            end > start &&
            (text[end - 1] == '\n' || text[end - 1] == '\r')
        ) {
            --end;
        }

        if (end <= start) {
            return;
        }

        MarkdownBlock block;
        block.type = MarkdownBlock::block_text;
        block.content = source.substring(start, end - start);
        blocks.push_back(block);
    }

    void append_code_block(
        const FormattedText& source,
        int code_start,
        int code_end,
        const char* text,
        int info_start,
        int info_end,
        std::vector<MarkdownBlock>& blocks
    ) {
        while (
            code_end > code_start &&
            (text[code_end - 1] == '\n' || text[code_end - 1] == '\r')
        ) {
            --code_end;
        }

        MarkdownBlock block;
        block.type = MarkdownBlock::block_code;
        block.content = source.substring(code_start, code_end - code_start);

        if (info_end > info_start) {
            block.language.assign(text + info_start, info_end - info_start);
        }

        blocks.push_back(block);
    }
}

bool MarkdownBlockParser::parse(
    const FormattedText& source,
    std::vector<MarkdownBlock>& blocks
) {
    blocks.clear();

    const char* text = source.get_text();
    int length = source.get_length();

    if (text == 0 || length <= 0) {
        return false;
    }

    int text_segment_start = 0;
    int line_start = 0;

    while (line_start < length) {
        int line_end = find_line_end(text, length, line_start);
        int fence_start = 0;
        int info_start = 0;
        int info_end = 0;

        if (!get_fence_info(
                text,
                line_start,
                line_end,
                fence_start,
                info_start,
                info_end
            )) {
            line_start = line_end < length ? line_end + 1 : length;
            continue;
        }

        int code_start = line_end < length ? line_end + 1 : line_end;
        int search_line_start = code_start;
        int closing_line_start = -1;
        int closing_line_end = -1;

        while (search_line_start < length) {
            int search_line_end = find_line_end(
                text,
                length,
                search_line_start
            );
            int closing_fence_start = 0;
            int closing_info_start = 0;
            int closing_info_end = 0;

            if (get_fence_info(
                    text,
                    search_line_start,
                    search_line_end,
                    closing_fence_start,
                    closing_info_start,
                    closing_info_end
                )) {
                closing_line_start = search_line_start;
                closing_line_end = search_line_end;
                break;
            }

            search_line_start = search_line_end < length
                ? search_line_end + 1
                : length;
        }

        if (closing_line_start < 0) {
            break;
        }

        append_text_block(
            source,
            text_segment_start,
            line_start,
            blocks
        );
        append_code_block(
            source,
            code_start,
            closing_line_start,
            text,
            info_start,
            info_end,
            blocks
        );

        line_start = closing_line_end < length
            ? closing_line_end + 1
            : length;
        text_segment_start = line_start;
    }

    append_text_block(
        source,
        text_segment_start,
        length,
        blocks
    );

    if (blocks.empty()) {
        MarkdownBlock block;
        block.type = MarkdownBlock::block_text;
        block.content = source;
        blocks.push_back(block);
    }

    return !blocks.empty();
}
