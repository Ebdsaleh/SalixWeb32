// =================================================================================
// Filename:    conversation/ConversationRequest.cpp
// Author:      Ebdsaleh
// Description: Implements the backend-neutral semantic conversation request.
// =================================================================================

#include "ConversationRequest.h"

ConversationRequest::ConversationRequest() {
}

void ConversationRequest::clear() {
    text.clear();
    attachment_paths.clear();
}

void ConversationRequest::set_text(const char* new_text) {
    text = new_text == 0 ? "" : new_text;
}

const char* ConversationRequest::get_text() const {
    return text.c_str();
}

void ConversationRequest::add_attachment_path(const char* path) {
    if (path == 0 || path[0] == '\0') {
        return;
    }

    attachment_paths.push_back(path);
}

int ConversationRequest::get_attachment_count() const {
    return (int)attachment_paths.size();
}

const char* ConversationRequest::get_attachment_path(int index) const {
    if (index < 0 || index >= (int)attachment_paths.size()) {
        return "";
    }

    return attachment_paths[index].c_str();
}

bool ConversationRequest::empty() const {
    return text.empty() && attachment_paths.empty();
}
