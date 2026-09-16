// =================================================================================
// Filename:    framework/ContextMenu.h
// Author:      Ebdsaleh
// Description: Declares a backend-neutral popup context-menu model.
// =================================================================================
#pragma once

#include <string>
#include <vector>

class ContextMenu {
    public:
        struct Item {
            Item()
                : command_id(0),
                  enabled(true),
                  separator(false) {
            }

            int command_id;
            std::string text;
            bool enabled;
            bool separator;
        };

        void clear() {
            items.clear();
        }

        void add_item(
            int command_id,
            const char* text,
            bool enabled = true
        ) {
            Item item;
            item.command_id = command_id;
            item.text = text == 0 ? "" : text;
            item.enabled = enabled;
            item.separator = false;
            items.push_back(item);
        }

        void add_separator() {
            Item item;
            item.command_id = 0;
            item.enabled = false;
            item.separator = true;
            items.push_back(item);
        }

        int get_item_count() const {
            return (int)items.size();
        }

        const Item* get_item(int index) const {
            if (index < 0 || index >= (int)items.size()) {
                return 0;
            }

            return &items[index];
        }

    private:
        std::vector<Item> items;
};
