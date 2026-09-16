// =================================================================================
// Filename:    app/ConversationMessageView.cpp
// Author:      Ebdsaleh
// Description: Implements one block-composed conversation message presentation.
// =================================================================================

#include <string.h>
#include <string>

#include "ConversationMessageView.h"
#include "CodeBlockView.h"
#include "framework/Clipboard.h"
#include "framework/ContextMenu.h"
#include "framework/MarkdownBlockParser.h"
#include "framework/MarkdownFormatter.h"
#include "framework/MimeData.h"
#include "framework/NativeControlHost.h"
#include "framework/TextMetrics.h"
#include "framework/TextWrapLayout.h"
#include "framework/UIEvent.h"

namespace {
    enum ConversationContextCommand {
        context_copy = 1,
        context_select_all
    };
}

ConversationMessageView::ConversationMessageView()
    : native_control_host(0),
      separate_role_header(false),
      contains_code_blocks(false),
      presentation_drag_selecting(false),
      selection_anchor_label_index(-1),
      selection_anchor_character_index(0),
      preferred_height(24),
      last_layout_width(0),
      block_spacing(8),
      role_spacing(6) {

    get_style().background_color = Color(255, 255, 255);
    get_style().border_width = 0;

    role_header_label.set_horizontal_alignment(Label::align_left);
    role_header_label.set_selectable(true);
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
    presentation_drag_selecting = false;
    selection_anchor_label_index = -1;
    selection_anchor_character_index = 0;

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

bool ConversationMessageView::handle_event(const UIEvent& event) {
    if (!get_is_visible()) {
        return false;
    }

    if (
        event.type == UIEvent::event_context_menu &&
        contains_point(event.x, event.y)
    ) {
        return handle_context_menu(event);
    }

    if (
        event.type == UIEvent::event_key_down &&
        event.control_down
    ) {
        if (
            event.key_code == UIEvent::key_c &&
            has_presentation_selection()
        ) {
            copy_presentation_selection(event.clipboard);
            return true;
        }

        if (
            event.key_code == UIEvent::key_a &&
            has_focused_presentation_label()
        ) {
            select_all_presentation();
            return true;
        }
    }

    std::vector<Label*> labels;
    collect_selectable_labels(labels);

    if (
        event.type == UIEvent::event_mouse_down &&
        event.click_count == 1 &&
        !event.control_down &&
        !event.alt_down &&
        !event.shift_down &&
        !contains_point(event.x, event.y)
    ) {
        clear_presentation_selection(labels);
        presentation_drag_selecting = false;
        selection_anchor_label_index = -1;
        selection_anchor_character_index = 0;
        return false;
    }

    if (
        event.type == UIEvent::event_mouse_down &&
        event.click_count == 1 &&
        !event.control_down &&
        !event.alt_down &&
        !event.shift_down
    ) {
        int label_index = find_selectable_label_at_point(
            labels,
            event.x,
            event.y
        );

        if (label_index >= 0) {
            clear_presentation_selection(labels);

            for (int index = 0; index < (int)labels.size(); ++index) {
                labels[index]->set_focused(index == label_index);
            }

            int character_index = labels[label_index]->
                get_character_index_at_event(event);

            selection_anchor_label_index = label_index;
            selection_anchor_character_index = character_index;
            presentation_drag_selecting = true;
            labels[label_index]->set_selection_range(
                character_index,
                character_index
            );
            return true;
        }
    }

    if (
        event.type == UIEvent::event_mouse_move &&
        presentation_drag_selecting &&
        event.left_button_down
    ) {
        int target_label_index = -1;
        int target_character_index = 0;

        if (resolve_selection_endpoint(
                labels,
                event,
                target_label_index,
                target_character_index
            )) {
            apply_presentation_selection(
                labels,
                target_label_index,
                target_character_index
            );
        }

        return true;
    }

    if (
        event.type == UIEvent::event_mouse_up &&
        presentation_drag_selecting
    ) {
        int target_label_index = -1;
        int target_character_index = 0;

        if (resolve_selection_endpoint(
                labels,
                event,
                target_label_index,
                target_character_index
            )) {
            apply_presentation_selection(
                labels,
                target_label_index,
                target_character_index
            );
        }

        presentation_drag_selecting = false;
        return true;
    }

    return Panel::handle_event(event);
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
    presentation_drag_selecting = false;
    selection_anchor_label_index = -1;
    selection_anchor_character_index = 0;
    role_header_label.clear_selection();
    role_header_label.set_focused(false);
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

void ConversationMessageView::collect_selectable_labels(
    std::vector<Label*>& labels
) {
    labels.clear();

    if (
        role_header_label.get_is_visible() &&
        role_header_label.get_is_selectable()
    ) {
        labels.push_back(&role_header_label);
    }

    for (int index = 0; index < (int)blocks.size(); ++index) {
        BlockEntry& block = blocks[index];

        if (
            block.type == BlockEntry::block_text &&
            block.text_label != 0 &&
            block.text_label->get_is_visible() &&
            block.text_label->get_is_selectable()
        ) {
            labels.push_back(block.text_label);
        } else if (
            block.type == BlockEntry::block_code &&
            block.code_block != 0 &&
            block.code_block->get_is_visible()
        ) {
            Label* code_label = block.code_block->get_selectable_label();
            if (
                code_label != 0 &&
                code_label->get_is_visible() &&
                code_label->get_is_selectable()
            ) {
                labels.push_back(code_label);
            }
        }
    }
}

void ConversationMessageView::collect_selectable_labels(
    std::vector<const Label*>& labels
) const {
    labels.clear();

    if (
        role_header_label.get_is_visible() &&
        role_header_label.get_is_selectable()
    ) {
        labels.push_back(&role_header_label);
    }

    for (int index = 0; index < (int)blocks.size(); ++index) {
        const BlockEntry& block = blocks[index];

        if (
            block.type == BlockEntry::block_text &&
            block.text_label != 0 &&
            block.text_label->get_is_visible() &&
            block.text_label->get_is_selectable()
        ) {
            labels.push_back(block.text_label);
        } else if (
            block.type == BlockEntry::block_code &&
            block.code_block != 0 &&
            block.code_block->get_is_visible()
        ) {
            const Label* code_label = block.code_block->get_selectable_label();
            if (
                code_label != 0 &&
                code_label->get_is_visible() &&
                code_label->get_is_selectable()
            ) {
                labels.push_back(code_label);
            }
        }
    }
}

int ConversationMessageView::find_selectable_label_at_point(
    const std::vector<Label*>& labels,
    int x,
    int y
) const {
    for (int index = 0; index < (int)labels.size(); ++index) {
        if (labels[index] != 0 && labels[index]->contains_point(x, y)) {
            return index;
        }
    }

    return -1;
}

bool ConversationMessageView::resolve_selection_endpoint(
    const std::vector<Label*>& labels,
    const UIEvent& event,
    int& label_index,
    int& character_index
) const {
    label_index = -1;
    character_index = 0;

    if (labels.empty()) {
        return false;
    }

    int direct_index = find_selectable_label_at_point(
        labels,
        event.x,
        event.y
    );

    if (direct_index >= 0) {
        label_index = direct_index;
        character_index = labels[direct_index]->
            get_character_index_at_event(event);
        return true;
    }

    if (event.y < labels[0]->get_y()) {
        label_index = 0;
        character_index = 0;
        return true;
    }

    for (int index = 0; index < (int)labels.size(); ++index) {
        Label* label = labels[index];
        int top = label->get_y();
        int bottom = top + label->get_height();

        if (event.y >= top && event.y < bottom) {
            label_index = index;
            character_index = label->get_character_index_at_event(event);
            return true;
        }

        if (index + 1 < (int)labels.size()) {
            int next_top = labels[index + 1]->get_y();
            if (event.y >= bottom && event.y < next_top) {
                int distance_to_previous = event.y - bottom;
                int distance_to_next = next_top - event.y;

                if (distance_to_previous <= distance_to_next) {
                    label_index = index;
                    const char* text = label->get_text();
                    character_index = text == 0 ? 0 : (int)strlen(text);
                } else {
                    label_index = index + 1;
                    character_index = 0;
                }
                return true;
            }
        }
    }

    label_index = (int)labels.size() - 1;
    const char* last_text = labels[label_index]->get_text();
    character_index = last_text == 0 ? 0 : (int)strlen(last_text);
    return true;
}

void ConversationMessageView::clear_presentation_selection(
    std::vector<Label*>& labels
) {
    for (int index = 0; index < (int)labels.size(); ++index) {
        labels[index]->clear_selection();
        labels[index]->set_focused(false);
    }
}

void ConversationMessageView::apply_presentation_selection(
    std::vector<Label*>& labels,
    int target_label_index,
    int target_character_index
) {
    if (
        selection_anchor_label_index < 0 ||
        selection_anchor_label_index >= (int)labels.size() ||
        target_label_index < 0 ||
        target_label_index >= (int)labels.size()
    ) {
        return;
    }

    for (int index = 0; index < (int)labels.size(); ++index) {
        labels[index]->clear_selection();
    }

    int anchor_index = selection_anchor_label_index;
    int anchor_character = selection_anchor_character_index;

    if (target_label_index == anchor_index) {
        labels[anchor_index]->set_selection_range(
            anchor_character,
            target_character_index
        );
        return;
    }

    if (target_label_index > anchor_index) {
        const char* anchor_text = labels[anchor_index]->get_text();
        int anchor_length = anchor_text == 0 ? 0 : (int)strlen(anchor_text);

        labels[anchor_index]->set_selection_range(
            anchor_character,
            anchor_length
        );

        for (
            int index = anchor_index + 1;
            index < target_label_index;
            ++index
        ) {
            labels[index]->select_all();
        }

        labels[target_label_index]->set_selection_range(
            0,
            target_character_index
        );
        return;
    }

    labels[target_label_index]->set_selection_range(
        target_character_index,
        labels[target_label_index]->get_text() == 0
            ? 0
            : (int)strlen(labels[target_label_index]->get_text())
    );

    for (
        int index = target_label_index + 1;
        index < anchor_index;
        ++index
    ) {
        labels[index]->select_all();
    }

    labels[anchor_index]->set_selection_range(
        0,
        anchor_character
    );
}

bool ConversationMessageView::has_presentation_selection() const {
    std::vector<const Label*> labels;
    collect_selectable_labels(labels);

    for (int index = 0; index < (int)labels.size(); ++index) {
        if (labels[index]->has_selection()) {
            return true;
        }
    }

    return false;
}

bool ConversationMessageView::has_focused_presentation_label() const {
    std::vector<const Label*> labels;
    collect_selectable_labels(labels);

    for (int index = 0; index < (int)labels.size(); ++index) {
        if (labels[index]->get_is_focused()) {
            return true;
        }
    }

    return false;
}

bool ConversationMessageView::copy_presentation_selection(
    Clipboard* clipboard
) const {
    if (clipboard == 0) {
        return false;
    }

    std::vector<const Label*> labels;
    collect_selectable_labels(labels);

    std::string output;

    for (int label_index = 0; label_index < (int)labels.size(); ++label_index) {
        const Label* label = labels[label_index];
        if (label == 0 || !label->has_selection()) {
            continue;
        }

        const char* label_text = label->get_text();
        if (label_text == 0) {
            continue;
        }

        int text_length = (int)strlen(label_text);
        std::string selected_text;

        for (
            int range_index = 0;
            range_index < label->get_selection_range_count();
            ++range_index
        ) {
            int start = 0;
            int end = 0;

            if (!label->get_selection_range(range_index, start, end)) {
                continue;
            }

            if (start < 0) {
                start = 0;
            }
            if (end > text_length) {
                end = text_length;
            }
            if (end <= start) {
                continue;
            }

            if (!selected_text.empty()) {
                selected_text += ' ';
            }

            selected_text.append(
                label_text + start,
                (std::string::size_type)(end - start)
            );
        }

        if (selected_text.empty()) {
            continue;
        }

        if (!output.empty()) {
            output += "\r\n";
        }

        output += selected_text;
    }

    if (output.empty()) {
        return false;
    }

    MimeData data;
    data.set_text(output.c_str());
    return clipboard->set_data(data);
}

void ConversationMessageView::select_all_presentation() {
    std::vector<Label*> labels;
    collect_selectable_labels(labels);

    for (int index = 0; index < (int)labels.size(); ++index) {
        labels[index]->select_all();
        labels[index]->set_focused(index == 0);
    }
}

bool ConversationMessageView::handle_context_menu(
    const UIEvent& event
) {
    NativeControlHost* menu_host = event.native_control_host != 0
        ? event.native_control_host
        : native_control_host;

    if (menu_host == 0) {
        return false;
    }

    std::vector<Label*> labels;
    collect_selectable_labels(labels);

    bool has_selection = has_presentation_selection();
    bool has_text = false;

    for (int index = 0; index < (int)labels.size(); ++index) {
        const char* text = labels[index]->get_text();
        if (text != 0 && text[0] != '\0') {
            has_text = true;
            break;
        }
    }

    ContextMenu menu;
    menu.add_item(context_copy, "Copy", has_selection);
    menu.add_separator();
    menu.add_item(context_select_all, "Select All", has_text);

    int command_id = menu_host->show_context_menu(
        menu,
        event.x,
        event.y
    );

    if (command_id == context_copy) {
        copy_presentation_selection(event.clipboard);
    } else if (command_id == context_select_all) {
        select_all_presentation();
    }

    return true;
}
