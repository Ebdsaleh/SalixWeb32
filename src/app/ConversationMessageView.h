// =================================================================================
// Filename:    app/ConversationMessageView.h
// Author:      Ebdsaleh
// Description: Declares one block-composed conversation message presentation.
// =================================================================================
#pragma once

#include <vector>

#include "framework/Panel.h"
#include "framework/Label.h"
#include "framework/FormattedText.h"

class Clipboard;
class CodeBlockView;
class NativeControlHost;
class TextMetrics;
class UIEvent;

class ConversationMessageView : public Panel {
    public:
        ConversationMessageView();
        virtual ~ConversationMessageView();

        bool set_message(
            const FormattedText& source,
            const char* role_label,
            const char* role_prefix,
            const Color& role_color
        );

        void attach_native_controls(NativeControlHost* control_host);
        void detach_native_controls();

        int measure_height(int width, TextMetrics* text_metrics);
        void arrange(
            int x,
            int y,
            int width,
            int height,
            TextMetrics* text_metrics = 0
        );

        int get_preferred_height() const;
        bool has_code_blocks() const;

        virtual bool handle_event(const UIEvent& event);

    private:
        struct BlockEntry {
            enum Type {
                block_text = 0,
                block_code
            };

            BlockEntry()
                : type(block_text),
                  text_label(0),
                  code_block(0),
                  height(0) {
            }

            Type type;
            Label* text_label;
            CodeBlockView* code_block;
            int height;
        };

        void clear_blocks();
        bool build_blocks(
            const FormattedText& source,
            const char* role_prefix,
            const Color& role_color
        );
        int measure_text_label(
            const Label& label,
            int width,
            TextMetrics* text_metrics
        ) const;
        void layout_children(TextMetrics* text_metrics);

        void collect_selectable_labels(std::vector<Label*>& labels);
        void collect_selectable_labels(
            std::vector<const Label*>& labels
        ) const;
        int find_selectable_label_at_point(
            const std::vector<Label*>& labels,
            int x,
            int y
        ) const;
        bool resolve_selection_endpoint(
            const std::vector<Label*>& labels,
            const UIEvent& event,
            int& label_index,
            int& character_index
        ) const;
        void clear_presentation_selection(std::vector<Label*>& labels);
        void apply_presentation_selection(
            std::vector<Label*>& labels,
            int target_label_index,
            int target_character_index
        );
        bool has_presentation_selection() const;
        bool has_focused_presentation_label() const;
        bool copy_presentation_selection(Clipboard* clipboard) const;
        void select_all_presentation();
        bool handle_context_menu(const UIEvent& event);

        Label role_header_label;
        std::vector<BlockEntry> blocks;
        NativeControlHost* native_control_host;

        bool separate_role_header;
        bool contains_code_blocks;
        bool presentation_drag_selecting;
        int selection_anchor_label_index;
        int selection_anchor_character_index;
        int preferred_height;
        int last_layout_width;
        int block_spacing;
        int role_spacing;
};
