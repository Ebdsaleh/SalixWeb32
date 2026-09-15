// =================================================================================
// Filename:    framework/ComboBox.h
// Author:      Ebdsaleh
// Description: Declares a lightweight backend-neutral combo-box component.
// =================================================================================
#pragma once

#include <string>
#include <vector>

#include "Container.h"
#include "Button.h"

class ComboBox : public Container {
    public:
        enum DropDirection {
            drop_down = 0,
            drop_up
        };

        typedef void (*SelectionChangedHandler)(
            ComboBox* combo_box,
            int selected_value,
            const char* selected_text,
            void* context
        );

        ComboBox();
        virtual ~ComboBox();

        void add_item(const char* text, int value);
        int get_item_count() const;

        void set_selected_index(int new_selected_index);
        int get_selected_index() const;
        int get_selected_value() const;
        const char* get_selected_text() const;

        void set_drop_direction(DropDirection new_drop_direction);
        DropDirection get_drop_direction() const;

        void set_open(bool new_is_open);
        bool get_is_open() const;
        bool contains_open_popup_point(int x, int y) const;

        void set_enabled(bool new_is_enabled);
        bool get_is_enabled() const;

        void set_selection_changed_handler(
            SelectionChangedHandler new_handler,
            void* new_context
        );

        void arrange(int x, int y, int width, int height);

        virtual bool handle_event(const UIEvent& event);

    private:
        struct Item {
            Item();

            std::string text;
            int value;
            Button* button;
        };

        void update_main_button_text();
        void update_option_visibility();
        bool contains_option_point(int x, int y) const;

        Button main_button;
        std::vector<Item> items;
        int selected_index;
        bool is_open;
        bool is_enabled;
        DropDirection drop_direction;
        SelectionChangedHandler selection_changed_handler;
        void* selection_changed_context;
};
