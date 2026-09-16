// =================================================================================
// Filename:    app/AttachmentChip.cpp
// Author:      Ebdsaleh
// Description: Implements one removable composer attachment chip.
// =================================================================================

#include <string.h>

#include "AttachmentChip.h"
#include "Attachment.h"

AttachmentChip::AttachmentChip()
    : remove_handler(0),
      remove_context(0) {

    get_style().background_color = Color(244, 248, 252);
    get_style().border_color = Color(154, 181, 203);
    get_style().border_width = 1;

    file_name_label.set_horizontal_alignment(Label::align_left);
    file_name_label.get_style().foreground_color = Color(42, 75, 105);

    remove_button.set_text("x");
    remove_button.get_style().background_color = Color(236, 241, 246);
    remove_button.get_style().foreground_color = Color(92, 66, 66);
    remove_button.get_style().border_color = Color(154, 181, 203);
    remove_button.set_click_handler(
        AttachmentChip::on_remove_clicked,
        this
    );

    add_child(&file_name_label);
    add_child(&remove_button);
}

void AttachmentChip::set_path(const char* new_path) {
    path = new_path == 0 ? "" : new_path;

    Attachment attachment(path.c_str());
    file_name = attachment.get_file_name();

    if (file_name.empty()) {
        file_name = path;
    }

    file_name_label.set_text(file_name.c_str());
}

const char* AttachmentChip::get_path() const {
    return path.c_str();
}

const char* AttachmentChip::get_file_name() const {
    return file_name.c_str();
}

void AttachmentChip::set_remove_handler(
    RemoveHandler new_handler,
    void* new_context
) {
    remove_handler = new_handler;
    remove_context = new_context;
}

int AttachmentChip::get_preferred_width() const {
    int text_length = (int)file_name.length();
    int preferred_width = 48 + (text_length * 7);

    if (preferred_width < 96) {
        preferred_width = 96;
    }

    if (preferred_width > 220) {
        preferred_width = 220;
    }

    return preferred_width;
}

void AttachmentChip::arrange(
    int x,
    int y,
    int width,
    int height
) {
    set_bounds(x, y, width, height);

    const int padding = 5;
    const int remove_width = 22;

    int content_height = height - 4;
    if (content_height < 0) {
        content_height = 0;
    }

    int label_width = width - (padding * 2) - remove_width - 3;
    if (label_width < 0) {
        label_width = 0;
    }

    file_name_label.set_bounds(
        x + padding,
        y + 2,
        label_width,
        content_height
    );

    int remove_x = x + width - padding - remove_width;
    if (remove_x < x + padding) {
        remove_x = x + padding;
    }

    remove_button.set_bounds(
        remove_x,
        y + 3,
        remove_width,
        height - 6
    );
}

void AttachmentChip::on_remove_clicked(
    Button* button,
    void* context
) {
    (void)button;

    AttachmentChip* chip = (AttachmentChip*)context;
    if (chip == 0 || chip->remove_handler == 0) {
        return;
    }

    chip->remove_handler(chip, chip->remove_context);
}
