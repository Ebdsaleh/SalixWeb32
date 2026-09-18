// =================================================================================
// Filename:    conversation/ConversationRequest.h
// Author:      Ebdsaleh
// Description: Declares a backend-neutral semantic conversation request.
// =================================================================================
#pragma once

#include <string>
#include <vector>

class ConversationRequest {
    public:
        ConversationRequest();

        void clear();

        void set_text(const char* new_text);
        const char* get_text() const;

        void add_attachment_path(const char* path);
        int get_attachment_count() const;
        const char* get_attachment_path(int index) const;

        bool empty() const;

    private:
        std::string text;
        std::vector<std::string> attachment_paths;
};
