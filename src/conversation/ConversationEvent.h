// =================================================================================
// Filename:    conversation/ConversationEvent.h
// Author:      Ebdsaleh
// Description: Declares semantic events emitted by a conversation service backend.
// =================================================================================
#pragma once

#include <string>

class ConversationEvent {
    public:
        enum Type {
            event_none = 0,
            event_request_started,
            event_message_started,
            event_text_delta,
            event_attachment,
            event_message_completed,
            event_request_failed
        };

        ConversationEvent();

        void clear();

        void set_type(Type new_type);
        Type get_type() const;

        void set_request_id(unsigned long new_request_id);
        unsigned long get_request_id() const;

        void set_text(const char* new_text);
        const char* get_text() const;

        void set_attachment(
            const char* name,
            const char* mime_type,
            const std::string& data
        );
        const char* get_attachment_name() const;
        const char* get_attachment_mime_type() const;
        const std::string& get_attachment_data() const;

    private:
        Type type;
        unsigned long request_id;
        std::string text;
        std::string attachment_name;
        std::string attachment_mime_type;
        std::string attachment_data;
};
