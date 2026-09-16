// =================================================================================
// Filename:    framework/TextViewportState.cpp
// Author:      Ebdsaleh
// Description: Implements shared viewport offsets for custom-rendered text inputs.
// =================================================================================

#include <vector>

#include "TextViewportState.h"
#include "TextInput.h"

namespace {
    struct ViewportEntry {
        ViewportEntry()
            : text_input(0),
              scroll_x(0),
              scroll_y(0) {
        }

        const TextInput* text_input;
        int scroll_x;
        int scroll_y;
    };

    std::vector<ViewportEntry>& entries() {
        static std::vector<ViewportEntry> viewport_entries;
        return viewport_entries;
    }

    int find_entry(const TextInput* text_input) {
        std::vector<ViewportEntry>& viewport_entries = entries();

        for (int index = 0;
             index < (int)viewport_entries.size();
             ++index) {
            if (viewport_entries[index].text_input == text_input) {
                return index;
            }
        }

        return -1;
    }
}

void TextViewportState::set_scroll(
    const TextInput* text_input,
    int scroll_x,
    int scroll_y
) {
    if (text_input == 0) {
        return;
    }

    if (scroll_x < 0) {
        scroll_x = 0;
    }
    if (scroll_y < 0) {
        scroll_y = 0;
    }

    std::vector<ViewportEntry>& viewport_entries = entries();
    int index = find_entry(text_input);

    if (index < 0) {
        ViewportEntry entry;
        entry.text_input = text_input;
        entry.scroll_x = scroll_x;
        entry.scroll_y = scroll_y;
        viewport_entries.push_back(entry);
        return;
    }

    viewport_entries[index].scroll_x = scroll_x;
    viewport_entries[index].scroll_y = scroll_y;
}

void TextViewportState::get_scroll(
    const TextInput* text_input,
    int& scroll_x,
    int& scroll_y
) {
    scroll_x = 0;
    scroll_y = 0;

    int index = find_entry(text_input);
    if (index < 0) {
        return;
    }

    std::vector<ViewportEntry>& viewport_entries = entries();
    scroll_x = viewport_entries[index].scroll_x;
    scroll_y = viewport_entries[index].scroll_y;
}

void TextViewportState::clear(const TextInput* text_input) {
    int index = find_entry(text_input);
    if (index < 0) {
        return;
    }

    std::vector<ViewportEntry>& viewport_entries = entries();
    viewport_entries.erase(viewport_entries.begin() + index);
}
