// =================================================================================
// Filename:    app/MessageDraft.cpp
// Author:      Ebdsaleh
// Description: Implements a composer message payload before transport handoff.
// =================================================================================

#include "MessageDraft.h"

MessageDraft::MessageDraft() {
}

void MessageDraft::clear() {
    body.clear();
    attachment_paths.clear();
}

void MessageDraft::set_body(const FormattedText& new_body) {
    body = new_body;
}

const FormattedText& MessageDraft::get_body() const {
    return body;
}

void MessageDraft::add_attachment(const char* path) {
    if (path == 0 || path[0] == '\0') {
        return;
    }

    attachment_paths.push_back(std::string(path));
}

int MessageDraft::get_attachment_count() const {
    return (int)attachment_paths.size();
}

const char* MessageDraft::get_attachment_path(int index) const {
    if (index < 0 || index >= (int)attachment_paths.size()) {
        return "";
    }

    return attachment_paths[index].c_str();
}

bool MessageDraft::empty() const {
    return body.empty() && attachment_paths.empty();
}
