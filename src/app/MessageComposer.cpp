// =================================================================================
// Filename:    app/MessageComposer.cpp
// Author:      Ebdsaleh
// Description: Implements the complete messenger composer.
// =================================================================================

#include <string.h>

#include "MessageComposer.h"
#include "framework/FormattedText.h"
#include "framework/TextFormat.h"
#include "framework/MimeData.h"
#include "framework/UIEvent.h"

MessageComposer::MessageComposer(FileDialog* file_dialog)
    : message_toolbar(file_dialog),
      submit_handler(0),
      submit_context(0) {

    get_style().background_color = Color(229, 240, 249);
    get_style().border_width = 0;

    message_input_strip.set_button_text("Send");
    message_input_strip.set_submit_on_enter(true);
    message_input_strip.set_submit_handler(
        MessageComposer::on_input_submitted,
        this
    );

    message_toolbar.set_insert_text_handler(
        MessageComposer::on_toolbar_insert_text,
        this
    );
    message_toolbar.set_attachments_added_handler(
        MessageComposer::on_toolbar_attachments_added,
        this
    );
    message_toolbar.set_format_changed_handler(
        MessageComposer::on_toolbar_format_changed,
        this
    );
    message_toolbar.set_list_requested_handler(
        MessageComposer::on_toolbar_list_requested,
        this
    );
    message_toolbar.set_code_mode_changed_handler(
        MessageComposer::on_toolbar_code_mode_changed,
        this
    );

    message_input_strip.set_code_mode(message_toolbar.get_code_mode());
    message_input_strip.set_tab_size(message_toolbar.get_tab_size());

    add_child(&message_input_strip);
    add_child(&message_toolbar);
    sync_list_state();
}

void MessageComposer::set_text(const char* new_text) {
    message_input_strip.set_text(new_text);
    sync_list_state();
}

const char* MessageComposer::get_text() const {
    return message_input_strip.get_text();
}

void MessageComposer::clear() {
    message_input_strip.clear();
    clear_attachments();
    sync_list_state();
}

void MessageComposer::set_button_text(const char* new_text) {
    message_input_strip.set_button_text(new_text);
}

void MessageComposer::set_max_length(int new_max_length) {
    message_input_strip.set_max_length(new_max_length);
}

void MessageComposer::set_submit_on_enter(bool new_submit_on_enter) {
    message_input_strip.set_submit_on_enter(new_submit_on_enter);
}

bool MessageComposer::get_submit_on_enter() const {
    return message_input_strip.get_submit_on_enter();
}

void MessageComposer::set_submit_handler(
    SubmitHandler new_submit_handler,
    void* new_context
) {
    submit_handler = new_submit_handler;
    submit_context = new_context;
}

int MessageComposer::get_attachment_count() const {
    return (int)attachment_paths.size();
}

const char* MessageComposer::get_attachment_path(int index) const {
    if (index < 0 || index >= (int)attachment_paths.size()) {
        return "";
    }

    return attachment_paths[index].c_str();
}

bool MessageComposer::get_bold() const {
    return message_toolbar.get_bold();
}

bool MessageComposer::get_italic() const {
    return message_toolbar.get_italic();
}

bool MessageComposer::get_underline() const {
    return message_toolbar.get_underline();
}

int MessageComposer::get_font_size() const {
    return message_toolbar.get_font_size();
}

bool MessageComposer::get_code_mode() const {
    return message_toolbar.get_code_mode();
}

int MessageComposer::get_tab_size() const {
    return message_toolbar.get_tab_size();
}

void MessageComposer::attach_native_controls(
    NativeControlHost* control_host
) {
    message_input_strip.attach_native_controls(control_host);
    message_toolbar.attach_native_controls(control_host);
}

void MessageComposer::detach_native_controls() {
    message_input_strip.detach_native_controls();
    message_toolbar.detach_native_controls();
}

bool MessageComposer::contains_popup_point(int x, int y) const {
    return message_toolbar.contains_popup_point(x, y);
}

void MessageComposer::arrange(int x, int y, int width, int height) {
    const int toolbar_height = 34;
    const int gap = 2;

    set_bounds(x, y, width, height);

    int input_height = height - toolbar_height - gap;
    if (input_height < 0) {
        input_height = 0;
    }

    message_toolbar.arrange(
        x,
        y,
        width,
        toolbar_height
    );

    message_input_strip.arrange(
        x,
        y + toolbar_height + gap,
        width,
        input_height
    );
}

bool MessageComposer::handle_event(const UIEvent& event) {
    if (!get_is_visible()) {
        return false;
    }

    if (
        event.type == UIEvent::event_key_down &&
        event.control_down &&
        !event.alt_down &&
        event.key_code == UIEvent::key_semicolon
    ) {
        message_toolbar.toggle_code_mode_from_shortcut();
        sync_list_state();
        return true;
    }

    // TranslateMessage can emit a WM_CHAR for Ctrl+; after the key-down event.
    // Consume that character so the shortcut does not toggle twice or insert ';'.
    if (
        event.type == UIEvent::event_character &&
        event.control_down &&
        !event.alt_down &&
        event.character_code == 59
    ) {
        return true;
    }

    if (message_toolbar.handle_event(event)) {
        sync_list_state();
        return true;
    }

    bool was_handled = message_input_strip.handle_event(event);
    sync_list_state();
    return was_handled;
}

void MessageComposer::on_input_submitted(
    MessageInputStrip* input_strip,
    const char* text,
    void* context
) {
    (void)input_strip;
    (void)text;

    MessageComposer* composer = (MessageComposer*)context;
    if (composer == 0) {
        return;
    }

    MessageDraft draft;
    composer->build_draft(draft);

    if (composer->submit_handler != 0 && !draft.empty()) {
        composer->submit_handler(
            composer,
            draft,
            composer->submit_context
        );
    }

    composer->clear_attachments();
}

void MessageComposer::on_toolbar_insert_text(
    MessageToolbar* toolbar,
    const char* text,
    void* context
) {
    (void)toolbar;

    MessageComposer* composer = (MessageComposer*)context;
    if (composer == 0 || text == 0) {
        return;
    }

    MimeData data;
    data.set_text(text);
    composer->message_input_strip.insert_mime_data(data);
    composer->sync_list_state();
}

void MessageComposer::on_toolbar_attachments_added(
    MessageToolbar* toolbar,
    const std::vector<std::string>& paths,
    void* context
) {
    (void)toolbar;

    MessageComposer* composer = (MessageComposer*)context;
    if (composer != 0) {
        composer->add_attachments(paths);
    }
}

void MessageComposer::on_toolbar_format_changed(
    MessageToolbar* toolbar,
    bool bold,
    bool italic,
    bool underline,
    int font_size,
    void* context
) {
    (void)toolbar;

    MessageComposer* composer = (MessageComposer*)context;
    if (composer == 0) {
        return;
    }

    composer->message_input_strip.set_text_format(
        bold,
        italic,
        underline,
        font_size
    );
}

void MessageComposer::on_toolbar_list_requested(
    MessageToolbar* toolbar,
    ListPanel::ListStyle style,
    void* context
) {
    (void)toolbar;

    MessageComposer* composer = (MessageComposer*)context;
    if (composer == 0) {
        return;
    }

    TextInput::ListStyle input_style = TextInput::list_clear;

    if (style == ListPanel::list_bulleted) {
        input_style = TextInput::list_bulleted;
    } else if (style == ListPanel::list_numbered) {
        input_style = TextInput::list_numbered;
    }

    composer->message_input_strip.apply_list_style(input_style);
    composer->sync_list_state();
}

void MessageComposer::on_toolbar_code_mode_changed(
    MessageToolbar* toolbar,
    bool code_mode,
    int tab_size,
    void* context
) {
    (void)toolbar;

    MessageComposer* composer = (MessageComposer*)context;
    if (composer == 0) {
        return;
    }

    composer->message_input_strip.set_code_mode(code_mode);
    composer->message_input_strip.set_tab_size(tab_size);
}

void MessageComposer::build_draft(MessageDraft& draft) const {
    draft.clear();

    FormattedText body;
    message_input_strip.get_formatted_text(body);

    if (message_toolbar.get_code_mode() && !body.empty()) {
        FormattedText fenced_body;
        TextFormat fence_format(false, false, false, 12);

        fenced_body.append_plain_text("```\n", fence_format);
        fenced_body.append_formatted_text(body);

        const char* body_text = body.get_text();
        int body_length = body.get_length();
        if (
            body_text != 0 &&
            body_length > 0 &&
            body_text[body_length - 1] != '\n'
        ) {
            fenced_body.append_plain_text("\n", fence_format);
        }

        fenced_body.append_plain_text("```", fence_format);
        draft.set_body(fenced_body);
    } else {
        draft.set_body(body);
    }

    for (int index = 0; index < (int)attachment_paths.size(); ++index) {
        draft.add_attachment(attachment_paths[index].c_str());
    }
}

void MessageComposer::add_attachments(
    const std::vector<std::string>& paths
) {
    for (int index = 0; index < (int)paths.size(); ++index) {
        if (!paths[index].empty()) {
            attachment_paths.push_back(paths[index]);
        }
    }

    message_toolbar.set_attachment_count((int)attachment_paths.size());
    message_input_strip.set_allow_empty_submit(!attachment_paths.empty());
}

void MessageComposer::clear_attachments() {
    attachment_paths.clear();
    message_toolbar.set_attachment_count(0);
    message_input_strip.set_allow_empty_submit(false);
}

void MessageComposer::sync_list_state() {
    TextInput::ListStyle input_style =
        message_input_strip.get_current_list_style();
    ListPanel::ListStyle toolbar_style = ListPanel::list_clear;

    if (input_style == TextInput::list_bulleted) {
        toolbar_style = ListPanel::list_bulleted;
    } else if (input_style == TextInput::list_numbered) {
        toolbar_style = ListPanel::list_numbered;
    }

    message_toolbar.set_list_style(toolbar_style);
}
