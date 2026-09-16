// =================================================================================
// Filename:    app/ConversationMessageView.cpp
// Author:      Ebdsaleh
// Description: Implements one block-composed conversation message presentation.
// =================================================================================

#include <string.h>

#include "ConversationMessageView.h"
#include "CodeBlockView.h"
#include "framework/MarkdownBlockParser.h"
#include "framework/MarkdownFormatter.h"
#include "framework/NativeControlHost.h"
#include "framework/TextMetrics.h"
#include "framework/TextWrapLayout.h"

ConversationMessageView::ConversationMessageView()
    : native_control_host(0),
      separate_role_header(false),
      contains_code_blocks(false),
      preferred_height(24),
      last_layout_width(0),
      block_spacing(8),
      role_spacing(6) {

    get_style().background_color = Color(255, 255, 255);
    get_style().border_width = 0;

    role_header_label.set_horizontal_alignment(Label::align_left);
    role_header_label.set_selectable(false);
    role_header_label.set_visible(false);

    add_child(&role_header_label);
}

ConversationMessageView::~ConversationMessageView() {
    detach_native_controls();
    clear_blocks();
}

bool ConversationMessageView::set_message(
    const FormattedText& source,
    const char* role_label,
    const char* role_prefix,
    const Color& role_color
) {
    clear_blocks();

    FormattedText role_text;
    role_text.set_plain_text(
        role_label == 0 ? "" : role_label,
        TextFormat(true, false, false, 12)
    );
    role_header_label.set_formatted_text(role_text);
    role_header_label.get_style().foreground_color = role_color;

    if (!build_blocks(source, role_prefix, role_color)) {
        role_header_label.set_visible(false);
        preferred_height = 24;
        return false;
    }

    bool markdown_block_layout = MarkdownFormatter::has_block_structure(
        source.get_text()
    );

    separate_role_header =
        contains_code_blocks ||
        blocks.size() > 1 ||
        markdown_block_layout;

    role_header_label.set_visible(separate_role_header);

    if (!separate_role_header && !blocks.empty()) {
        BlockEntry& first_block = blocks[0];
        if (
            first_block.type == BlockEntry::block_text &&
            first_block.text_label != 0
        ) {
            FormattedText presentation;
            FormattedText prefix;
            prefix.set_plain_text(
                role_prefix == 0 ? "" : role_prefix,
                TextFormat(true, false, false, 12)
            );

            presentation.append_formatted_text(prefix);

            FormattedText markdown_text;
            if (!MarkdownFormatter::format(source, markdown_text)) {
                markdown_text = source;
            }
            presentation.append_formatted_text(markdown_text);
            first_block.text_label->set_formatted_text(presentation);
        }
    }

    last_layout_width = 0;
    preferred_height = 24;
    return true;
}

void ConversationMessageView::attach_native_controls(
    NativeControlHost* control_host
) {
    if (native_control_host == control_host) {
        return;
    }

    detach_native_controls();
    native_control_host = control_host;

    if (native_control_host == 0) {
        return;
    }

    for (int index = 0; index < (int)blocks.size(); ++index) {
        if (
            blocks[index].type == BlockEntry::block_code &&
            blocks[index].code_block != 0
        ) {
            blocks[index].code_block->attach_native_controls(
                native_control_host
            );
        }
    }
}

void ConversationMessageView::detach_native_controls() {
    if (native_control_host == 0) {
        return;
    }

    for (int index = 0; index < (int)blocks.size(); ++index) {
        if (
            blocks[index].type == BlockEntry::block_code &&
            blocks[index].code_block != 0
        ) {
            blocks[index].code_block->detach_native_controls();
        }
    }

    native_control_host = 0;
}

int ConversationMessageView::measure_height(
    int width,
    TextMetrics* text_metrics
) {
    if (width < 0) {
        width = 0;
    }

    int total_height = 0;

    if (separate_role_header) {
        total_height += 22 + role_spacing;
    }

    for (int index = 0; index < (int)blocks.size(); ++index) {
        BlockEntry& block = blocks[index];

        if (block.type == BlockEntry::block_code && block.code_block != 0) {
            block.height = block.code_block->measure_height(
                width,
                text_metrics
            );
        } else if (block.text_label != 0) {
            block.height = measure_text_label(
                *block.text_label,
                width,
                text_metrics
            );
        } else {
            block.height = 0;
        }

        if (block.height < 1) {
            block.height = 1;
        }

        total_height += block.height;
        if (index + 1 < (int)blocks.size()) {
            total_height += block_spacing;
        }
    }

    if (total_height < 24) {
        total_height = 24;
    }

    preferred_height = total_height;
    last_layout_width = width;
    return preferred_height;
}

void ConversationMessageView::arrange(
    int x,
    int y,
    int width,
    int height,
    TextMetrics* text_metrics
) {
    set_bounds(x, y, width, height);

    if (width != last_layout_width || text_metrics != 0) {
        measure_height(width, text_metrics);
    }

    layout_children(text_metrics);
}

int ConversationMessageView::get_preferred_height() const {
    return preferred_height;
}

bool ConversationMessageView::has_code_blocks() const {
    return contains_code_blocks;
}

void ConversationMessageView::clear_blocks() {
    for (int index = 0; index < (int)blocks.size(); ++index) {
        BlockEntry& block = blocks[index];

        if (block.type == BlockEntry::block_code && block.code_block != 0) {
            if (native_control_host != 0) {
                block.code_block->detach_native_controls();
            }
            remove_child(block.code_block);
            delete block.code_block;
            block.code_block = 0;
        } else if (block.text_label != 0) {
            remove_child(block.text_label);
            delete block.text_label;
            block.text_label = 0;
        }
    }

    blocks.clear();
    contains_code_blocks = false;
}

bool ConversationMessageView::build_blocks(
    const FormattedText& source,
    const char* role_prefix,
    const Color& role_color
) {
    std::vector<MarkdownBlock> markdown_blocks;
    if (!MarkdownBlockParser::parse(source, markdown_blocks)) {
        return false;
    }

    for (int index = 0; index < (int)markdown_blocks.size(); ++index) {
        const MarkdownBlock& markdown_block = markdown_blocks[index];
        BlockEntry entry;

        if (markdown_block.type == MarkdownBlock::block_code) {
            CodeBlockView* code_block = new CodeBlockView();
            if (code_block == 0) {
                clear_blocks();
                return false;
            }

            code_block->set_code(
                markdown_block.content,
                markdown_block.language.c_str()
            );

            if (native_control_host != 0) {
                code_block->attach_native_controls(native_control_host);
            }

            entry.type = BlockEntry::block_code;
            entry.code_block = code_block;
            contains_code_blocks = true;
            add_child(code_block);
        } else {
            Label* label = new Label();
            if (label == 0) {
                clear_blocks();
                return false;
            }

            FormattedText presentation;
            if (!MarkdownFormatter::format(
                    markdown_block.content,
                    presentation
                )) {
                presentation = markdown_block.content;
            }

            label->set_formatted_text(presentation);
            label->set_horizontal_alignment(Label::align_left);
            label->set_word_wrap(true);
            label->set_selectable(true);
            label->get_style().foreground_color = role_color;

            entry.type = BlockEntry::block_text;
            entry.text_label = label;
            add_child(label);
        }

        blocks.push_back(entry);
    }

    (void)role_prefix;
    return !blocks.empty();
}

int ConversationMessageView::measure_text_label(
    const Label& label,
    int width,
    TextMetrics* text_metrics
) const {
    const char* text = label.get_text();
    int text_length = text == 0 ? 0 : (int)strlen(text);

    if (text_metrics != 0 && width > 0) {
        int height = TextWrapLayout::measure_height(
            text,
            text_length,
            label.get_format_data(),
            label.get_format_count(),
            width,
            2,
            text_metrics
        );

        if (height > 0) {
            return height + 2;
        }
    }

    int maximum_font_size = label.get_max_font_size();
    int line_height = maximum_font_size + 8;
    if (line_height < 20) {
        line_height = 20;
    }

    int character_width = maximum_font_size / 2;
    if (character_width < 6) {
        character_width = 6;
    }

    int characters_per_line = width > 0
        ? width / character_width
        : text_length + 1;
    if (characters_per_line < 1) {
        characters_per_line = 1;
    }

    int visual_lines = 1;
    int current_line_length = 0;

    for (int index = 0; index < text_length; ++index) {
        if (text[index] == '\n') {
            ++visual_lines;
            current_line_length = 0;
            continue;
        }

        ++current_line_length;
        if (current_line_length > characters_per_line) {
            ++visual_lines;
            current_line_length = 1;
        }
    }

    return visual_lines * line_height + (visual_lines - 1) * 2 + 2;
}

void ConversationMessageView::layout_children(
    TextMetrics* text_metrics
) {
    int current_y = get_y();

    if (separate_role_header) {
        role_header_label.set_visible(true);
        role_header_label.set_bounds(
            get_x(),
            current_y,
            get_width(),
            22
        );
        current_y += 22 + role_spacing;
    } else {
        role_header_label.set_visible(false);
    }

    for (int index = 0; index < (int)blocks.size(); ++index) {
        BlockEntry& block = blocks[index];

        if (block.type == BlockEntry::block_code && block.code_block != 0) {
            block.code_block->arrange(
                get_x(),
                current_y,
                get_width(),
                block.height,
                text_metrics
            );
        } else if (block.text_label != 0) {
            block.text_label->set_bounds(
                get_x(),
                current_y,
                get_width(),
                block.height
            );
        }

        current_y += block.height;
        if (index + 1 < (int)blocks.size()) {
            current_y += block_spacing;
        }
    }
}
