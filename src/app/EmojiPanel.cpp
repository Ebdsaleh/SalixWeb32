// =================================================================================
// Filename:    app/EmojiPanel.cpp
// Author:      Ebdsaleh
// Description: Implements the classic emoticon picker popup used by the composer.
// =================================================================================

#include "EmojiPanel.h"
#include "EmoticonRegistry.h"

EmojiPanel::EmojiPanel()
    : selected_handler(0),
      selected_context(0) {

    get_style().background_color = Color(250, 253, 255);
    get_style().border_color = Color(135, 169, 197);
    get_style().border_width = 1;

    int emoticon_count = EmoticonRegistry::get_count();

    for (int index = 0; index < emoticon_count; ++index) {
        Button* button = new Button();

        if (button == 0) {
            continue;
        }

        button->set_text(EmoticonRegistry::get_alias(index));
        button->get_style().background_color = Color(242, 248, 252);
        button->get_style().foreground_color = Color(34, 68, 97);
        button->get_style().border_color = Color(165, 190, 210);
        button->set_click_handler(
            EmojiPanel::on_emoticon_clicked,
            this
        );

        emoticon_buttons.push_back(button);
        add_child(button);
    }

    set_visible(false);
}

EmojiPanel::~EmojiPanel() {
    for (int index = 0; index < (int)emoticon_buttons.size(); ++index) {
        if (emoticon_buttons[index] != 0) {
            remove_child(emoticon_buttons[index]);
            delete emoticon_buttons[index];
        }
    }

    emoticon_buttons.clear();
}

void EmojiPanel::set_emoticon_selected_handler(
    EmoticonSelectedHandler new_handler,
    void* new_context
) {
    selected_handler = new_handler;
    selected_context = new_context;
}

void EmojiPanel::set_open(bool new_is_open) {
    set_visible(new_is_open);
}

bool EmojiPanel::get_is_open() const {
    return get_is_visible();
}

void EmojiPanel::arrange(int x, int y, int width, int height) {
    const int padding = 5;
    const int gap = 4;
    const int columns = 4;

    set_bounds(x, y, width, height);

    int button_count = (int)emoticon_buttons.size();
    int rows = (button_count + columns - 1) / columns;

    if (rows <= 0) {
        return;
    }

    int usable_width = width - (padding * 2) - (gap * (columns - 1));
    int usable_height = height - (padding * 2) - (gap * (rows - 1));

    if (usable_width < 0) {
        usable_width = 0;
    }

    if (usable_height < 0) {
        usable_height = 0;
    }

    int button_width = usable_width / columns;
    int button_height = usable_height / rows;

    for (int index = 0; index < button_count; ++index) {
        int column = index % columns;
        int row = index / columns;

        if (emoticon_buttons[index] == 0) {
            continue;
        }

        emoticon_buttons[index]->set_bounds(
            x + padding + (column * (button_width + gap)),
            y + padding + (row * (button_height + gap)),
            button_width,
            button_height
        );
    }
}

void EmojiPanel::on_emoticon_clicked(
    Button* button,
    void* context
) {
    EmojiPanel* panel = (EmojiPanel*)context;

    if (panel == 0 || button == 0) {
        return;
    }

    if (panel->selected_handler != 0) {
        panel->selected_handler(
            panel,
            button->get_text(),
            panel->selected_context
        );
    }

    panel->set_open(false);
}
