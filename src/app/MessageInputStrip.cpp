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
#include "framework/NativeControlHost.h"
#include "framework/TextMetrics.h"
#include "framework/TextViewportState.h"
#include "framework/EmoticonRegistry.h"

namespace {
    int maximum_int(int first, int second) {
        return first > second ? first : second;
    }
}

MessageInputStrip::MessageInputStrip()
    : horizontal_scroll_bar(ScrollBar::horizontal),
      vertical_scroll_bar(ScrollBar::vertical),
      native_control_host(0),
      submit_on_enter(true),
      allow_empty_submit(false),
      code_mode(false),
      tab_size(4),
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

    horizontal_scroll_bar.set_line_step(24);
    vertical_scroll_bar.set_line_step(20);
    horizontal_scroll_bar.set_value_changed_handler(
        MessageInputStrip::on_scroll_changed,
        this
    );
    vertical_scroll_bar.set_value_changed_handler(
        MessageInputStrip::on_scroll_changed,
        this
    );

    scroll_corner.get_style().background_color = Color(224, 232, 239);
    scroll_corner.get_style().border_color = Color(150, 168, 184);
    scroll_corner.get_style().border_width = 1;

    add_child(&message_input);
    add_child(&horizontal_scroll_bar);
    add_child(&vertical_scroll_bar);
    add_child(&scroll_corner);
    add_child(&send_button);

    apply_viewport_state();
}

MessageInputStrip::~MessageInputStrip() {
    detach_native_controls();
    TextViewportState::clear(&message_input);
}

void MessageInputStrip::set_text(const char* new_text) {
    message_input.set_text(new_text);
    update_scrollbars(0);
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
    horizontal_scroll_bar.set_value(0);
    vertical_scroll_bar.set_value(0);
    update_scrollbars(0);
}

void MessageInputStrip::set_button_text(const char* new_text) {
    send_button.set_text(new_text);
}

void MessageInputStrip::set_max_length(int new_max_length) {
    message_input.set_max_length(new_max_length);
    update_scrollbars(0);
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

void MessageInputStrip::set_code_mode(bool new_code_mode) {
    code_mode = new_code_mode;
}

bool MessageInputStrip::get_code_mode() const {
    return code_mode;
}

void MessageInputStrip::set_tab_size(int new_tab_size) {
    if (
        new_tab_size != 2 &&
        new_tab_size != 4 &&
        new_tab_size != 6 &&
        new_tab_size != 8
    ) {
        new_tab_size = 4;
    }

    tab_size = new_tab_size;
}

int MessageInputStrip::get_tab_size() const {
    return tab_size;
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
    update_scrollbars(0);
}

bool MessageInputStrip::apply_list_style(TextInput::ListStyle style) {
    bool applied = message_input.apply_list_style(style);
    if (applied) {
        update_scrollbars(0);
    }
    return applied;
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
    bool inserted = message_input.insert_mime_data(data);
    if (inserted) {
        update_scrollbars(0);
    }
    return inserted;
}

void MessageInputStrip::set_submit_handler(
    SubmitHandler new_submit_handler,
    void* new_context
) {
    submit_handler = new_submit_handler;
    submit_context = new_context;
}

void MessageInputStrip::attach_native_controls(
    NativeControlHost* control_host
) {
    if (native_control_host == control_host) {
        return;
    }

    detach_native_controls();
    native_control_host = control_host;

    if (native_control_host != 0) {
        native_control_host->attach_scroll_bar(&horizontal_scroll_bar);
        native_control_host->attach_scroll_bar(&vertical_scroll_bar);
        sync_native_scrollbars();
    }
}

void MessageInputStrip::detach_native_controls() {
    if (native_control_host == 0) {
        return;
    }

    native_control_host->detach_scroll_bar(&horizontal_scroll_bar);
    native_control_host->detach_scroll_bar(&vertical_scroll_bar);
    native_control_host = 0;
}

void MessageInputStrip::arrange(int x, int y, int width, int height) {
    const int padding = 6;
    const int gap = 6;
    const int scroll_bar_size = 16;
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

    int viewport_width = input_width - scroll_bar_size;
    int viewport_height = inner_height - scroll_bar_size;

    if (viewport_width < 0) {
        viewport_width = 0;
    }
    if (viewport_height < 0) {
        viewport_height = 0;
    }

    int input_y = y + padding;

    message_input.set_bounds(
        input_x,
        input_y,
        viewport_width,
        viewport_height
    );

    vertical_scroll_bar.arrange(
        input_x + viewport_width,
        input_y,
        scroll_bar_size,
        viewport_height
    );

    horizontal_scroll_bar.arrange(
        input_x,
        input_y + viewport_height,
        viewport_width,
        scroll_bar_size
    );

    scroll_corner.set_bounds(
        input_x + viewport_width,
        input_y + viewport_height,
        scroll_bar_size,
        scroll_bar_size
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

    update_scrollbars(0);
}

bool MessageInputStrip::handle_event(const UIEvent& event) {
    if (!get_is_visible()) {
        return false;
    }

    if (
        horizontal_scroll_bar.handle_event(event) ||
        vertical_scroll_bar.handle_event(event)
    ) {
        apply_viewport_state();
        return true;
    }

    if (send_button.handle_event(event)) {
        return true;
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

        if (code_mode) {
            bool handled = handle_text_input_event(event);
            update_scrollbars(event.text_metrics);
            ensure_caret_visible(event.text_metrics);
            return handled;
        }

        if (
            !event.shift_down &&
            get_current_list_style() != TextInput::list_clear
        ) {
            continue_current_list();
            ensure_caret_visible(event.text_metrics);
            return true;
        }

        if (submit_on_enter && !event.shift_down) {
            submit();
            return true;
        }
    }

    if (
        event.type == UIEvent::event_key_down &&
        event.key_code == UIEvent::key_tab &&
        code_mode &&
        message_input.get_is_focused()
    ) {
        insert_indentation();
        update_scrollbars(event.text_metrics);
        ensure_caret_visible(event.text_metrics);
        return true;
    }

    bool handled = handle_text_input_event(event);
    if (!handled) {
        return false;
    }

    update_scrollbars(event.text_metrics);

    if (message_input.get_is_focused()) {
        ensure_caret_visible(event.text_metrics);
    }

    return true;
}

void MessageInputStrip::on_button_clicked(Button* button, void* context) {
    (void)button;

    MessageInputStrip* input_strip = (MessageInputStrip*)context;
    if (input_strip != 0) {
        input_strip->submit();
    }
}

void MessageInputStrip::on_scroll_changed(
    ScrollBar* scroll_bar,
    int value,
    void* context
) {
    (void)scroll_bar;
    (void)value;

    MessageInputStrip* input_strip = (MessageInputStrip*)context;
    if (input_strip != 0) {
        input_strip->apply_viewport_state();
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
    bool inserted = message_input.insert_mime_data(data);
    update_scrollbars(0);
    return inserted;
}

bool MessageInputStrip::insert_indentation() {
    char indentation[9];
    int count = tab_size;

    if (count < 1) {
        count = 4;
    }
    if (count > 8) {
        count = 8;
    }

    for (int index = 0; index < count; ++index) {
        indentation[index] = ' ';
    }
    indentation[count] = '\0';

    MimeData data;
    data.set_text(indentation);
    return message_input.insert_mime_data(data);
}

bool MessageInputStrip::handle_text_input_event(const UIEvent& event) {
    bool is_mouse_event =
        event.type == UIEvent::event_mouse_down ||
        event.type == UIEvent::event_mouse_move ||
        event.type == UIEvent::event_mouse_up;

    if (
        is_mouse_event &&
        event.type == UIEvent::event_mouse_down &&
        !message_input.contains_point(event.x, event.y)
    ) {
        return false;
    }

    int scroll_x = horizontal_scroll_bar.get_value();
    int scroll_y = vertical_scroll_bar.get_value();

    if (!is_mouse_event || (scroll_x == 0 && scroll_y == 0)) {
        return message_input.handle_event(event);
    }

    int old_x = message_input.get_x();
    int old_y = message_input.get_y();
    int old_width = message_input.get_width();
    int old_height = message_input.get_height();

    message_input.set_bounds(
        old_x - scroll_x,
        old_y - scroll_y,
        old_width + scroll_x,
        old_height + scroll_y
    );

    bool handled = message_input.handle_event(event);

    message_input.set_bounds(
        old_x,
        old_y,
        old_width,
        old_height
    );

    return handled;
}

void MessageInputStrip::update_scrollbars(TextMetrics* text_metrics) {
    const char* text = message_input.get_text();
    int text_length = text == 0 ? 0 : (int)strlen(text);

    int content_width = 0;
    int content_height = 0;

    if (text_metrics != 0 && text_length > 0) {
        content_width = text_metrics->measure_formatted_text_width(
            text,
            text_length,
            message_input.get_format_data(),
            message_input.get_format_count()
        );
        content_height = text_metrics->measure_formatted_text_height(
            text,
            text_length,
            message_input.get_format_data(),
            message_input.get_format_count(),
            message_input.get_line_spacing()
        );
    } else {
        estimate_content_extent(content_width, content_height);
    }

    int viewport_width = message_input.get_width() -
        (message_input.get_text_padding() * 2);
    int viewport_height = message_input.get_height() -
        (message_input.get_text_padding() * 2);

    if (viewport_width < 1) {
        viewport_width = 1;
    }
    if (viewport_height < 1) {
        viewport_height = 1;
    }

    int maximum_x = content_width + 4 - viewport_width;
    int maximum_y = content_height + 4 - viewport_height;

    if (maximum_x < 0) {
        maximum_x = 0;
    }
    if (maximum_y < 0) {
        maximum_y = 0;
    }

    horizontal_scroll_bar.set_range(0, maximum_x, viewport_width);
    vertical_scroll_bar.set_range(0, maximum_y, viewport_height);
    apply_viewport_state();
}

void MessageInputStrip::ensure_caret_visible(TextMetrics* text_metrics) {
    const char* text = message_input.get_text();
    if (text == 0) {
        return;
    }

    int text_length = (int)strlen(text);
    int caret = message_input.get_cursor_position();
    if (caret < 0) {
        caret = 0;
    }
    if (caret > text_length) {
        caret = text_length;
    }

    int line_start = caret;
    while (line_start > 0 && text[line_start - 1] != '\n') {
        --line_start;
    }

    int caret_x = 0;
    int caret_y = 0;
    int caret_line_height = 18;
    const TextFormat* formats = message_input.get_format_data();

    if (text_metrics != 0) {
        int prefix_length = caret - line_start;
        caret_x = text_metrics->measure_formatted_text_width(
            text + line_start,
            prefix_length,
            formats == 0 ? 0 : formats + line_start,
            prefix_length
        );

        int current_start = 0;
        while (current_start < line_start) {
            int current_end = current_start;
            while (
                current_end < text_length &&
                text[current_end] != '\n'
            ) {
                ++current_end;
            }

            int line_length = current_end - current_start;
            int line_height = text_metrics->measure_formatted_text_height(
                text + current_start,
                line_length,
                formats == 0 ? 0 : formats + current_start,
                line_length,
                0
            );
            if (line_height < 16) {
                line_height = 16;
            }
            caret_y += line_height + message_input.get_line_spacing();

            if (current_end >= text_length) {
                break;
            }
            current_start = current_end + 1;
        }

        int line_end = line_start;
        while (line_end < text_length && text[line_end] != '\n') {
            ++line_end;
        }
        int line_length = line_end - line_start;
        caret_line_height = text_metrics->measure_formatted_text_height(
            text + line_start,
            line_length,
            formats == 0 ? 0 : formats + line_start,
            line_length,
            0
        );
        if (caret_line_height < 16) {
            caret_line_height = 16;
        }
    } else {
        int current_x = 0;
        int current_y = 0;
        int line_height = 18;

        for (int index = 0; index < caret; ++index) {
            if (text[index] == '\n') {
                current_y += line_height + message_input.get_line_spacing();
                current_x = 0;
                line_height = 18;
                continue;
            }

            TextFormat format = message_input.get_character_format(index);
            int character_width = maximum_int(4, (format.font_size * 3) / 5);
            current_x += character_width;
            line_height = maximum_int(line_height, format.font_size + 8);
        }

        caret_x = current_x;
        caret_y = current_y;
        caret_line_height = line_height;
    }

    int viewport_width = message_input.get_width() -
        (message_input.get_text_padding() * 2);
    int viewport_height = message_input.get_height() -
        (message_input.get_text_padding() * 2);

    if (viewport_width < 1) {
        viewport_width = 1;
    }
    if (viewport_height < 1) {
        viewport_height = 1;
    }

    int scroll_x = horizontal_scroll_bar.get_value();
    int scroll_y = vertical_scroll_bar.get_value();

    if (caret_x < scroll_x) {
        scroll_x = caret_x;
    } else if (caret_x > scroll_x + viewport_width - 4) {
        scroll_x = caret_x - viewport_width + 4;
    }

    if (caret_y < scroll_y) {
        scroll_y = caret_y;
    } else if (
        caret_y + caret_line_height >
        scroll_y + viewport_height
    ) {
        scroll_y = caret_y + caret_line_height - viewport_height;
    }

    horizontal_scroll_bar.set_value(scroll_x);
    vertical_scroll_bar.set_value(scroll_y);
    apply_viewport_state();
}

void MessageInputStrip::estimate_content_extent(
    int& width,
    int& height
) const {
    width = 0;
    height = 18;

    const char* text = message_input.get_text();
    if (text == 0 || text[0] == '\0') {
        return;
    }

    int text_length = (int)strlen(text);
    int current_width = 0;
    int current_line_height = 18;
    int total_height = 0;
    int position = 0;

    while (position < text_length) {
        if (text[position] == '\n') {
            width = maximum_int(width, current_width);
            total_height += current_line_height +
                message_input.get_line_spacing();
            current_width = 0;
            current_line_height = 18;
            ++position;
            continue;
        }

        TextFormat format = message_input.get_character_format(position);
        EmoticonRegistry::EmoticonId emoticon_id;
        int alias_length = 0;

        if (
            format.code_style == TextFormat::code_none &&
            EmoticonRegistry::match_at(
                text,
                text_length,
                position,
                emoticon_id,
                alias_length
            )
        ) {
            int visual_size = EmoticonRegistry::get_visual_size(
                format.font_size
            );
            current_width += visual_size;
            current_line_height = maximum_int(
                current_line_height,
                visual_size
            );
            position += alias_length;
            continue;
        }

        current_width += maximum_int(4, (format.font_size * 3) / 5);
        current_line_height = maximum_int(
            current_line_height,
            format.font_size + 8
        );
        ++position;
    }

    width = maximum_int(width, current_width);
    total_height += current_line_height;
    height = maximum_int(18, total_height);
}

void MessageInputStrip::apply_viewport_state() {
    TextViewportState::set_scroll(
        &message_input,
        horizontal_scroll_bar.get_value(),
        vertical_scroll_bar.get_value()
    );

    sync_native_scrollbars();
}

void MessageInputStrip::sync_native_scrollbars() {
    if (native_control_host == 0) {
        return;
    }

    native_control_host->sync_scroll_bar(&horizontal_scroll_bar);
    native_control_host->sync_scroll_bar(&vertical_scroll_bar);
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