// =================================================================================
// Filename:    app/AttachmentTray.cpp
// Author:      Ebdsaleh
// Description: Implements the removable attachment queue shown in the composer.
// =================================================================================

#include "AttachmentTray.h"

AttachmentTray::AttachmentTray()
    : attachment_removed_handler(0),
      attachment_removed_context(0),
      first_visible_index(0) {

    get_style().background_color = Color(235, 244, 251);
    get_style().border_color = Color(164, 190, 212);
    get_style().border_width = 1;

    previous_button.set_text("<");
    next_button.set_text(">");

    previous_button.get_style().background_color = Color(238, 245, 251);
    next_button.get_style().background_color = Color(238, 245, 251);
    previous_button.get_style().foreground_color = Color(42, 75, 105);
    next_button.get_style().foreground_color = Color(42, 75, 105);
    previous_button.get_style().border_color = Color(145, 173, 197);
    next_button.get_style().border_color = Color(145, 173, 197);

    previous_button.set_click_handler(
        AttachmentTray::on_previous_clicked,
        this
    );
    next_button.set_click_handler(
        AttachmentTray::on_next_clicked,
        this
    );

    previous_button.set_visible(false);
    next_button.set_visible(false);

    add_child(&previous_button);
    add_child(&next_button);
}

AttachmentTray::~AttachmentTray() {
    clear_chips();
}

void AttachmentTray::set_paths(
    const std::vector<std::string>& paths
) {
    rebuild_chips(paths);
    layout_chips();
}

void AttachmentTray::clear() {
    clear_chips();
    first_visible_index = 0;
    previous_button.set_visible(false);
    next_button.set_visible(false);
}

void AttachmentTray::set_attachment_removed_handler(
    AttachmentRemovedHandler new_handler,
    void* new_context
) {
    attachment_removed_handler = new_handler;
    attachment_removed_context = new_context;
}

int AttachmentTray::get_attachment_count() const {
    return (int)chips.size();
}

int AttachmentTray::get_preferred_height() const {
    return chips.empty() ? 0 : 30;
}

void AttachmentTray::arrange(
    int x,
    int y,
    int width,
    int height
) {
    set_bounds(x, y, width, height);
    layout_chips();
}

void AttachmentTray::on_chip_remove(
    AttachmentChip* chip,
    void* context
) {
    AttachmentTray* tray = (AttachmentTray*)context;
    if (tray == 0 || tray->attachment_removed_handler == 0) {
        return;
    }

    int index = tray->find_chip_index(chip);
    if (index < 0) {
        return;
    }

    tray->attachment_removed_handler(
        tray,
        index,
        tray->attachment_removed_context
    );
}

void AttachmentTray::on_previous_clicked(
    Button* button,
    void* context
) {
    (void)button;

    AttachmentTray* tray = (AttachmentTray*)context;
    if (tray == 0) {
        return;
    }

    if (tray->first_visible_index > 0) {
        --tray->first_visible_index;
        tray->layout_chips();
    }
}

void AttachmentTray::on_next_clicked(
    Button* button,
    void* context
) {
    (void)button;

    AttachmentTray* tray = (AttachmentTray*)context;
    if (tray == 0) {
        return;
    }

    if (
        tray->first_visible_index + 1 <
        (int)tray->chips.size()
    ) {
        ++tray->first_visible_index;
        tray->layout_chips();
    }
}

void AttachmentTray::rebuild_chips(
    const std::vector<std::string>& paths
) {
    clear_chips();

    for (int index = 0; index < (int)paths.size(); ++index) {
        if (paths[index].empty()) {
            continue;
        }

        AttachmentChip* chip = new AttachmentChip();
        if (chip == 0) {
            continue;
        }

        chip->set_path(paths[index].c_str());
        chip->set_remove_handler(
            AttachmentTray::on_chip_remove,
            this
        );

        if (!add_child(chip)) {
            delete chip;
            continue;
        }

        chips.push_back(chip);
    }

    if (chips.empty()) {
        first_visible_index = 0;
    } else if (first_visible_index >= (int)chips.size()) {
        first_visible_index = (int)chips.size() - 1;
    }
}

void AttachmentTray::layout_chips() {
    const int padding = 3;
    const int gap = 4;
    const int nav_width = 22;

    int width = get_width();
    int height = get_height();

    for (int index = 0; index < (int)chips.size(); ++index) {
        if (chips[index] != 0) {
            chips[index]->set_visible(false);
        }
    }

    if (chips.empty() || width <= 0 || height <= 0) {
        previous_button.set_visible(false);
        next_button.set_visible(false);
        return;
    }

    int available_width = width - (padding * 2);
    if (available_width < 0) {
        available_width = 0;
    }

    int total_width = 0;
    for (int index = 0; index < (int)chips.size(); ++index) {
        total_width += chips[index]->get_preferred_width();
        if (index + 1 < (int)chips.size()) {
            total_width += gap;
        }
    }

    bool needs_navigation = total_width > available_width;

    if (!needs_navigation) {
        first_visible_index = 0;
        previous_button.set_visible(false);
        next_button.set_visible(false);

        int cursor_x = get_x() + padding;
        int chip_y = get_y() + padding;
        int chip_height = height - (padding * 2);
        if (chip_height < 0) {
            chip_height = 0;
        }

        for (int index = 0; index < (int)chips.size(); ++index) {
            AttachmentChip* chip = chips[index];
            if (chip == 0) {
                continue;
            }

            int chip_width = chip->get_preferred_width();
            chip->set_visible(true);
            chip->arrange(
                cursor_x,
                chip_y,
                chip_width,
                chip_height
            );
            cursor_x += chip_width + gap;
        }

        return;
    }

    previous_button.set_visible(true);
    next_button.set_visible(true);

    previous_button.set_enabled(first_visible_index > 0);

    int button_y = get_y() + padding;
    int button_height = height - (padding * 2);
    if (button_height < 0) {
        button_height = 0;
    }

    previous_button.set_bounds(
        get_x() + padding,
        button_y,
        nav_width,
        button_height
    );
    next_button.set_bounds(
        get_x() + width - padding - nav_width,
        button_y,
        nav_width,
        button_height
    );

    int content_x = get_x() + padding + nav_width + gap;
    int content_right = get_x() + width - padding - nav_width - gap;
    int content_width = content_right - content_x;
    if (content_width < 0) {
        content_width = 0;
    }

    int cursor_x = content_x;
    int last_visible_index = first_visible_index - 1;

    for (
        int index = first_visible_index;
        index < (int)chips.size();
        ++index
    ) {
        AttachmentChip* chip = chips[index];
        if (chip == 0) {
            continue;
        }

        int remaining_width = content_right - cursor_x;
        if (remaining_width <= 0) {
            break;
        }

        int chip_width = chip->get_preferred_width();
        if (chip_width > remaining_width) {
            if (index != first_visible_index || remaining_width < 60) {
                break;
            }
            chip_width = remaining_width;
        }

        chip->set_visible(true);
        chip->arrange(
            cursor_x,
            get_y() + padding,
            chip_width,
            button_height
        );
        last_visible_index = index;
        cursor_x += chip_width + gap;
    }

    next_button.set_enabled(
        last_visible_index + 1 < (int)chips.size()
    );
}

void AttachmentTray::clear_chips() {
    for (int index = 0; index < (int)chips.size(); ++index) {
        AttachmentChip* chip = chips[index];
        if (chip != 0) {
            remove_child(chip);
            delete chip;
        }
    }

    chips.clear();
}

int AttachmentTray::find_chip_index(
    AttachmentChip* chip
) const {
    if (chip == 0) {
        return -1;
    }

    for (int index = 0; index < (int)chips.size(); ++index) {
        if (chips[index] == chip) {
            return index;
        }
    }

    return -1;
}
