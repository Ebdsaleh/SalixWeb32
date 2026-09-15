// =================================================================================
// Filename:    app/ListPanel.cpp
// Author:      Ebdsaleh
// Description: Implements the paragraph-list popup used by the message composer.
// =================================================================================

#include "ListPanel.h"

ListPanel::ListPanel()
    : selected_handler(0),
      selected_context(0) {

    get_style().background_color = Color(250, 253, 255);
    get_style().border_color = Color(135, 169, 197);
    get_style().border_width = 1;

    bullet_button.set_text("Bullets");
    numbered_button.set_text("Numbered");
    clear_button.set_text("Clear list");

    bullet_button.get_style().background_color = Color(242, 248, 252);
    numbered_button.get_style().background_color = Color(242, 248, 252);
    clear_button.get_style().background_color = Color(242, 248, 252);

    bullet_button.get_style().foreground_color = Color(34, 68, 97);
    numbered_button.get_style().foreground_color = Color(34, 68, 97);
    clear_button.get_style().foreground_color = Color(34, 68, 97);

    bullet_button.get_style().border_color = Color(165, 190, 210);
    numbered_button.get_style().border_color = Color(165, 190, 210);
    clear_button.get_style().border_color = Color(165, 190, 210);

    bullet_button.set_click_handler(ListPanel::on_button_clicked, this);
    numbered_button.set_click_handler(ListPanel::on_button_clicked, this);
    clear_button.set_click_handler(ListPanel::on_button_clicked, this);

    add_child(&bullet_button);
    add_child(&numbered_button);
    add_child(&clear_button);

    set_visible(false);
}

void ListPanel::set_list_selected_handler(
    ListSelectedHandler new_handler,
    void* new_context
) {
    selected_handler = new_handler;
    selected_context = new_context;
}

void ListPanel::set_open(bool new_is_open) {
    set_visible(new_is_open);
}

bool ListPanel::get_is_open() const {
    return get_is_visible();
}

void ListPanel::arrange(int x, int y, int width, int height) {
    const int padding = 5;
    const int gap = 3;

    set_bounds(x, y, width, height);

    int inner_width = width - (padding * 2);
    int inner_height = height - (padding * 2) - (gap * 2);

    if (inner_width < 0) {
        inner_width = 0;
    }

    if (inner_height < 0) {
        inner_height = 0;
    }

    int row_height = inner_height / 3;
    int current_y = y + padding;

    bullet_button.set_bounds(
        x + padding,
        current_y,
        inner_width,
        row_height
    );

    current_y += row_height + gap;

    numbered_button.set_bounds(
        x + padding,
        current_y,
        inner_width,
        row_height
    );

    current_y += row_height + gap;

    clear_button.set_bounds(
        x + padding,
        current_y,
        inner_width,
        row_height
    );
}

void ListPanel::on_button_clicked(Button* button, void* context) {
    ListPanel* panel = (ListPanel*)context;

    if (panel == 0 || button == 0) {
        return;
    }

    if (button == &panel->bullet_button) {
        panel->select_style(list_bulleted);
    } else if (button == &panel->numbered_button) {
        panel->select_style(list_numbered);
    } else if (button == &panel->clear_button) {
        panel->select_style(list_clear);
    }
}

void ListPanel::select_style(ListStyle style) {
    if (selected_handler != 0) {
        selected_handler(this, style, selected_context);
    }

    set_open(false);
}
