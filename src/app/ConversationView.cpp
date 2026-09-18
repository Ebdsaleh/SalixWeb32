// =================================================================================
// Filename:    app/ConversationView.cpp
// Author:      Ebdsaleh
// Description: Implements the append-only scrollable conversation message surface.
// =================================================================================

#include <string>

#include "ConversationView.h"
#include "ConversationMessageView.h"
#include "framework/Clipboard.h"
#include "framework/ContextMenu.h"
#include "framework/DesktopServices.h"
#include "framework/MimeData.h"
#include "framework/NativeControlHost.h"
#include "framework/RasterImage.h"
#include "framework/TextMetrics.h"
#include "framework/UIEvent.h"
#include "framework/rendering/ComponentRenderer.h"

namespace {
    enum ConversationContextCommand {
        context_copy = 1,
        context_select_message,
        context_select_conversation,

        context_attachment_preview = 100,
        context_attachment_open,
        context_attachment_copy_reference
    };

    const int attachment_block_spacing = 6;

    int calculate_image_height_for_width(
        const ImageView* image_view,
        int available_width
    ) {
        if (
            image_view == 0 ||
            !image_view->has_image() ||
            available_width <= 0
        ) {
            return 0;
        }

        const RasterImage& image = image_view->get_image();
        int image_width = image.get_width();
        int image_height = image.get_height();

        if (image_width <= 0 || image_height <= 0) {
            return 0;
        }

        if (image_width <= available_width) {
            return image_height;
        }

        int scaled_height = (int)(
            ((long)image_height * (long)available_width) /
            (long)image_width
        );

        return scaled_height > 0 ? scaled_height : 1;
    }

    int calculate_image_width_for_width(
        const ImageView* image_view,
        int available_width
    ) {
        if (
            image_view == 0 ||
            !image_view->has_image() ||
            available_width <= 0
        ) {
            return 0;
        }

        int image_width = image_view->get_image().get_width();
        return image_width < available_width
            ? image_width
            : available_width;
    }
}

ConversationView::ConversationView()
    : vertical_scroll_bar(ScrollBar::vertical),
      native_control_host(0),
      desktop_services(0),
      scroll_offset_y(0),
      row_spacing(8),
      content_padding(4),
      scroll_bar_width(16),
      line_step_pixels(24),
      last_message_width(0),
      layout_dirty(true),
      conversation_drag_selecting(false),
      selection_context_active(false),
      selection_anchor_is_attachment(false),
      selection_anchor_message_index(-1),
      selection_anchor_label_index(-1),
      selection_anchor_character_index(0) {

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

void ConversationView::set_desktop_services(DesktopServices* services) {
    desktop_services = services;
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

    ConversationMessageView* message_view = new ConversationMessageView();
    if (message_view == 0) {
        return false;
    }

    if (!message_view->set_message(
            text,
            get_role_label(role),
            get_role_prefix(role),
            get_role_color(role)
        )) {
        delete message_view;
        return false;
    }

    if (!add_child(message_view)) {
        delete message_view;
        return false;
    }

    MessageEntry entry;
    entry.role = role;
    entry.source_text = text;
    entry.view = message_view;
    entry.text_height = calculate_entry_height(
        *message_view,
        last_message_width,
        0
    );
    entry.row_height = entry.text_height;
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

bool ConversationView::update_message(
    int index,
    const char* text
) {
    if (text == 0 || text[0] == '\0') {
        return false;
    }

    FormattedText formatted_text;
    formatted_text.set_plain_text(text, TextFormat());
    return update_message(index, formatted_text);
}

bool ConversationView::update_message(
    int index,
    const FormattedText& text
) {
    if (
        index < 0 ||
        index >= (int)messages.size() ||
        text.empty()
    ) {
        return false;
    }

    MessageEntry& entry = messages[index];
    if (entry.view == 0 || entry.image_view != 0) {
        return false;
    }

    if (!entry.view->set_message(
            text,
            get_role_label(entry.role),
            get_role_prefix(entry.role),
            get_role_color(entry.role)
        )) {
        return false;
    }

    entry.source_text = text;
    entry.text_height = calculate_entry_height(
        *entry.view,
        last_message_width,
        0
    );
    entry.row_height = entry.text_height;

    layout_dirty = true;
    relayout();
    scroll_to_bottom();
    return true;
}

bool ConversationView::append_attachment(const Attachment& attachment) {
    if (attachment.empty()) {
        return false;
    }

    if (!append_system_message(attachment.get_file_name())) {
        return false;
    }

    MessageEntry& entry = messages[messages.size() - 1];
    entry.attachment = attachment;

    if (
        attachment.is_image() &&
        desktop_services != 0
    ) {
        RasterImage thumbnail;
        if (desktop_services->load_image_thumbnail(
                attachment.get_path(),
                400,
                400,
                thumbnail
            )) {
            ImageView* image_view = new ImageView();
            if (image_view != 0) {
                image_view->set_image(thumbnail);

                if (add_child(image_view)) {
                    entry.image_view = image_view;
                } else {
                    delete image_view;
                }
            }
        }
    }

    layout_dirty = true;
    recalculate_entry_heights(last_message_width, 0);
    relayout();
    scroll_to_bottom();
    return true;
}

void ConversationView::clear_messages() {
    for (int index = 0; index < (int)messages.size(); ++index) {
        ConversationMessageView* message_view = messages[index].view;
        if (message_view != 0) {
            remove_child(message_view);
            delete message_view;
        }

        ImageView* image_view = messages[index].image_view;
        if (image_view != 0) {
            remove_child(image_view);
            delete image_view;
        }
    }

    messages.clear();
    scroll_offset_y = 0;
    last_message_width = 0;
    layout_dirty = true;
    conversation_drag_selecting = false;
    selection_context_active = false;
    selection_anchor_is_attachment = false;
    selection_anchor_message_index = -1;
    selection_anchor_label_index = -1;
    selection_anchor_character_index = 0;
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

    bool inside_view = contains_point(event.x, event.y);

    if (event.type == UIEvent::event_mouse_down && !inside_view) {
        selection_context_active = false;
    }

    if (
        is_mouse_event &&
        !inside_view &&
        !(
            conversation_drag_selecting &&
            (
                event.type == UIEvent::event_mouse_move ||
                event.type == UIEvent::event_mouse_up
            )
        )
    ) {
        return false;
    }

    if (layout_dirty && event.text_metrics != 0 && last_message_width > 0) {
        recalculate_entry_heights(last_message_width, event.text_metrics);
        layout_dirty = false;
        relayout();
    }

    if (
        event.type == UIEvent::event_context_menu &&
        inside_view
    ) {
        selection_context_active = true;
        return handle_context_menu(event);
    }

    if (
        event.type == UIEvent::event_key_down &&
        event.control_down &&
        selection_context_active
    ) {
        if (event.key_code == UIEvent::key_c) {
            if (has_conversation_selection()) {
                copy_conversation_selection(event.clipboard);
                return true;
            }
        } else if (event.key_code == UIEvent::key_a) {
            if (!messages.empty()) {
                select_all_conversation();
                return true;
            }
        }
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

    if (
        event.type == UIEvent::event_mouse_down &&
        event.click_count == 1 &&
        !event.control_down &&
        !event.alt_down &&
        !event.shift_down
    ) {
        int message_index = find_message_at_point(event.x, event.y);

        if (message_index >= 0 && messages[message_index].view != 0) {
            int label_index = -1;
            int character_index = 0;

            if (messages[message_index].view->resolve_presentation_position(
                    event,
                    label_index,
                    character_index
                )) {
                clear_conversation_selection();
                selection_context_active = true;
                conversation_drag_selecting = true;
                selection_anchor_is_attachment = is_attachment_point(
                    message_index,
                    event.x,
                    event.y
                );
                selection_anchor_message_index = message_index;
                selection_anchor_label_index = label_index;
                selection_anchor_character_index = character_index;

                messages[message_index].view->set_presentation_selection(
                    label_index,
                    character_index,
                    label_index,
                    character_index
                );
                return true;
            }
        } else if (!vertical_scroll_bar.contains_point(event.x, event.y)) {
            clear_conversation_selection();
            selection_context_active = true;
        }
    }

    if (
        event.type == UIEvent::event_mouse_down &&
        inside_view &&
        find_message_at_point(event.x, event.y) >= 0
    ) {
        selection_context_active = true;
    }

    if (
        event.type == UIEvent::event_mouse_move &&
        conversation_drag_selecting &&
        event.left_button_down
    ) {
        int viewport_top = get_y() + content_padding;
        int viewport_bottom = get_y() + get_height() - content_padding;

        if (event.y < viewport_top) {
            scroll_pixels(-line_step_pixels);
        } else if (event.y >= viewport_bottom) {
            scroll_pixels(line_step_pixels);
        }

        int target_message_index = resolve_message_index_for_selection(
            event.y
        );

        if (
            target_message_index >= 0 &&
            messages[target_message_index].view != 0
        ) {
            int target_label_index = -1;
            int target_character_index = 0;

            if (messages[target_message_index].view->resolve_presentation_position(
                    event,
                    target_label_index,
                    target_character_index
                )) {
                apply_conversation_selection(
                    target_message_index,
                    target_label_index,
                    target_character_index,
                    is_attachment_point(
                        target_message_index,
                        event.x,
                        event.y
                    )
                );
            }
        }

        return true;
    }

    if (
        event.type == UIEvent::event_mouse_up &&
        conversation_drag_selecting
    ) {
        int target_message_index = resolve_message_index_for_selection(
            event.y
        );

        if (
            target_message_index >= 0 &&
            messages[target_message_index].view != 0
        ) {
            int target_label_index = -1;
            int target_character_index = 0;

            if (messages[target_message_index].view->resolve_presentation_position(
                    event,
                    target_label_index,
                    target_character_index
                )) {
                apply_conversation_selection(
                    target_message_index,
                    target_label_index,
                    target_character_index,
                    is_attachment_point(
                        target_message_index,
                        event.x,
                        event.y
                    )
                );
            }
        }

        conversation_drag_selecting = false;
        selection_anchor_is_attachment = false;
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
        MessageEntry& entry = messages[index];
        ConversationMessageView* message_view = entry.view;
        if (message_view == 0) {
            continue;
        }

        int row_height = entry.row_height;
        if (row_height < 1) {
            row_height = 1;
        }

        int row_bottom = current_y + row_height;
        bool is_visible =
            row_bottom > viewport_top &&
            current_y < viewport_bottom;

        message_view->set_visible(is_visible);
        message_view->arrange(
            view_x + content_padding,
            current_y,
            message_width,
            entry.text_height,
            0
        );

        if (entry.image_view != 0) {
            int image_width = calculate_image_width_for_width(
                entry.image_view,
                message_width
            );
            int image_height = calculate_image_height_for_width(
                entry.image_view,
                message_width
            );

            entry.image_view->set_visible(is_visible);
            entry.image_view->set_bounds(
                view_x + content_padding,
                current_y + entry.text_height + attachment_block_spacing,
                image_width,
                image_height
            );
        }

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
        MessageEntry& entry = messages[index];
        ConversationMessageView* message_view = entry.view;
        if (message_view == 0) {
            continue;
        }

        entry.text_height = calculate_entry_height(
            *message_view,
            message_width,
            text_metrics
        );
        entry.row_height = entry.text_height;

        if (entry.image_view != 0) {
            int image_height = calculate_image_height_for_width(
                entry.image_view,
                message_width
            );
            if (image_height > 0) {
                entry.row_height += attachment_block_spacing + image_height;
            }
        }
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
    ConversationMessageView& message_view,
    int message_width,
    TextMetrics* text_metrics
) const {
    int height = message_view.measure_height(
        message_width,
        text_metrics
    );

    return height > 0 ? height : 24;
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

int ConversationView::find_message_at_point(int x, int y) const {
    for (int index = 0; index < (int)messages.size(); ++index) {
        const MessageEntry& entry = messages[index];
        const ConversationMessageView* message_view = entry.view;

        if (
            message_view != 0 &&
            message_view->get_is_visible() &&
            message_view->contains_point(x, y)
        ) {
            return index;
        }

        if (
            entry.image_view != 0 &&
            entry.image_view->get_is_visible() &&
            entry.image_view->contains_point(x, y)
        ) {
            return index;
        }
    }

    return -1;
}

int ConversationView::resolve_message_index_for_selection(int y) const {
    if (messages.empty()) {
        return -1;
    }

    int first_valid = -1;
    int last_valid = -1;

    for (int index = 0; index < (int)messages.size(); ++index) {
        const ConversationMessageView* message_view = messages[index].view;
        if (message_view == 0) {
            continue;
        }

        if (first_valid < 0) {
            first_valid = index;
        }
        last_valid = index;

        int top = message_view->get_y();
        int bottom = top + messages[index].row_height;

        if (y >= top && y < bottom) {
            return index;
        }

        if (index + 1 < (int)messages.size()) {
            const ConversationMessageView* next_view = messages[index + 1].view;
            if (next_view != 0) {
                int next_top = next_view->get_y();
                if (y >= bottom && y < next_top) {
                    int distance_to_previous = y - bottom;
                    int distance_to_next = next_top - y;
                    return distance_to_previous <= distance_to_next
                        ? index
                        : index + 1;
                }
            }
        }
    }

    if (first_valid < 0) {
        return -1;
    }

    if (y < messages[first_valid].view->get_y()) {
        return first_valid;
    }

    return last_valid;
}

bool ConversationView::is_attachment_point(
    int message_index,
    int x,
    int y
) const {
    if (
        message_index < 0 ||
        message_index >= (int)messages.size()
    ) {
        return false;
    }

    const ImageView* image_view = messages[message_index].image_view;
    return
        image_view != 0 &&
        image_view->get_is_visible() &&
        image_view->contains_point(x, y);
}

void ConversationView::clear_conversation_selection() {
    for (int index = 0; index < (int)messages.size(); ++index) {
        if (messages[index].view != 0) {
            messages[index].view->clear_presentation_selection();
        }
    }
}

void ConversationView::select_all_conversation() {
    for (int index = 0; index < (int)messages.size(); ++index) {
        if (messages[index].view != 0) {
            messages[index].view->select_all_presentation();
        }
    }
}

bool ConversationView::has_conversation_selection() const {
    for (int index = 0; index < (int)messages.size(); ++index) {
        if (
            messages[index].view != 0 &&
            messages[index].view->has_presentation_selection()
        ) {
            return true;
        }
    }

    return false;
}

bool ConversationView::copy_conversation_selection(
    Clipboard* clipboard
) const {
    if (clipboard == 0) {
        return false;
    }

    std::string output;

    for (int index = 0; index < (int)messages.size(); ++index) {
        const ConversationMessageView* message_view = messages[index].view;
        if (message_view == 0) {
            continue;
        }

        std::string message_text;
        if (!message_view->get_presentation_selection_text(message_text)) {
            continue;
        }

        if (!output.empty()) {
            output += "\r\n";
        }

        output += message_text;
    }

    if (output.empty()) {
        return false;
    }

    MimeData data;
    data.set_text(output.c_str());
    return clipboard->set_data(data);
}

void ConversationView::apply_conversation_selection(
    int target_message_index,
    int target_label_index,
    int target_character_index,
    bool target_is_attachment
) {
    if (
        selection_anchor_message_index < 0 ||
        selection_anchor_message_index >= (int)messages.size() ||
        target_message_index < 0 ||
        target_message_index >= (int)messages.size()
    ) {
        return;
    }

    ConversationMessageView* anchor_view =
        messages[selection_anchor_message_index].view;
    ConversationMessageView* target_view =
        messages[target_message_index].view;

    if (anchor_view == 0 || target_view == 0) {
        return;
    }

    clear_conversation_selection();

    if (target_message_index == selection_anchor_message_index) {
        anchor_view->set_presentation_selection(
            selection_anchor_label_index,
            selection_anchor_character_index,
            target_label_index,
            target_character_index
        );

        if (selection_anchor_is_attachment || target_is_attachment) {
            anchor_view->select_all_presentation();
        }
        return;
    }

    if (target_message_index > selection_anchor_message_index) {
        int anchor_last_label = anchor_view->get_presentation_label_count() - 1;
        int anchor_last_character = anchor_view->get_presentation_label_length(
            anchor_last_label
        );

        anchor_view->set_presentation_selection(
            selection_anchor_label_index,
            selection_anchor_character_index,
            anchor_last_label,
            anchor_last_character
        );

        for (
            int index = selection_anchor_message_index + 1;
            index < target_message_index;
            ++index
        ) {
            if (messages[index].view != 0) {
                messages[index].view->select_all_presentation();
            }
        }

        target_view->set_presentation_selection(
            0,
            0,
            target_label_index,
            target_character_index
        );
    } else {
        int target_last_label = target_view->get_presentation_label_count() - 1;
        int target_last_character = target_view->get_presentation_label_length(
            target_last_label
        );

        target_view->set_presentation_selection(
            target_label_index,
            target_character_index,
            target_last_label,
            target_last_character
        );

        for (
            int index = target_message_index + 1;
            index < selection_anchor_message_index;
            ++index
        ) {
            if (messages[index].view != 0) {
                messages[index].view->select_all_presentation();
            }
        }

        anchor_view->set_presentation_selection(
            0,
            0,
            selection_anchor_label_index,
            selection_anchor_character_index
        );
    }

    if (selection_anchor_is_attachment) {
        anchor_view->select_all_presentation();
    }

    if (target_is_attachment) {
        target_view->select_all_presentation();
    }
}

bool ConversationView::handle_context_menu(const UIEvent& event) {
    NativeControlHost* menu_host = event.native_control_host != 0
        ? event.native_control_host
        : native_control_host;

    if (menu_host == 0) {
        return false;
    }

    int message_index = find_message_at_point(event.x, event.y);

    if (
        message_index >= 0 &&
        is_attachment_point(message_index, event.x, event.y)
    ) {
        return handle_attachment_context_menu(event, message_index);
    }

    bool has_selection = has_conversation_selection();
    bool has_messages = !messages.empty();

    ContextMenu menu;
    menu.add_item(context_copy, "Copy", has_selection);
    menu.add_separator();

    if (
        message_index >= 0 &&
        message_index < (int)messages.size()
    ) {
        std::string message_label = "Select All in ";
        message_label += get_role_context_name(messages[message_index].role);
        message_label += " Message";

        menu.add_item(
            context_select_message,
            message_label.c_str(),
            true
        );
    }

    menu.add_item(
        context_select_conversation,
        "Select All Conversation",
        has_messages
    );

    int command_id = menu_host->show_context_menu(
        menu,
        event.x,
        event.y
    );

    if (command_id == context_copy) {
        copy_conversation_selection(event.clipboard);
    } else if (
        command_id == context_select_message &&
        message_index >= 0 &&
        message_index < (int)messages.size() &&
        messages[message_index].view != 0
    ) {
        clear_conversation_selection();
        messages[message_index].view->select_all_presentation();
    } else if (command_id == context_select_conversation) {
        select_all_conversation();
    }

    return true;
}

bool ConversationView::handle_attachment_context_menu(
    const UIEvent& event,
    int message_index
) {
    if (
        message_index < 0 ||
        message_index >= (int)messages.size()
    ) {
        return false;
    }

    NativeControlHost* menu_host = event.native_control_host != 0
        ? event.native_control_host
        : native_control_host;

    if (menu_host == 0) {
        return false;
    }

    const Attachment& attachment = messages[message_index].attachment;
    if (attachment.empty()) {
        return false;
    }

    ContextMenu menu;
    menu.add_item(
        context_attachment_preview,
        "Preview",
        attachment.is_image() && desktop_services != 0
    );
    menu.add_item(
        context_attachment_open,
        "Open",
        desktop_services != 0
    );
    menu.add_separator();
    menu.add_item(
        context_attachment_copy_reference,
        "Copy Reference",
        event.clipboard != 0
    );

    int command_id = menu_host->show_context_menu(
        menu,
        event.x,
        event.y
    );

    if (
        command_id == context_attachment_preview &&
        desktop_services != 0
    ) {
        desktop_services->preview_image(
            attachment.get_path(),
            attachment.get_file_name()
        );
    } else if (
        command_id == context_attachment_open &&
        desktop_services != 0
    ) {
        desktop_services->open_file(attachment.get_path());
    } else if (
        command_id == context_attachment_copy_reference &&
        event.clipboard != 0
    ) {
        std::string reference("System: ");
        reference += attachment.get_file_name();
        MimeData data;
        data.set_text(reference.c_str());
        event.clipboard->set_data(data);
    }

    return true;
}

const char* ConversationView::get_role_context_name(MessageRole role) const {
    switch (role) {
        case message_local:
            return "User";

        case message_remote:
            return "Remote";

        case message_system:
        default:
            return "System";
    }
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
