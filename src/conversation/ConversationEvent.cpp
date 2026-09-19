// =================================================================================
// Filename:    conversation/ConversationEvent.cpp
// Author:      Ebdsaleh
// Description: Implements semantic conversation service events.
// =================================================================================

#include "ConversationEvent.h"

ConversationEvent::ConversationEvent()
    : type(event_none),
      request_id(0) {
}

void ConversationEvent::clear() {
    type = event_none;
    request_id = 0;
    text.clear();
    attachment_name.clear();
    attachment_mime_type.clear();
    attachment_data.clear();
}

void ConversationEvent::set_type(Type new_type) {
    type = new_type;
}

ConversationEvent::Type ConversationEvent::get_type() const {
    return type;
}

void ConversationEvent::set_request_id(unsigned long new_request_id) {
    request_id = new_request_id;
}

unsigned long ConversationEvent::get_request_id() const {
    return request_id;
}

void ConversationEvent::set_text(const char* new_text) {
    text = new_text == 0 ? "" : new_text;
}

const char* ConversationEvent::get_text() const {
    return text.c_str();
}

void ConversationEvent::set_attachment(
    const char* name,
    const char* mime_type,
    const std::string& data
) {
    attachment_name = name == 0 ? "" : name;
    attachment_mime_type =
        mime_type == 0 ? "" : mime_type;
    attachment_data = data;
}

const char* ConversationEvent::get_attachment_name() const {
    return attachment_name.c_str();
}

const char* ConversationEvent::get_attachment_mime_type() const {
    return attachment_mime_type.c_str();
}

const std::string& ConversationEvent::get_attachment_data() const {
    return attachment_data;
}
