// =================================================================================
// Filename:    app/Attachment.h
// Author:      Ebdsaleh
// Description: Declares semantic metadata for a message attachment.
// =================================================================================
#pragma once

#include <string>

class Attachment {
    public:
        enum Kind {
            attachment_generic = 0,
            attachment_image
        };

        Attachment();
        Attachment(const char* path);

        void set_path(const char* path);

        const char* get_path() const;
        const char* get_file_name() const;
        Kind get_kind() const;
        bool is_image() const;
        bool empty() const;

    private:
        void refresh_metadata();

        std::string path;
        std::string file_name;
        Kind kind;
};
