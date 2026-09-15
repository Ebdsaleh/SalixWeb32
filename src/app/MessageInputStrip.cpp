// =================================================================================
// Filename:    app/MessageInputStrip.cpp
// Author:      Ebdsaleh
// Description: Implements the composite message-entry strip used by the shell.
// =================================================================================

#include "MessageInputStrip.h"
#include "framework/UIEvent.h"

MessageInputStrip::MessageInputStrip()
    : submit_on_enter(true),
      submit_handler(0),
      submit_context(0) {

    get_style().background_color = Color(229, 240, 249);
    get_style().border_color = Color(147, 181, 211);
    get_style().border_width = 1;

    message_input.set_max_length(180);
    message_input.get_style().background_color = Color(255, 255, 255);
    message_input.get_style().border_color = Color(132, 157, 181);

    send_button.set_text("Send");
    send_button.get_style().background_color = Color(214, 232, 247);
    send_button.get_style().foreground_color = Color(26, 68, 108);
    send_button.get_style().border_color = Color(112, 153, 191);
    send_button.set_click_handler(
        MessageInputStrip::on_button_clicked,
        this
    );

    add_child(&message_input);
    add_child(&send_button);
}

void MessageInputStrip::set_text(const char* new_text) {
    message_input.set_text(new_text);
}

const char* MessageInputStrip::get_text() const {
    return message_input.get_text();
}

void MessageInputStrip::clear() {
    message_input.set_text("");
}

void MessageInputStrip::set_button_text(const char* new_text) {
    send_button.set_text(new_text);
}

void MessageInputStrip::set_max_length(int new_max_length) {
    message_input.set_max_length(new_max_length);
}

void MessageInputStrip::set_submit_on_enter(bool new_submit_on_enter) {
    submit_on_enter = new_submit_on_enter;
}

bool MessageInputStrip::get_submit_on_enter() const {
    return submit_on_enter;
}

void MessageInputStrip::set_submit_handler(
    SubmitHandler new_submit_handler,
    void* new_context
) {
    submit_handler = new_submit_handler;
    submit_context = new_context;
}

void MessageInputStrip::arrange(int x, int y, int width, int height) {
    const int padding = 6;
    const int gap = 6;
    int button_width = 82;

    set_bounds(x, y, width, height);

    if (button_width > width / 3) {
        button_width = width / 3;
    }

    if (button_width < 52) {
        button_width = 52;
    }

    int inner_height = height - (padding * 2);
    if (inner_height < 0) {
        inner_height = 0;
    }

    int button_x = x + width - padding - button_width;
    int input_x = x + padding;
    int input_width = button_x - gap - input_x;

    if (input_width < 0) {
        input_width = 0;
    }

    message_input.set_bounds(
        input_x,
        y + padding,
        input_width,
        inner_height
    );

    send_button.set_bounds(
        button_x,
        y + padding,
        button_width,
        inner_height
    );
}

bool MessageInputStrip::handle_event(const UIEvent& event) {
    if (!get_is_visible()) {
        return false;
    }

    if (
        submit_on_enter &&
        event.type == UIEvent::event_character &&
        event.character_code == 13 &&
        message_input.get_is_focused()
    ) {
        submit();
        return true;
    }

    return Panel::handle_event(event);
}

void MessageInputStrip::on_button_clicked(Button* button, void* context) {
    (void)button;

    MessageInputStrip* input_strip = (MessageInputStrip*)context;
    if (input_strip != 0) {
        input_strip->submit();
    }
}

void MessageInputStrip::submit() {
    const char* current_text = message_input.get_text();

    if (current_text == 0 || current_text[0] == '\0') {
        return;
    }

    if (submit_handler != 0) {
        submit_handler(this, current_text, submit_context);
    }

    clear();
}
