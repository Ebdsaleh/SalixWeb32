// =================================================================================
// Filename:    app/MessageDraft.cpp
// Author:      Ebdsaleh
// Description: Implements a composer message payload before transport handoff.
// =================================================================================

#include "MessageDraft.h"

namespace {
    const Attachment empty_attachment;
}

MessageDraft::MessageDraft() {
}

void MessageDraft::clear() {
    body.clear();
    attachments.clear();
}

void MessageDraft::set_body(const FormattedText& new_body) {
    body = new_body;
}

const FormattedText& MessageDraft::get_body() const {
    return body;
}

void MessageDraft::add_attachment(const char* path) {
    Attachment attachment(path);
    if (!attachment.empty()) {
        attachments.push_back(attachment);
    }
}

int MessageDraft::get_attachment_count() const {
    return (int)attachments.size();
}

const Attachment& MessageDraft::get_attachment(int index) const {
    if (index < 0 || index >= (int)attachments.size()) {
        return empty_attachment;
    }

    return attachments[index];
}

const char* MessageDraft::get_attachment_path(int index) const {
    return get_attachment(index).get_path();
}

bool MessageDraft::empty() const {
    return body.empty() && attachments.empty();
}
