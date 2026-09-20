// =================================================================================
// Filename:    app/ConversationView.h
// Author:      Ebdsaleh
// Description: Declares the append-only scrollable conversation message surface.
// =================================================================================
#pragma once

#include <vector>

#include "Attachment.h"
#include "framework/Panel.h"
#include "framework/FormattedText.h"
#include "framework/ImageView.h"
#include "framework/ScrollBar.h"

class Clipboard;
class ComponentRenderer;
class ConversationMessageView;
class DesktopServices;
class NativeControlHost;
class TextMetrics;
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

        void set_desktop_services(DesktopServices* services);

        bool append_message(MessageRole role, const char* text);
        bool append_message(MessageRole role, const FormattedText& text);
        bool append_system_message(const char* text);
        bool append_local_message(const char* text);
        bool append_local_message(const FormattedText& text);
        bool append_remote_message(const char* text);
        bool append_remote_message(const FormattedText& text);
        bool update_message(int index, const char* text);
        bool update_message(int index, const FormattedText& text);
        bool append_attachment(const Attachment& attachment);
        bool append_attachment(
            MessageRole role,
            const Attachment& attachment
        );

        void clear_messages();
        int get_message_count() const;

        void attach_native_controls(NativeControlHost* control_host);
        void detach_native_controls();

        void arrange(
            int x,
            int y,
            int width,
            int height,
            TextMetrics* text_metrics = 0
        );
        void scroll_lines(int line_count);
        void scroll_pixels(int pixel_count);
        void scroll_to_bottom();

        virtual bool handle_event(const UIEvent& event);
        virtual void render(ComponentRenderer& renderer) const;

    private:
        struct MessageEntry {
            MessageEntry()
                : role(message_system),
                  view(0),
                  image_view(0),
                  text_height(0),
                  row_height(0) {
            }

            MessageRole role;
            FormattedText source_text;
            ConversationMessageView* view;
            ImageView* image_view;
            Attachment attachment;
            int text_height;
            int row_height;
        };

        static void on_scroll_changed(
            ScrollBar* scroll_bar,
            int value,
            void* context
        );

        void relayout();
        void recalculate_entry_heights(
            int message_width,
            TextMetrics* text_metrics
        );
        void clamp_scroll_offset();
        void update_scrollbar_state(int available_height);
        void sync_native_scrollbar();
        int calculate_entry_height(
            ConversationMessageView& message_view,
            int message_width,
            TextMetrics* text_metrics
        ) const;
        int calculate_total_content_height() const;
        int calculate_max_scroll_offset() const;

        int find_message_at_point(int x, int y) const;
        int resolve_message_index_for_selection(int y) const;
        bool is_attachment_point(int message_index, int x, int y) const;
        void clear_conversation_selection();
        void select_all_conversation();
        bool has_conversation_selection() const;
        bool copy_conversation_selection(Clipboard* clipboard) const;
        void apply_conversation_selection(
            int target_message_index,
            int target_label_index,
            int target_character_index,
            bool target_is_attachment
        );
        bool handle_context_menu(const UIEvent& event);
        bool handle_attachment_context_menu(
            const UIEvent& event,
            int message_index
        );
        const char* get_role_context_name(MessageRole role) const;

        const char* get_role_prefix(MessageRole role) const;
        const char* get_role_label(MessageRole role) const;
        Color get_role_color(MessageRole role) const;

        std::vector<MessageEntry> messages;
        ScrollBar vertical_scroll_bar;
        NativeControlHost* native_control_host;
        DesktopServices* desktop_services;
        int scroll_offset_y;
        int row_spacing;
        int content_padding;
        int scroll_bar_width;
        int line_step_pixels;
        int last_message_width;
        bool layout_dirty;

        bool conversation_drag_selecting;
        bool selection_context_active;
        bool selection_anchor_is_attachment;
        int selection_anchor_message_index;
        int selection_anchor_label_index;
        int selection_anchor_character_index;
};
