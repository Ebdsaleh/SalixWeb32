// =================================================================================
// Filename:    app/ConversationView.h
// Author:      Ebdsaleh
// Description: Declares the append-only scrollable conversation message surface.
// =================================================================================
#pragma once

#include <vector>

#include "framework/Panel.h"
#include "framework/Label.h"
#include "framework/FormattedText.h"
#include "framework/ScrollBar.h"

class NativeControlHost;
class UIEvent;

class ConversationView : public Panel {
    public:
        enum MessageRole {
            message_system = 0,
            message_local,
            message_remote
        };

        ConversationView();
        virtual ~ConversationView();

        bool append_message(MessageRole role, const char* text);
        bool append_message(MessageRole role, const FormattedText& text);
        bool append_system_message(const char* text);
        bool append_local_message(const char* text);
        bool append_local_message(const FormattedText& text);
        bool append_remote_message(const char* text);
        bool append_remote_message(const FormattedText& text);

        void clear_messages();
        int get_message_count() const;

        void attach_native_controls(NativeControlHost* control_host);
        void detach_native_controls();

        void arrange(int x, int y, int width, int height);
        void scroll_lines(int line_count);
        void scroll_to_bottom();

        virtual bool handle_event(const UIEvent& event);

    private:
        struct MessageEntry {
            MessageRole role;
            FormattedText source_text;
            Label* label;
            int row_height;
        };

        static void on_scroll_changed(
            ScrollBar* scroll_bar,
            int value,
            void* context
        );

        void relayout();
        void clamp_first_visible_index();
        void update_scrollbar_state(int visible_count);
        void sync_native_scrollbar();
        int calculate_entry_height(const Label& label) const;
        int calculate_first_index_for_bottom() const;
        int calculate_total_content_height() const;
        const char* get_role_prefix(MessageRole role) const;
        const char* get_role_label(MessageRole role) const;
        Color get_role_color(MessageRole role) const;

        std::vector<MessageEntry> messages;
        ScrollBar vertical_scroll_bar;
        NativeControlHost* native_control_host;
        int first_visible_index;
        int row_spacing;
        int content_padding;
        int scroll_bar_width;
};
