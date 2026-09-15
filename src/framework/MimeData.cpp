// =================================================================================
// Filename:    framework/MimeData.cpp
// Author:      Ebdsaleh
// Description: Implements backend-neutral MIME-tagged data storage.
// =================================================================================

#include <string.h>

#include "MimeData.h"

MimeData::MimeData() {
}

void MimeData::clear() {
    items.clear();
}

bool MimeData::is_empty() const {
    return items.empty();
}

void MimeData::set_data(
    const char* mime_type,
    const char* data,
    int data_size
) {
    if (mime_type == 0 || mime_type[0] == '\0') {
        return;
    }

    if (data_size < 0) {
        data_size = 0;
    }

    if (data == 0 && data_size > 0) {
        return;
    }

    int item_index = find_item_index(mime_type);

    if (item_index < 0) {
        Item item;
        item.mime_type = mime_type;
        items.push_back(item);
        item_index = (int)items.size() - 1;
    }

    if (data == 0 || data_size == 0) {
        items[item_index].data.clear();
        return;
    }

    items[item_index].data.assign(data, data_size);
}

bool MimeData::has_format(const char* mime_type) const {
    return find_item_index(mime_type) >= 0;
}

const char* MimeData::get_data(const char* mime_type) const {
    int item_index = find_item_index(mime_type);
    if (item_index < 0) {
        return 0;
    }

    return items[item_index].data.c_str();
}

int MimeData::get_data_size(const char* mime_type) const {
    int item_index = find_item_index(mime_type);
    if (item_index < 0) {
        return 0;
    }

    return (int)items[item_index].data.size();
}

void MimeData::set_text(const char* text) {
    if (text == 0) {
        set_data("text/plain", "", 0);
        return;
    }

    set_data(
        "text/plain",
        text,
        (int)strlen(text)
    );
}

const char* MimeData::get_text() const {
    return get_data("text/plain");
}

int MimeData::find_item_index(const char* mime_type) const {
    if (mime_type == 0) {
        return -1;
    }

    for (int index = 0; index < (int)items.size(); ++index) {
        if (items[index].mime_type == mime_type) {
            return index;
        }
    }

    return -1;
}
