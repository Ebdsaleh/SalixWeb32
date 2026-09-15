// =================================================================================
// Filename:    app/MessageInputStrip.h
// Author:      Ebdsaleh
// Description: Declares the composite message-entry strip used by the shell.
// =================================================================================
#pragma once

#include "framework/Panel.h"
#include "framework/TextInput.h"
#include "framework/Button.h"

class MimeData;

class MessageInputStrip : public Panel {
    public:
        typedef void (*SubmitHandler)(
            MessageInputStrip* input_strip,
            const char* text,
            void* context
        );

        MessageInputStrip();

        void set_text(const char* new_text);
        const char* get_text() const;
        void clear();

        void set_button_text(const char* new_text);
        void set_max_length(int new_max_length);

        void set_submit_on_enter(bool new_submit_on_enter);
        bool get_submit_on_enter() const;

        bool accepts_mime_type(const char* mime_type) const;
        bool insert_mime_data(const MimeData& data);

        void set_submit_handler(
            SubmitHandler new_submit_handler,
            void* new_context
        );

        void arrange(int x, int y, int width, int height);

        virtual bool handle_event(const UIEvent& event);

    private:
        static void on_button_clicked(Button* button, void* context);
        void submit();

        TextInput message_input;
        Button send_button;
        bool submit_on_enter;
        SubmitHandler submit_handler;
        void* submit_context;
};
