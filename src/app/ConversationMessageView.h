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

class CodeBlockView;
class NativeControlHost;
class TextMetrics;

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

        Label role_header_label;
        std::vector<BlockEntry> blocks;
        NativeControlHost* native_control_host;

        bool separate_role_header;
        bool contains_code_blocks;
        int preferred_height;
        int last_layout_width;
        int block_spacing;
        int role_spacing;
};
