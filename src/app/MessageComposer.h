// =================================================================================
// Filename:    app/MessageComposer.h
// Author:      Ebdsaleh
// Description: Declares the complete messenger composer (toolbar + input strip).
// =================================================================================
#pragma once

#include <string>
#include <vector>

#include "framework/Panel.h"
#include "MessageToolbar.h"
#include "MessageInputStrip.h"

class FileDialog;

class MessageComposer : public Panel {
    public:
        typedef void (*SubmitHandler)(
            MessageComposer* composer,
            const char* text,
            void* context
        );

        MessageComposer(FileDialog* file_dialog);

        void set_text(const char* new_text);
        const char* get_text() const;
        void clear();

        void set_button_text(const char* new_text);
        void set_max_length(int new_max_length);
        void set_submit_on_enter(bool new_submit_on_enter);
        bool get_submit_on_enter() const;

        void set_submit_handler(
            SubmitHandler new_submit_handler,
            void* new_context
        );

        int get_attachment_count() const;
        const char* get_attachment_path(int index) const;

        bool get_bold() const;
        bool get_italic() const;
        bool get_underline() const;
        int get_font_size() const;

        void arrange(int x, int y, int width, int height);

        virtual bool handle_event(const UIEvent& event);

    private:
        static void on_input_submitted(
            MessageInputStrip* input_strip,
            const char* text,
            void* context
        );

        static void on_toolbar_insert_text(
            MessageToolbar* toolbar,
            const char* text,
            void* context
        );

        static void on_toolbar_attachments_added(
            MessageToolbar* toolbar,
            const std::vector<std::string>& paths,
            void* context
        );

        void add_attachments(const std::vector<std::string>& paths);
        void clear_attachments();

        MessageInputStrip message_input_strip;
        MessageToolbar message_toolbar;
        std::vector<std::string> attachment_paths;

        SubmitHandler submit_handler;
        void* submit_context;
};
