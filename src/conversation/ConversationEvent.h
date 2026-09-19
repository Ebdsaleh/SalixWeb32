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

    private:
        Type type;
        unsigned long request_id;
        std::string text;
};
