// =================================================================================
// Filename:    app/MessageComposer.cpp
// Author:      Ebdsaleh
// Description: Implements the complete messenger composer.
// =================================================================================

#include <stdio.h>
#include <string.h>

#include "MessageComposer.h"
#include "framework/FormattedText.h"
#include "framework/TextFormat.h"
#include "framework/MimeData.h"
#include "framework/UIEvent.h"
#include "conversation/ConversationAttachmentPolicy.h"

namespace {
    bool get_file_size(
        const std::string& path,
        unsigned long& file_size
    ) {
        file_size = 0;

        if (path.empty()) {
            return false;
        }

        FILE* file = fopen(path.c_str(), "rb");
        if (file == 0) {
            return false;
        }

        bool valid = false;

        if (fseek(file, 0, SEEK_END) == 0) {
            long length = ftell(file);

            if (length >= 0) {
                file_size = (unsigned long)length;
                valid = true;
            }
        }

        fclose(file);
        return valid;
    }

    unsigned long calculate_attachment_bytes(
        const std::vector<std::string>& paths
    ) {
        unsigned long total = 0;

        for (int index = 0; index < (int)paths.size(); ++index) {
            unsigned long file_size = 0;

            if (get_file_size(paths[index], file_size)) {
                total += file_size;
            }
        }

        return total;
    }

    TextFormat get_portable_format(const TextFormat& source_format) {
        TextFormat format = source_format;
        format.code_style = TextFormat::code_none;
        format.syntax_style = TextFormat::syntax_none;
        return format;
    }

    void append_source_character(
        FormattedText& destination,
        const FormattedText& source,
        int index
    ) {
        const char* source_text = source.get_text();
        if (
            source_text == 0 ||
            index < 0 ||
            index >= source.get_length()
        ) {
            return;
        }

        char value[2];
        value[0] = source_text[index];
        value[1] = '\0';

        destination.append_plain_text(
            value,
            get_portable_format(source.get_character_format(index))
        );
    }

    bool contains_code_ranges(const FormattedText& body) {
        for (int index = 0; index < body.get_length(); ++index) {
            if (
                body.get_character_format(index).code_style ==
                TextFormat::code_block
            ) {
                return true;
            }
        }

        return false;
    }

    bool ends_with_newline(const FormattedText& text) {
        int length = text.get_length();
        const char* value = text.get_text();

        return
            value != 0 &&
            length > 0 &&
            value[length - 1] == '\n';
    }

    void append_open_fence(
        FormattedText& destination,
        const char* language,
        const TextFormat& fence_format
    ) {
        destination.append_plain_text("```", fence_format);

        if (language != 0 && language[0] != '\0') {
            destination.append_plain_text(language, fence_format);
        }

        destination.append_plain_text("\n", fence_format);
    }

    void serialize_code_ranges(
        const FormattedText& body,
        FormattedText& serialized,
        const char* language
    ) {
        serialized.clear();

        const char* source_text = body.get_text();
        int source_length = body.get_length();
        if (source_text == 0 || source_length <= 0) {
            return;
        }

        TextFormat fence_format(false, false, false, 12);
        bool in_code = false;

        for (int index = 0; index < source_length; ++index) {
            bool is_code =
                body.get_character_format(index).code_style ==
                TextFormat::code_block;

            if (is_code && !in_code) {
                if (
                    serialized.get_length() > 0 &&
                    !ends_with_newline(serialized)
                ) {
                    serialized.append_plain_text("\n", fence_format);
                }

                append_open_fence(serialized, language, fence_format);
                in_code = true;
            } else if (!is_code && in_code) {
                if (!ends_with_newline(serialized)) {
                    serialized.append_plain_text("\n", fence_format);
                }

                serialized.append_plain_text("```", fence_format);

                if (source_text[index] != '\n') {
                    serialized.append_plain_text("\n", fence_format);
                }

                in_code = false;
            }

            append_source_character(serialized, body, index);
        }

        if (in_code) {
            if (!ends_with_newline(serialized)) {
                serialized.append_plain_text("\n", fence_format);
            }

            serialized.append_plain_text("```", fence_format);
        }
    }

    void wrap_whole_body_as_code(
        const FormattedText& body,
        FormattedText& fenced_body,
        const char* language
    ) {
        fenced_body.clear();

        TextFormat fence_format(false, false, false, 12);
        append_open_fence(fenced_body, language, fence_format);

        for (int index = 0; index < body.get_length(); ++index) {
            append_source_character(fenced_body, body, index);
        }

        if (!ends_with_newline(fenced_body)) {
            fenced_body.append_plain_text("\n", fence_format);
        }

        fenced_body.append_plain_text("```", fence_format);
    }
}

MessageComposer::MessageComposer(FileDialog* file_dialog)
    : message_toolbar(file_dialog),
      pending_attachment_remove_index(-1),
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

    attachment_tray.set_attachment_removed_handler(
        MessageComposer::on_attachment_removed,
        this
    );
    attachment_tray.set_visible(false);

    message_input_strip.set_code_mode(message_toolbar.get_code_mode());
    message_input_strip.apply_code_style(message_toolbar.get_code_mode());
    message_input_strip.set_tab_size(message_toolbar.get_tab_size());

    add_child(&message_input_strip);
    add_child(&attachment_tray);
    add_child(&message_toolbar);

    sync_attachment_state();
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

const char* MessageComposer::get_code_language() const {
    return message_toolbar.get_code_language();
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

    int tray_height = attachment_tray.get_preferred_height();
    int tray_gap = tray_height > 0 ? gap : 0;

    int input_height =
        height -
        toolbar_height -
        gap -
        tray_height -
        tray_gap;
    if (input_height < 0) {
        input_height = 0;
    }

    message_toolbar.arrange(
        x,
        y,
        width,
        toolbar_height
    );

    int next_y = y + toolbar_height + gap;

    if (tray_height > 0) {
        attachment_tray.set_visible(true);
        attachment_tray.arrange(
            x,
            next_y,
            width,
            tray_height
        );
        next_y += tray_height + tray_gap;
    } else {
        attachment_tray.set_visible(false);
        attachment_tray.arrange(0, 0, 0, 0);
    }

    message_input_strip.arrange(
        x,
        next_y,
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

    if (
        attachment_tray.get_is_visible() &&
        attachment_tray.handle_event(event)
    ) {
        if (pending_attachment_remove_index >= 0) {
            int remove_index = pending_attachment_remove_index;
            pending_attachment_remove_index = -1;
            remove_attachment(remove_index);
        }

        sync_list_state();
        return true;
    }

    if (pending_attachment_remove_index >= 0) {
        int remove_index = pending_attachment_remove_index;
        pending_attachment_remove_index = -1;
        remove_attachment(remove_index);
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

    composer->message_input_strip.set_text_format_preserving_code(
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
    const char* code_language,
    void* context
) {
    (void)toolbar;
    (void)code_language;

    MessageComposer* composer = (MessageComposer*)context;
    if (composer == 0) {
        return;
    }

    composer->message_input_strip.set_code_mode(code_mode);
    composer->message_input_strip.apply_code_style(code_mode);
    composer->message_input_strip.set_tab_size(tab_size);
}

void MessageComposer::on_attachment_removed(
    AttachmentTray* tray,
    int index,
    void* context
) {
    (void)tray;

    MessageComposer* composer = (MessageComposer*)context;
    if (composer == 0) {
        return;
    }

    composer->pending_attachment_remove_index = index;
}

void MessageComposer::build_draft(MessageDraft& draft) const {
    draft.clear();

    FormattedText body;
    message_input_strip.get_formatted_text(body);

    const char* code_language = message_toolbar.get_code_language();

    if (contains_code_ranges(body)) {
        FormattedText serialized_body;
        serialize_code_ranges(body, serialized_body, code_language);
        draft.set_body(serialized_body);
    } else if (message_toolbar.get_code_mode() && !body.empty()) {
        FormattedText fenced_body;
        wrap_whole_body_as_code(body, fenced_body, code_language);
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
    const int maximum_count =
        ConversationAttachmentPolicy::maximum_attachment_count;
    const unsigned long maximum_file_bytes =
        ConversationAttachmentPolicy::maximum_attachment_bytes();
    const unsigned long maximum_total_bytes =
        ConversationAttachmentPolicy::maximum_total_attachment_bytes();

    unsigned long total_bytes =
        calculate_attachment_bytes(attachment_paths);

    int rejected_count = 0;
    int rejected_file_size = 0;
    int rejected_total_size = 0;
    int rejected_unreadable = 0;

    attachment_notice.clear();

    for (int index = 0; index < (int)paths.size(); ++index) {
        if (paths[index].empty()) {
            continue;
        }

        if ((int)attachment_paths.size() >= maximum_count) {
            ++rejected_count;
            continue;
        }

        unsigned long file_size = 0;
        if (!get_file_size(paths[index], file_size)) {
            ++rejected_unreadable;
            continue;
        }

        if (file_size > maximum_file_bytes) {
            ++rejected_file_size;
            continue;
        }

        if (
            total_bytes > maximum_total_bytes ||
            file_size > maximum_total_bytes - total_bytes
        ) {
            ++rejected_total_size;
            continue;
        }

        attachment_paths.push_back(paths[index]);
        total_bytes += file_size;
    }

    if (
        rejected_count > 0 ||
        rejected_file_size > 0 ||
        rejected_total_size > 0 ||
        rejected_unreadable > 0
    ) {
        if (
            rejected_count > 0 &&
            rejected_file_size == 0 &&
            rejected_total_size == 0 &&
            rejected_unreadable == 0
        ) {
            set_attachment_notice("limit reached: 8 files");
        } else if (
            rejected_file_size > 0 &&
            rejected_count == 0 &&
            rejected_total_size == 0 &&
            rejected_unreadable == 0
        ) {
            set_attachment_notice("rejected: file exceeds 2 MB");
        } else if (
            rejected_total_size > 0 &&
            rejected_count == 0 &&
            rejected_file_size == 0 &&
            rejected_unreadable == 0
        ) {
            set_attachment_notice("rejected: 4 MB total limit");
        } else if (
            rejected_unreadable > 0 &&
            rejected_count == 0 &&
            rejected_file_size == 0 &&
            rejected_total_size == 0
        ) {
            set_attachment_notice("rejected: file could not be read");
        } else {
            set_attachment_notice(
                "some rejected: 8 files, 2 MB each, 4 MB total"
            );
        }
    }

    sync_attachment_state();
}

void MessageComposer::remove_attachment(int index) {
    if (index < 0 || index >= (int)attachment_paths.size()) {
        return;
    }

    attachment_paths.erase(attachment_paths.begin() + index);
    attachment_notice.clear();
    sync_attachment_state();
}

void MessageComposer::clear_attachments() {
    pending_attachment_remove_index = -1;
    attachment_paths.clear();
    attachment_notice.clear();
    sync_attachment_state();
}

void MessageComposer::sync_attachment_state() {
    int attachment_count = (int)attachment_paths.size();

    message_toolbar.set_attachment_status(
        attachment_count,
        ConversationAttachmentPolicy::maximum_attachment_count,
        attachment_notice.c_str()
    );
    message_input_strip.set_allow_empty_submit(attachment_count > 0);
    attachment_tray.set_paths(attachment_paths);
    attachment_tray.set_visible(attachment_count > 0);

    if (get_width() > 0 && get_height() > 0) {
        arrange(
            get_x(),
            get_y(),
            get_width(),
            get_height()
        );
    }
}

void MessageComposer::set_attachment_notice(
    const char* notice
) {
    attachment_notice = notice == 0 ? "" : notice;
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
