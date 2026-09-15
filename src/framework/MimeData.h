// =================================================================================
// Filename:    framework/MimeData.h
// Author:      Ebdsaleh
// Description: Declares backend-neutral MIME-tagged data used by clipboard/input paths.
// =================================================================================
#pragma once

#include <string>
#include <vector>

class MimeData {
    public:
        MimeData();

        void clear();
        bool is_empty() const;

        void set_data(
            const char* mime_type,
            const char* data,
            int data_size
        );

        bool has_format(const char* mime_type) const;
        const char* get_data(const char* mime_type) const;
        int get_data_size(const char* mime_type) const;

        void set_text(const char* text);
        const char* get_text() const;

    private:
        struct Item {
            std::string mime_type;
            std::string data;
        };

        int find_item_index(const char* mime_type) const;

        std::vector<Item> items;
};
