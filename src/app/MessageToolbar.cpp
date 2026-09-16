// =================================================================================
// Filename:    app/MessageToolbar.cpp
// Author:      Ebdsaleh
// Description: Implements the formatting/attachment toolbar above the message input.
// =================================================================================

#include <stdio.h>

#include "MessageToolbar.h"
#include "framework/FileDialog.h"
#include "framework/NativeControlHost.h"
#include "framework/UIEvent.h"

namespace {
    const int control_character_b = 2;
    const int control_character_i = 9;
    const int control_character_u = 21;
}

MessageToolbar::MessageToolbar(FileDialog* new_file_dialog)
    : file_dialog(new_file_dialog),
      native_control_host(0),
      insert_text_handler(0),
      insert_text_context(0),
      attachments_added_handler(0),
      attachments_added_context(0),
      format_changed_handler(0),
      format_changed_context(0),
      list_requested_handler(0),
      list_requested_context(0),
      font_size(12),
      list_style(ListPanel::list_clear) {

    get_style().background_color = Color(229, 240, 249);
    get_style().border_color = Color(147, 181, 211);
    get_style().border_width = 1;

    attach_button.set_text("+");
    attach_button.get_style().background_color = Color(238, 245, 251);
    attach_button.get_style().foreground_color = Color(26, 68, 108);
    attach_button.get_style().border_color = Color(132, 157, 181);

    bold_button.set_text("B");
    italic_button.set_text("I");
    underline_button.set_text("U");

    bold_button.set_unchecked_background_color(Color(238, 245, 251));
    italic_button.set_unchecked_background_color(Color(238, 245, 251));
    underline_button.set_unchecked_background_color(Color(238, 245, 251));

    bold_button.set_checked_background_color(Color(177, 213, 239));
    italic_button.set_checked_background_color(Color(177, 213, 239));
    underline_button.set_checked_background_color(Color(177, 213, 239));

    bold_button.get_style().foreground_color = Color(26, 68, 108);
    italic_button.get_style().foreground_color = Color(26, 68, 108);
    underline_button.get_style().foreground_color = Color(26, 68, 108);

    bold_button.get_style().border_color = Color(132, 157, 181);
    italic_button.get_style().border_color = Color(132, 157, 181);
    underline_button.get_style().border_color = Color(132, 157, 181);

    font_size_combo.add_item("8", 8);
    font_size_combo.add_item("10", 10);
    font_size_combo.add_item("12", 12);
    font_size_combo.add_item("14", 14);
    font_size_combo.add_item("16", 16);
    font_size_combo.add_item("18", 18);
    font_size_combo.add_item("20", 20);
    font_size_combo.add_item("24", 24);
    font_size_combo.set_selected_index(2);
    font_size_combo.set_drop_direction(ComboBox::drop_up);
    font_size_combo.set_selection_changed_handler(
        MessageToolbar::on_font_size_changed,
        this
    );

    list_button.set_text("List");
    list_button.set_enabled(true);
    list_button.set_unchecked_background_color(Color(238, 245, 251));
    list_button.set_checked_background_color(Color(177, 213, 239));
    list_button.get_style().foreground_color = Color(26, 68, 108);
    list_button.get_style().border_color = Color(132, 157, 181);

    emoji_button.set_text(":)");
    emoji_button.get_style().background_color = Color(238, 245, 251);
    emoji_button.get_style().foreground_color = Color(26, 68, 108);
    emoji_button.get_style().border_color = Color(132, 157, 181);

    attachment_status_label.set_text("");
    attachment_status_label.set_horizontal_alignment(Label::align_left);
    attachment_status_label.get_style().foreground_color = Color(73, 105, 133);

    emoji_panel.set_emoticon_selected_handler(
        MessageToolbar::on_emoticon_selected,
        this
    );

    list_panel.set_list_selected_handler(
        MessageToolbar::on_list_selected,
        this
    );

    add_child(&attach_button);
    add_child(&bold_button);
    add_child(&italic_button);
    add_child(&underline_button);
    add_child(&font_size_combo);
    add_child(&list_button);
    add_child(&emoji_button);
    add_child(&attachment_status_label);
    add_child(&list_panel);
    add_child(&emoji_panel);
}

MessageToolbar::~MessageToolbar() {
    detach_native_controls();
}

void MessageToolbar::set_file_dialog(FileDialog* new_file_dialog) {
    file_dialog = new_file_dialog;
}

void MessageToolbar::attach_native_controls(NativeControlHost* control_host) {
    if (native_control_host == control_host) {
        return;
    }

    detach_native_controls();
    native_control_host = control_host;

    if (native_control_host != 0) {
        native_control_host->attach_combo_box(&font_size_combo);
        native_control_host->sync_combo_box(&font_size_combo);
    }
}

void MessageToolbar::detach_native_controls() {
    if (native_control_host == 0) {
        return;
    }

    native_control_host->detach_combo_box(&font_size_combo);
    native_control_host = 0;
}

void MessageToolbar::set_insert_text_handler(
    InsertTextHandler new_handler,
    void* new_context
) {
    insert_text_handler = new_handler;
    insert_text_context = new_context;
}

void MessageToolbar::set_attachments_added_handler(
    AttachmentsAddedHandler new_handler,
    void* new_context
) {
    attachments_added_handler = new_handler;
    attachments_added_context = new_context;
}

void MessageToolbar::set_format_changed_handler(
    FormatChangedHandler new_handler,
    void* new_context
) {
    format_changed_handler = new_handler;
    format_changed_context = new_context;
}

void MessageToolbar::set_list_requested_handler(
    ListRequestedHandler new_handler,
    void* new_context
) {
    list_requested_handler = new_handler;
    list_requested_context = new_context;
}

void MessageToolbar::set_attachment_count(int attachment_count) {
    if (attachment_count <= 0) {
        attachment_status_label.set_text("");
        return;
    }

    char status_text[64];
    sprintf(status_text, "Files: %d", attachment_count);
    attachment_status_label.set_text(status_text);
}

void MessageToolbar::set_list_style(ListPanel::ListStyle new_list_style) {
    list_style = new_list_style;
    list_button.set_checked(list_style != ListPanel::list_clear);
}

ListPanel::ListStyle MessageToolbar::get_list_style() const {
    return list_style;
}

bool MessageToolbar::get_bold() const {
    return bold_button.get_is_checked();
}

bool MessageToolbar::get_italic() const {
    return italic_button.get_is_checked();
}

bool MessageToolbar::get_underline() const {
    return underline_button.get_is_checked();
}

int MessageToolbar::get_font_size() const {
    return font_size;
}

bool MessageToolbar::contains_popup_point(int x, int y) const {
    if (list_panel.get_is_open() && list_panel.contains_point(x, y)) {
        return true;
    }

    if (emoji_panel.get_is_open() && emoji_panel.contains_point(x, y)) {
        return true;
    }

    return font_size_combo.contains_open_popup_point(x, y);
}

void MessageToolbar::arrange(int x, int y, int width, int height) {
    const int padding = 4;
    const int gap = 4;
    const int attach_width = 30;
    const int toggle_width = 26;
    const int font_width = 58;
    const int list_width = 46;
    const int emoji_width = 38;
    const int list_popup_width = 128;
    const int list_popup_height = 82;
    const int emoji_popup_width = 188;
    const int emoji_popup_height = 58;

    set_bounds(x, y, width, height);

    int control_height = height - (padding * 2);
    if (control_height < 0) {
        control_height = 0;
    }

    int control_y = y + padding;
    int cursor_x = x + padding;

    attach_button.set_bounds(cursor_x, control_y, attach_width, control_height);
    cursor_x += attach_width + gap;

    bold_button.set_bounds(cursor_x, control_y, toggle_width, control_height);
    cursor_x += toggle_width + gap;

    italic_button.set_bounds(cursor_x, control_y, toggle_width, control_height);
    cursor_x += toggle_width + gap;

    underline_button.set_bounds(cursor_x, control_y, toggle_width, control_height);
    cursor_x += toggle_width + gap;

    font_size_combo.arrange(cursor_x, control_y, font_width, control_height);

    if (native_control_host != 0) {
        native_control_host->sync_combo_box(&font_size_combo);
    }

    cursor_x += font_width + gap;

    int list_x = cursor_x;
    list_button.set_bounds(list_x, control_y, list_width, control_height);
    cursor_x += list_width + gap;

    int emoji_x = cursor_x;
    emoji_button.set_bounds(emoji_x, control_y, emoji_width, control_height);
    cursor_x += emoji_width + gap;

    int remaining_width = x + width - padding - cursor_x;
    if (remaining_width < 0) {
        remaining_width = 0;
    }

    attachment_status_label.set_bounds(
        cursor_x,
        control_y,
        remaining_width,
        control_height
    );

    int list_popup_x = list_x;
    int maximum_list_x = x + width - padding - list_popup_width;
    if (list_popup_x > maximum_list_x) {
        list_popup_x = maximum_list_x;
    }
    if (list_popup_x < x + padding) {
        list_popup_x = x + padding;
    }

    list_panel.arrange(
        list_popup_x,
        y + height + 2,
        list_popup_width,
        list_popup_height
    );

    int popup_x = emoji_x + emoji_width - emoji_popup_width;
    int minimum_popup_x = x + padding;

    if (popup_x < minimum_popup_x) {
        popup_x = minimum_popup_x;
    }

    emoji_panel.arrange(
        popup_x,
        y + height + 2,
        emoji_popup_width,
        emoji_popup_height
    );
}

bool MessageToolbar::handle_event(const UIEvent& event) {
    if (!get_is_visible()) {
        return false;
    }

    if (
        event.type == UIEvent::event_character &&
        event.control_down &&
        !event.alt_down
    ) {
        if (event.character_code == control_character_b) {
            toggle_bold();
            return true;
        }

        if (event.character_code == control_character_i) {
            toggle_italic();
            return true;
        }

        if (event.character_code == control_character_u) {
            toggle_underline();
            return true;
        }
    }

    if (event.type == UIEvent::event_mouse_down) {
        if (
            list_panel.get_is_open() &&
            !list_panel.contains_point(event.x, event.y) &&
            !list_button.contains_point(event.x, event.y)
        ) {
            list_panel.set_open(false);
        }

        if (
            emoji_panel.get_is_open() &&
            !emoji_panel.contains_point(event.x, event.y) &&
            !emoji_button.contains_point(event.x, event.y)
        ) {
            emoji_panel.set_open(false);
        }

        if (
            !font_size_combo.get_native_peer_active() &&
            font_size_combo.contains_point(event.x, event.y)
        ) {
            list_panel.set_open(false);
            emoji_panel.set_open(false);
        }
    }

    bool attach_was_pressed = attach_button.get_is_pressed();
    bool list_was_pressed = list_button.get_is_pressed();
    bool emoji_was_pressed = emoji_button.get_is_pressed();
    bool bold_was_checked = bold_button.get_is_checked();
    bool italic_was_checked = italic_button.get_is_checked();
    bool underline_was_checked = underline_button.get_is_checked();

    bool was_handled = Panel::handle_event(event);

    // The List control is a state indicator, not a popup-state toggle. Restore
    // its visual checked state after ToggleButton processes pointer input.
    list_button.set_checked(list_style != ListPanel::list_clear);

    if (event.type != UIEvent::event_mouse_up) {
        return was_handled;
    }

    if (
        bold_was_checked != bold_button.get_is_checked() ||
        italic_was_checked != italic_button.get_is_checked() ||
        underline_was_checked != underline_button.get_is_checked()
    ) {
        notify_format_changed();
        return true;
    }

    if (
        attach_was_pressed &&
        attach_button.contains_point(event.x, event.y)
    ) {
        font_size_combo.set_open(false);
        list_panel.set_open(false);
        emoji_panel.set_open(false);
        open_attachment_dialog();
        return true;
    }

    if (
        list_was_pressed &&
        list_button.contains_point(event.x, event.y)
    ) {
        font_size_combo.set_open(false);
        emoji_panel.set_open(false);
        list_panel.set_open(!list_panel.get_is_open());
        list_button.set_checked(list_style != ListPanel::list_clear);
        return true;
    }

    if (
        emoji_was_pressed &&
        emoji_button.contains_point(event.x, event.y)
    ) {
        font_size_combo.set_open(false);
        list_panel.set_open(false);
        emoji_panel.set_open(!emoji_panel.get_is_open());
        return true;
    }

    return was_handled;
}

void MessageToolbar::on_font_size_changed(
    ComboBox* combo_box,
    int selected_value,
    const char* selected_text,
    void* context
) {
    (void)combo_box;
    (void)selected_text;

    MessageToolbar* toolbar = (MessageToolbar*)context;
    if (toolbar != 0) {
        toolbar->font_size = selected_value;
        toolbar->list_panel.set_open(false);
        toolbar->emoji_panel.set_open(false);
        toolbar->notify_format_changed();
    }
}

void MessageToolbar::on_emoticon_selected(
    EmojiPanel* panel,
    const char* alias,
    void* context
) {
    (void)panel;

    MessageToolbar* toolbar = (MessageToolbar*)context;
    if (
        toolbar != 0 &&
        toolbar->insert_text_handler != 0 &&
        alias != 0
    ) {
        toolbar->insert_text_handler(
            toolbar,
            alias,
            toolbar->insert_text_context
        );
    }
}

void MessageToolbar::on_list_selected(
    ListPanel* panel,
    ListPanel::ListStyle style,
    void* context
) {
    (void)panel;

    MessageToolbar* toolbar = (MessageToolbar*)context;
    if (
        toolbar != 0 &&
        toolbar->list_requested_handler != 0
    ) {
        toolbar->list_requested_handler(
            toolbar,
            style,
            toolbar->list_requested_context
        );
    }
}

void MessageToolbar::toggle_bold() {
    bold_button.set_checked(!bold_button.get_is_checked());
    notify_format_changed();
}

void MessageToolbar::toggle_italic() {
    italic_button.set_checked(!italic_button.get_is_checked());
    notify_format_changed();
}

void MessageToolbar::toggle_underline() {
    underline_button.set_checked(!underline_button.get_is_checked());
    notify_format_changed();
}

void MessageToolbar::notify_format_changed() {
    if (format_changed_handler == 0) {
        return;
    }

    format_changed_handler(
        this,
        get_bold(),
        get_italic(),
        get_underline(),
        get_font_size(),
        format_changed_context
    );
}

bool MessageToolbar::open_attachment_dialog() {
    if (file_dialog == 0) {
        return false;
    }

    std::vector<std::string> selected_paths;
    bool did_select = file_dialog->open_files(selected_paths);

    if (
        did_select &&
        !selected_paths.empty() &&
        attachments_added_handler != 0
    ) {
        attachments_added_handler(
            this,
            selected_paths,
            attachments_added_context
        );
    }

    return did_select;
}
