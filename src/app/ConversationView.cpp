// =================================================================================
// Filename:    app/ConversationView.cpp
// Author:      Ebdsaleh
// Description: Implements the append-only scrollable conversation message surface.
// =================================================================================

#include <string.h>

#include "ConversationView.h"
#include "framework/MarkdownFormatter.h"
#include "framework/NativeControlHost.h"
#include "framework/TextMetrics.h"
#include "framework/TextWrapLayout.h"
#include "framework/UIEvent.h"
#include "framework/rendering/ComponentRenderer.h"

namespace {
    bool is_whole_fenced_code_block(const char* text) {
        if (text == 0 || text[0] == '\0') {
            return false;
        }

        int length = (int)strlen(text);
        int first = 0;

        while (
            first < length &&
            (text[first] == ' ' || text[first] == '\t' || text[first] == '\r')
        ) {
            ++first;
        }

        if (
            first + 3 > length ||
            text[first] != '`' ||
            text[first + 1] != '`' ||
            text[first + 2] != '`'
        ) {
            return false;
        }

        int last = length - 1;
        while (
            last >= 0 &&
            (
                text[last] == ' ' ||
                text[last] == '\t' ||
                text[last] == '\r' ||
                text[last] == '\n'
            )
        ) {
            --last;
        }

        if (last < 2) {
            return false;
        }

        int last_line_start = last;
        while (last_line_start > 0 && text[last_line_start - 1] != '\n') {
            --last_line_start;
        }

        while (
            last_line_start <= last &&
            (text[last_line_start] == ' ' || text[last_line_start] == '\t')
        ) {
            ++last_line_start;
        }

        return
            last_line_start + 2 <= last &&
            text[last_line_start] == '`' &&
            text[last_line_start + 1] == '`' &&
            text[last_line_start + 2] == '`' &&
            last_line_start > first;
    }

    void mark_code_block(FormattedText& text) {
        for (int index = 0; index < text.get_length(); ++index) {
            TextFormat format = text.get_character_format(index);
            format.code_style = TextFormat::code_block;
            if (format.font_size > 11) {
                format.font_size = 11;
            }
            text.set_character_format(index, format);
        }
    }
}

ConversationView::ConversationView()
    : vertical_scroll_bar(ScrollBar::vertical),
      native_control_host(0),
      scroll_offset_y(0),
      row_spacing(6),
      content_padding(4),
      scroll_bar_width(16),
      line_step_pixels(24),
      last_message_width(0),
      layout_dirty(true) {

    get_style().background_color = Color(255, 255, 255);
    get_style().border_width = 0;

    vertical_scroll_bar.set_line_step(line_step_pixels);
    vertical_scroll_bar.set_value_changed_handler(
        ConversationView::on_scroll_changed,
        this
    );
    vertical_scroll_bar.set_visible(false);

    add_child(&vertical_scroll_bar);
}

ConversationView::~ConversationView() {
    detach_native_controls();
    clear_messages();
}

bool ConversationView::append_message(
    MessageRole role,
    const char* text
) {
    if (text == 0 || text[0] == '\0') {
        return false;
    }

    FormattedText formatted_text;
    formatted_text.set_plain_text(text, TextFormat());
    return append_message(role, formatted_text);
}

bool ConversationView::append_message(
    MessageRole role,
    const FormattedText& text
) {
    if (text.empty()) {
        return false;
    }

    Label* message_label = new Label();
    if (message_label == 0) {
        return false;
    }

    FormattedText markdown_text;
    if (!MarkdownFormatter::format(text, markdown_text)) {
        markdown_text = text;
    }

    if (is_whole_fenced_code_block(text.get_text())) {
        mark_code_block(markdown_text);
    }

    bool block_layout = MarkdownFormatter::has_block_structure(
        text.get_text()
    );

    FormattedText display_text;
    TextFormat role_format(true, false, false, 12);

    if (block_layout) {
        display_text.append_plain_text(
            get_role_label(role),
            role_format
        );
        display_text.append_plain_text("\n\n", role_format);
    } else {
        display_text.append_plain_text(
            get_role_prefix(role),
            role_format
        );
    }

    display_text.append_formatted_text(markdown_text);

    message_label->set_formatted_text(display_text);
    message_label->set_horizontal_alignment(Label::align_left);
    message_label->set_word_wrap(true);
    message_label->set_selectable(true);
    message_label->get_style().foreground_color = get_role_color(role);

    if (!add_child(message_label)) {
        delete message_label;
        return false;
    }

    MessageEntry entry;
    entry.role = role;
    entry.source_text = text;
    entry.label = message_label;
    entry.row_height = calculate_entry_height(
        *message_label,
        last_message_width,
        0
    );
    messages.push_back(entry);

    layout_dirty = true;
    relayout();
    scroll_to_bottom();
    return true;
}

bool ConversationView::append_system_message(const char* text) {
    return append_message(message_system, text);
}

bool ConversationView::append_local_message(const char* text) {
    return append_message(message_local, text);
}

bool ConversationView::append_local_message(
    const FormattedText& text
) {
    return append_message(message_local, text);
}

bool ConversationView::append_remote_message(const char* text) {
    return append_message(message_remote, text);
}

bool ConversationView::append_remote_message(
    const FormattedText& text
) {
    return append_message(message_remote, text);
}

void ConversationView::clear_messages() {
    for (int index = 0; index < (int)messages.size(); ++index) {
        Label* label = messages[index].label;
        if (label != 0) {
            remove_child(label);
            delete label;
        }
    }

    messages.clear();
    scroll_offset_y = 0;
    last_message_width = 0;
    layout_dirty = true;
    vertical_scroll_bar.set_range(0, 0, 1);
    vertical_scroll_bar.set_value(0);
    vertical_scroll_bar.set_visible(false);
    sync_native_scrollbar();
    relayout();
}

int ConversationView::get_message_count() const {
    return (int)messages.size();
}

void ConversationView::attach_native_controls(
    NativeControlHost* control_host
) {
    if (native_control_host == control_host) {
        return;
    }

    detach_native_controls();
    native_control_host = control_host;

    if (native_control_host != 0) {
        native_control_host->attach_scroll_bar(&vertical_scroll_bar);
        sync_native_scrollbar();
    }
}

void ConversationView::detach_native_controls() {
    if (native_control_host == 0) {
        return;
    }

    native_control_host->detach_scroll_bar(&vertical_scroll_bar);
    native_control_host = 0;
}

void ConversationView::arrange(
    int x,
    int y,
    int width,
    int height,
    TextMetrics* text_metrics
) {
    set_bounds(x, y, width, height);

    int available_height = height - (content_padding * 2);
    if (available_height < 0) {
        available_height = 0;
    }

    int base_message_width = width - (content_padding * 2);
    if (base_message_width < 0) {
        base_message_width = 0;
    }

    recalculate_entry_heights(base_message_width, text_metrics);

    bool show_scrollbar =
        calculate_total_content_height() > available_height;

    int final_message_width = base_message_width;
    if (show_scrollbar) {
        final_message_width -= scroll_bar_width + 4;
        if (final_message_width < 0) {
            final_message_width = 0;
        }
    }

    if (final_message_width != base_message_width) {
        recalculate_entry_heights(final_message_width, text_metrics);
    }

    last_message_width = final_message_width;
    layout_dirty = text_metrics == 0;
    relayout();
}

void ConversationView::scroll_lines(int line_count) {
    scroll_pixels(line_count * line_step_pixels);
}

void ConversationView::scroll_pixels(int pixel_count) {
    if (pixel_count == 0 || messages.empty()) {
        return;
    }

    scroll_offset_y += pixel_count;
    clamp_scroll_offset();
    vertical_scroll_bar.set_value(scroll_offset_y);
    relayout();
}

void ConversationView::scroll_to_bottom() {
    scroll_offset_y = calculate_max_scroll_offset();
    clamp_scroll_offset();
    vertical_scroll_bar.set_value(scroll_offset_y);
    relayout();
}

bool ConversationView::handle_event(const UIEvent& event) {
    if (!get_is_visible()) {
        return false;
    }

    bool is_mouse_event =
        event.type == UIEvent::event_mouse_move ||
        event.type == UIEvent::event_mouse_down ||
        event.type == UIEvent::event_mouse_up ||
        event.type == UIEvent::event_mouse_wheel;

    if (is_mouse_event && !contains_point(event.x, event.y)) {
        return false;
    }

    if (layout_dirty && event.text_metrics != 0 && last_message_width > 0) {
        recalculate_entry_heights(last_message_width, event.text_metrics);
        layout_dirty = false;
        relayout();
    }

    if (
        event.type == UIEvent::event_mouse_wheel &&
        event.wheel_delta != 0
    ) {
        int notches = event.wheel_delta / 120;
        if (notches == 0) {
            notches = event.wheel_delta > 0 ? 1 : -1;
        }

        scroll_pixels(-notches * line_step_pixels * 3);
        return true;
    }

    return Panel::handle_event(event);
}

void ConversationView::render(ComponentRenderer& renderer) const {
    if (!get_is_visible()) {
        return;
    }

    renderer.push_clip_rect(
        get_x(),
        get_y(),
        get_width(),
        get_height()
    );
    Panel::render(renderer);
    renderer.pop_clip_rect();
}

void ConversationView::on_scroll_changed(
    ScrollBar* scroll_bar,
    int value,
    void* context
) {
    (void)scroll_bar;

    ConversationView* conversation_view = (ConversationView*)context;
    if (conversation_view == 0) {
        return;
    }

    conversation_view->scroll_offset_y = value;
    conversation_view->clamp_scroll_offset();
    conversation_view->relayout();
}

void ConversationView::relayout() {
    int view_x = get_x();
    int view_y = get_y();
    int view_width = get_width();
    int view_height = get_height();

    int available_height = view_height - (content_padding * 2);
    if (available_height < 0) {
        available_height = 0;
    }

    bool show_scrollbar =
        calculate_total_content_height() > available_height;

    int reserved_scroll_width = show_scrollbar
        ? scroll_bar_width + 4
        : 0;

    int message_width = view_width -
        (content_padding * 2) -
        reserved_scroll_width;
    if (message_width < 0) {
        message_width = 0;
    }

    if (message_width != last_message_width) {
        recalculate_entry_heights(message_width, 0);
        last_message_width = message_width;
        show_scrollbar =
            calculate_total_content_height() > available_height;
    }

    clamp_scroll_offset();

    int viewport_top = view_y + content_padding;
    int viewport_bottom = view_y + view_height - content_padding;
    int current_y = viewport_top - scroll_offset_y;

    for (int index = 0; index < (int)messages.size(); ++index) {
        Label* label = messages[index].label;
        if (label == 0) {
            continue;
        }

        int row_height = messages[index].row_height;
        if (row_height < 1) {
            row_height = 1;
        }

        int row_bottom = current_y + row_height;
        bool is_visible =
            row_bottom > viewport_top &&
            current_y < viewport_bottom;

        label->set_visible(is_visible);
        label->set_bounds(
            view_x + content_padding,
            current_y,
            message_width,
            row_height
        );

        current_y = row_bottom + row_spacing;
    }

    vertical_scroll_bar.set_visible(show_scrollbar);

    if (show_scrollbar) {
        vertical_scroll_bar.arrange(
            view_x + view_width - content_padding - scroll_bar_width,
            view_y + content_padding,
            scroll_bar_width,
            available_height
        );
    } else {
        vertical_scroll_bar.arrange(0, 0, 0, 0);
    }

    update_scrollbar_state(available_height);
}

void ConversationView::recalculate_entry_heights(
    int message_width,
    TextMetrics* text_metrics
) {
    if (message_width < 0) {
        message_width = 0;
    }

    for (int index = 0; index < (int)messages.size(); ++index) {
        Label* label = messages[index].label;
        if (label == 0) {
            continue;
        }

        messages[index].row_height = calculate_entry_height(
            *label,
            message_width,
            text_metrics
        );
    }

    last_message_width = message_width;
    layout_dirty = text_metrics == 0;
}

void ConversationView::clamp_scroll_offset() {
    int maximum = calculate_max_scroll_offset();

    if (scroll_offset_y < 0) {
        scroll_offset_y = 0;
    }

    if (scroll_offset_y > maximum) {
        scroll_offset_y = maximum;
    }
}

void ConversationView::update_scrollbar_state(int available_height) {
    int maximum = calculate_max_scroll_offset();

    if (available_height < 1) {
        available_height = 1;
    }

    vertical_scroll_bar.set_range(
        0,
        maximum,
        available_height
    );
    vertical_scroll_bar.set_value(scroll_offset_y);
    sync_native_scrollbar();
}

void ConversationView::sync_native_scrollbar() {
    if (native_control_host == 0) {
        return;
    }

    native_control_host->sync_scroll_bar(&vertical_scroll_bar);
}

int ConversationView::calculate_entry_height(
    const Label& label,
    int message_width,
    TextMetrics* text_metrics
) const {
    const char* text = label.get_text();
    int text_length = text == 0 ? 0 : (int)strlen(text);

    if (text_metrics != 0 && message_width > 0) {
        int measured_height = TextWrapLayout::measure_height(
            text,
            text_length,
            label.get_format_data(),
            label.get_format_count(),
            message_width,
            2,
            text_metrics
        );

        if (measured_height > 0) {
            return measured_height + 8;
        }
    }

    int maximum_font_size = label.get_max_font_size();
    int line_height = maximum_font_size + 8;
    if (line_height < 20) {
        line_height = 20;
    }

    int approximate_character_width = maximum_font_size / 2;
    if (approximate_character_width < 6) {
        approximate_character_width = 6;
    }

    int characters_per_line = message_width > 0
        ? message_width / approximate_character_width
        : 80;
    if (characters_per_line < 1) {
        characters_per_line = 1;
    }

    int visual_line_count = 0;
    int logical_length = 0;

    for (int index = 0; index <= text_length; ++index) {
        bool line_end =
            index >= text_length ||
            (text != 0 && text[index] == '\n');

        if (!line_end) {
            ++logical_length;
            continue;
        }

        int wrapped_lines = logical_length <= 0
            ? 1
            : (logical_length + characters_per_line - 1) /
                characters_per_line;
        visual_line_count += wrapped_lines;
        logical_length = 0;
    }

    if (visual_line_count < 1) {
        visual_line_count = 1;
    }

    return
        (visual_line_count * line_height) +
        ((visual_line_count - 1) * 2) +
        8;
}

int ConversationView::calculate_total_content_height() const {
    if (messages.empty()) {
        return 0;
    }

    int total_height = 0;

    for (int index = 0; index < (int)messages.size(); ++index) {
        int row_height = messages[index].row_height;
        if (row_height < 1) {
            row_height = 1;
        }

        total_height += row_height;
        if (index + 1 < (int)messages.size()) {
            total_height += row_spacing;
        }
    }

    return total_height;
}

int ConversationView::calculate_max_scroll_offset() const {
    int available_height = get_height() - (content_padding * 2);
    if (available_height < 0) {
        available_height = 0;
    }

    int maximum = calculate_total_content_height() - available_height;
    return maximum > 0 ? maximum : 0;
}

const char* ConversationView::get_role_prefix(MessageRole role) const {
    switch (role) {
        case message_local:
            return "You: ";

        case message_remote:
            return "Remote: ";

        case message_system:
        default:
            return "System: ";
    }
}

const char* ConversationView::get_role_label(MessageRole role) const {
    switch (role) {
        case message_local:
            return "You:";

        case message_remote:
            return "Remote:";

        case message_system:
        default:
            return "System:";
    }
}

Color ConversationView::get_role_color(MessageRole role) const {
    switch (role) {
        case message_local:
            return Color(30, 72, 118);

        case message_remote:
            return Color(46, 101, 58);

        case message_system:
        default:
            return Color(92, 92, 92);
    }
}
