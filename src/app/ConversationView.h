// =================================================================================
// Filename:    app/ConversationView.h
// Author:      Ebdsaleh
// Description: Declares the append-only scrollable conversation message surface.
// =================================================================================
#pragma once

#include <vector>

#include "framework/Panel.h"
#include "framework/Label.h"
#include "framework/Button.h"

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
        bool append_system_message(const char* text);
        bool append_local_message(const char* text);
        bool append_remote_message(const char* text);

        void clear_messages();
        int get_message_count() const;

        void arrange(int x, int y, int width, int height);
        void scroll_lines(int line_count);
        void scroll_to_bottom();

    private:
        struct MessageEntry {
            MessageRole role;
            Label* label;
        };

        static void on_scroll_up(Button* button, void* context);
        static void on_scroll_down(Button* button, void* context);

        void relayout();
        void clamp_first_visible_index();
        int calculate_visible_capacity() const;
        const char* get_role_prefix(MessageRole role) const;
        Color get_role_color(MessageRole role) const;

        std::vector<MessageEntry> messages;
        Button scroll_up_button;
        Button scroll_down_button;
        int first_visible_index;
        int visible_capacity;
        int row_height;
        int row_spacing;
        int content_padding;
        int scroll_button_width;
};
