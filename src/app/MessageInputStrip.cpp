// =================================================================================
// Filename:    app/MessageInputStrip.cpp
// Author:      Ebdsaleh
// Description: Implements the composite message-entry strip used by the shell.
// =================================================================================

#include <stdio.h>
#include <string.h>

#include "MessageInputStrip.h"
#include "framework/FormattedText.h"
#include "framework/UIEvent.h"
#include "framework/MimeData.h"

MessageInputStrip::MessageInputStrip()
    : submit_on_enter(true),
      allow_empty_submit(false),
      submit_handler(0),
      submit_context(0) {

    get_style().background_color = Color(229, 240, 249);
    get_style().border_color = Color(147, 181, 211);
    get_style().border_width = 1;

    message_input.set_multiline(true);
    message_input.set_line_spacing(2);
    message_input.set_max_length(1200);
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

void MessageInputStrip::get_formatted_text(
    FormattedText& formatted_text
) const {
    formatted_text.set_formatted_text(
        message_input.get_text(),
        message_input.get_format_data(),
        message_input.get_format_count(),
        message_input.get_typing_format()
    );
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

void MessageInputStrip::set_allow_empty_submit(bool new_allow_empty_submit) {
    allow_empty_submit = new_allow_empty_submit;
}

bool MessageInputStrip::get_allow_empty_submit() const {
    return allow_empty_submit;
}

void MessageInputStrip::set_text_format(
    bool bold,
    bool italic,
    bool underline,
    int font_size
) {
    message_input.set_text_format(
        bold,
        italic,
        underline,
        font_size
    );
}

bool MessageInputStrip::apply_list_style(TextInput::ListStyle style) {
    return message_input.apply_list_style(style);
}

TextInput::ListStyle MessageInputStrip::get_current_list_style() const {
    const char* current_text = message_input.get_text();
    if (current_text == 0) {
        return TextInput::list_clear;
    }

    int text_length = (int)strlen(current_text);
    int caret = message_input.get_cursor_position();

    if (caret < 0) {
        caret = 0;
    }
    if (caret > text_length) {
        caret = text_length;
    }

    int line_start = caret;
    while (line_start > 0 && current_text[line_start - 1] != '\n') {
        --line_start;
    }

    if (
        line_start + 1 < text_length &&
        (current_text[line_start] == '*' || current_text[line_start] == '-') &&
        current_text[line_start + 1] == ' '
    ) {
        return TextInput::list_bulleted;
    }

    int position = line_start;
    bool found_digit = false;

    while (
        position < text_length &&
        current_text[position] >= '0' &&
        current_text[position] <= '9'
    ) {
        found_digit = true;
        ++position;
    }

    if (
        found_digit &&
        position + 1 < text_length &&
        current_text[position] == '.' &&
        current_text[position + 1] == ' '
    ) {
        return TextInput::list_numbered;
    }

    return TextInput::list_clear;
}

bool MessageInputStrip::accepts_mime_type(const char* mime_type) const {
    return message_input.accepts_mime_type(mime_type);
}

bool MessageInputStrip::insert_mime_data(const MimeData& data) {
    return message_input.insert_mime_data(data);
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

    int send_height = inner_height;
    if (send_height > 38) {
        send_height = 38;
    }

    int send_y = y + padding + inner_height - send_height;

    send_button.set_bounds(
        button_x,
        send_y,
        button_width,
        send_height
    );
}

bool MessageInputStrip::handle_event(const UIEvent& event) {
    if (!get_is_visible()) {
        return false;
    }

    bool is_enter_character =
        event.character_code == 13 ||
        (event.control_down && event.character_code == 10);

    if (
        event.type == UIEvent::event_character &&
        is_enter_character &&
        message_input.get_is_focused()
    ) {
        if (event.control_down) {
            submit();
            return true;
        }

        if (
            !event.shift_down &&
            get_current_list_style() != TextInput::list_clear
        ) {
            continue_current_list();
            return true;
        }

        if (submit_on_enter && !event.shift_down) {
            submit();
            return true;
        }
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

bool MessageInputStrip::continue_current_list() {
    TextInput::ListStyle style = get_current_list_style();
    if (style == TextInput::list_clear) {
        return false;
    }

    char continuation[64];
    continuation[0] = '\0';

    if (style == TextInput::list_bulleted) {
        strcpy(continuation, "\n* ");
    } else if (style == TextInput::list_numbered) {
        const char* current_text = message_input.get_text();
        int text_length = current_text == 0 ? 0 : (int)strlen(current_text);
        int caret = message_input.get_cursor_position();

        if (caret < 0) {
            caret = 0;
        }
        if (caret > text_length) {
            caret = text_length;
        }

        int line_start = caret;
        while (line_start > 0 && current_text[line_start - 1] != '\n') {
            --line_start;
        }

        int current_number = 0;
        int position = line_start;

        while (
            position < text_length &&
            current_text[position] >= '0' &&
            current_text[position] <= '9'
        ) {
            current_number = (current_number * 10) +
                (current_text[position] - '0');
            ++position;
        }

        if (current_number < 1) {
            current_number = 1;
        }

        sprintf(continuation, "\n%d. ", current_number + 1);
    }

    MimeData data;
    data.set_text(continuation);
    return message_input.insert_mime_data(data);
}

void MessageInputStrip::submit() {
    const char* current_text = message_input.get_text();
    bool is_empty = current_text == 0 || current_text[0] == '\0';

    if (is_empty && !allow_empty_submit) {
        return;
    }

    if (submit_handler != 0) {
        submit_handler(this, current_text == 0 ? "" : current_text, submit_context);
    }

    clear();
}
