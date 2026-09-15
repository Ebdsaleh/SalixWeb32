// =================================================================================
// Filename:    framework/ComboBox.cpp
// Author:      Ebdsaleh
// Description: Implements a lightweight backend-neutral combo-box component.
// =================================================================================

#include "ComboBox.h"
#include "UIEvent.h"

ComboBox::Item::Item()
    : value(0),
      button(0) {
}

ComboBox::ComboBox()
    : selected_index(-1),
      is_open(false),
      is_enabled(true),
      drop_direction(drop_down),
      selection_changed_handler(0),
      selection_changed_context(0) {

    main_button.set_text("v");
    main_button.get_style().background_color = Color(238, 245, 251);
    main_button.get_style().border_color = Color(132, 157, 181);
    add_child(&main_button);
}

ComboBox::~ComboBox() {
    for (int index = 0; index < (int)items.size(); ++index) {
        if (items[index].button != 0) {
            remove_child(items[index].button);
            delete items[index].button;
            items[index].button = 0;
        }
    }

    items.clear();
}

void ComboBox::add_item(const char* text, int value) {
    Item item;
    item.text = text == 0 ? "" : text;
    item.value = value;
    item.button = new Button();

    if (item.button == 0) {
        return;
    }

    item.button->set_text(item.text.c_str());
    item.button->get_style().background_color = Color(250, 252, 254);
    item.button->get_style().border_color = Color(160, 181, 199);
    item.button->set_visible(false);

    items.push_back(item);
    add_child(item.button);

    if (selected_index < 0) {
        selected_index = 0;
        update_main_button_text();
    }
}

int ComboBox::get_item_count() const {
    return (int)items.size();
}

void ComboBox::set_selected_index(int new_selected_index) {
    if (
        new_selected_index < 0 ||
        new_selected_index >= (int)items.size()
    ) {
        return;
    }

    selected_index = new_selected_index;
    update_main_button_text();
}

int ComboBox::get_selected_index() const {
    return selected_index;
}

int ComboBox::get_selected_value() const {
    if (selected_index < 0 || selected_index >= (int)items.size()) {
        return 0;
    }

    return items[selected_index].value;
}

const char* ComboBox::get_selected_text() const {
    if (selected_index < 0 || selected_index >= (int)items.size()) {
        return "";
    }

    return items[selected_index].text.c_str();
}

void ComboBox::set_drop_direction(DropDirection new_drop_direction) {
    drop_direction = new_drop_direction;
}

ComboBox::DropDirection ComboBox::get_drop_direction() const {
    return drop_direction;
}

void ComboBox::set_open(bool new_is_open) {
    is_open = new_is_open && is_enabled && !items.empty();
    update_option_visibility();
}

bool ComboBox::get_is_open() const {
    return is_open;
}

bool ComboBox::contains_open_popup_point(int x, int y) const {
    return is_open && contains_option_point(x, y);
}

void ComboBox::set_enabled(bool new_is_enabled) {
    is_enabled = new_is_enabled;
    main_button.set_enabled(is_enabled);

    for (int index = 0; index < (int)items.size(); ++index) {
        if (items[index].button != 0) {
            items[index].button->set_enabled(is_enabled);
        }
    }

    if (!is_enabled) {
        set_open(false);
    }
}

bool ComboBox::get_is_enabled() const {
    return is_enabled;
}

void ComboBox::set_selection_changed_handler(
    SelectionChangedHandler new_handler,
    void* new_context
) {
    selection_changed_handler = new_handler;
    selection_changed_context = new_context;
}

void ComboBox::arrange(int x, int y, int width, int height) {
    set_bounds(x, y, width, height);
    main_button.set_bounds(x, y, width, height);

    int item_count = (int)items.size();

    for (int index = 0; index < item_count; ++index) {
        if (items[index].button == 0) {
            continue;
        }

        int option_y;

        if (drop_direction == drop_up) {
            option_y = y - ((item_count - index) * height);
        } else {
            option_y = y + ((index + 1) * height);
        }

        items[index].button->set_bounds(
            x,
            option_y,
            width,
            height
        );
    }
}

bool ComboBox::handle_event(const UIEvent& event) {
    if (!get_is_visible() || !is_enabled) {
        return false;
    }

    bool main_was_pressed = main_button.get_is_pressed();
    int pressed_option_index = -1;

    for (int index = 0; index < (int)items.size(); ++index) {
        if (
            items[index].button != 0 &&
            items[index].button->get_is_pressed()
        ) {
            pressed_option_index = index;
            break;
        }
    }

    if (
        event.type == UIEvent::event_mouse_down &&
        is_open &&
        !main_button.contains_point(event.x, event.y) &&
        !contains_option_point(event.x, event.y)
    ) {
        set_open(false);
    }

    bool was_handled = Container::handle_event(event);

    if (event.type != UIEvent::event_mouse_up) {
        return was_handled;
    }

    if (
        main_was_pressed &&
        main_button.contains_point(event.x, event.y)
    ) {
        set_open(!is_open);
        return true;
    }

    if (
        is_open &&
        pressed_option_index >= 0 &&
        pressed_option_index < (int)items.size() &&
        items[pressed_option_index].button != 0 &&
        items[pressed_option_index].button->contains_point(event.x, event.y)
    ) {
        set_selected_index(pressed_option_index);
        set_open(false);

        if (selection_changed_handler != 0) {
            selection_changed_handler(
                this,
                get_selected_value(),
                get_selected_text(),
                selection_changed_context
            );
        }

        return true;
    }

    return was_handled;
}

void ComboBox::update_main_button_text() {
    std::string display_text;

    if (selected_index >= 0 && selected_index < (int)items.size()) {
        display_text = items[selected_index].text;
        display_text += " v";
    } else {
        display_text = "v";
    }

    main_button.set_text(display_text.c_str());
}

void ComboBox::update_option_visibility() {
    for (int index = 0; index < (int)items.size(); ++index) {
        if (items[index].button != 0) {
            items[index].button->set_visible(is_open);
        }
    }
}

bool ComboBox::contains_option_point(int x, int y) const {
    for (int index = 0; index < (int)items.size(); ++index) {
        if (
            items[index].button != 0 &&
            items[index].button->get_is_visible() &&
            items[index].button->contains_point(x, y)
        ) {
            return true;
        }
    }

    return false;
}
