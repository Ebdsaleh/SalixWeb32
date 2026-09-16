// =================================================================================
// Filename:    app/Attachment.cpp
// Author:      Ebdsaleh
// Description: Implements semantic metadata for a message attachment.
// =================================================================================

#include <ctype.h>
#include <string>

#include "Attachment.h"

namespace {
    std::string lower_copy(const std::string& value) {
        std::string result(value);

        for (std::string::size_type index = 0; index < result.size(); ++index) {
            result[index] = (char)tolower((unsigned char)result[index]);
        }

        return result;
    }

    bool has_image_extension(const std::string& file_name) {
        std::string lower_name = lower_copy(file_name);
        std::string::size_type dot = lower_name.find_last_of('.');

        if (dot == std::string::npos) {
            return false;
        }

        std::string extension = lower_name.substr(dot);
        return
            extension == ".bmp" ||
            extension == ".gif" ||
            extension == ".jpg" ||
            extension == ".jpeg" ||
            extension == ".png" ||
            extension == ".tif" ||
            extension == ".tiff";
    }
}

Attachment::Attachment()
    : kind(attachment_generic) {
}

Attachment::Attachment(const char* new_path)
    : kind(attachment_generic) {
    set_path(new_path);
}

void Attachment::set_path(const char* new_path) {
    path = new_path == 0 ? "" : new_path;
    refresh_metadata();
}

const char* Attachment::get_path() const {
    return path.c_str();
}

const char* Attachment::get_file_name() const {
    return file_name.c_str();
}

Attachment::Kind Attachment::get_kind() const {
    return kind;
}

bool Attachment::is_image() const {
    return kind == attachment_image;
}

bool Attachment::empty() const {
    return path.empty();
}

void Attachment::refresh_metadata() {
    file_name.clear();
    kind = attachment_generic;

    if (path.empty()) {
        return;
    }

    std::string::size_type slash = path.find_last_of("\\/");
    file_name = slash == std::string::npos
        ? path
        : path.substr(slash + 1);

    if (has_image_extension(file_name)) {
        kind = attachment_image;
    }
}
