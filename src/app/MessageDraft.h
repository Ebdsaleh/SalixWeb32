// =================================================================================
// Filename:    app/MessageDraft.h
// Author:      Ebdsaleh
// Description: Declares a composer message payload before transport/backend handoff.
// =================================================================================
#pragma once

#include <vector>

#include "Attachment.h"
#include "framework/FormattedText.h"

class MessageDraft {
    public:
        MessageDraft();

        void clear();

        void set_body(const FormattedText& new_body);
        const FormattedText& get_body() const;

        void add_attachment(const char* path);
        int get_attachment_count() const;
        const Attachment& get_attachment(int index) const;
        const char* get_attachment_path(int index) const;

        bool empty() const;

    private:
        FormattedText body;
        std::vector<Attachment> attachments;
};
