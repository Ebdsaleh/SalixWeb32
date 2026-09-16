// =================================================================================
// Filename:    app/MessageInputStrip.h
// Author:      Ebdsaleh
// Description: Declares the composite message-entry strip used by the shell.
// =================================================================================
#pragma once

#include "framework/Panel.h"
#include "framework/TextInput.h"
#include "framework/Button.h"
#include "framework/ScrollBar.h"

class FormattedText;
class MimeData;
class NativeControlHost;
class TextMetrics;

class MessageInputStrip : public Panel {
    public:
        typedef void (*SubmitHandler)(
            MessageInputStrip* input_strip,
            const char* text,
            void* context
        );

        MessageInputStrip();
        virtual ~MessageInputStrip();

        void set_text(const char* new_text);
        const char* get_text() const;
        void get_formatted_text(FormattedText& formatted_text) const;
        void clear();

        void set_button_text(const char* new_text);
        void set_max_length(int new_max_length);

        void set_submit_on_enter(bool new_submit_on_enter);
        bool get_submit_on_enter() const;

        void set_allow_empty_submit(bool new_allow_empty_submit);
        bool get_allow_empty_submit() const;

        void set_code_mode(bool new_code_mode);
        bool get_code_mode() const;

        void apply_code_style(bool enabled) {
            message_input.set_code_style(
                enabled
                    ? TextFormat::code_block
                    : TextFormat::code_none
            );
        }

        void set_tab_size(int new_tab_size);
        int get_tab_size() const;

        void set_text_format(
            bool bold,
            bool italic,
            bool underline,
            int font_size
        );

        bool apply_list_style(TextInput::ListStyle style);
        TextInput::ListStyle get_current_list_style() const;

        bool accepts_mime_type(const char* mime_type) const;
        bool insert_mime_data(const MimeData& data);

        void set_submit_handler(
            SubmitHandler new_submit_handler,
            void* new_context
        );

        void attach_native_controls(NativeControlHost* control_host);
        void detach_native_controls();

        void arrange(int x, int y, int width, int height);

        virtual bool handle_event(const UIEvent& event);

    private:
        static void on_button_clicked(Button* button, void* context);
        static void on_scroll_changed(
            ScrollBar* scroll_bar,
            int value,
            void* context
        );

        bool continue_current_list();
        bool insert_indentation();
        bool handle_text_input_event(const UIEvent& event);
        void update_scrollbars(TextMetrics* text_metrics);
        void ensure_caret_visible(TextMetrics* text_metrics);
        void estimate_content_extent(int& width, int& height) const;
        void apply_viewport_state();
        void sync_native_scrollbars();
        void submit();

        TextInput message_input;
        Button send_button;
        ScrollBar horizontal_scroll_bar;
        ScrollBar vertical_scroll_bar;
        Panel scroll_corner;
        NativeControlHost* native_control_host;

        bool submit_on_enter;
        bool allow_empty_submit;
        bool code_mode;
        int tab_size;

        SubmitHandler submit_handler;
        void* submit_context;
};
