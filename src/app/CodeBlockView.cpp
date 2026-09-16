// =================================================================================
// Filename:    app/CodeBlockView.cpp
// Author:      Ebdsaleh
// Description: Implements the dedicated conversation code-block component.
// =================================================================================

#include <ctype.h>
#include <string.h>

#include "CodeBlockView.h"
#include "framework/Clipboard.h"
#include "framework/Component.h"
#include "framework/MimeData.h"
#include "framework/NativeControlHost.h"
#include "framework/TextMetrics.h"
#include "framework/UIEvent.h"
#include "framework/rendering/ComponentRenderer.h"

namespace {
    std::string lower_copy(const std::string& value) {
        std::string result = value;
        for (int index = 0; index < (int)result.length(); ++index) {
            result[index] = (char)tolower((unsigned char)result[index]);
        }
        return result;
    }

    std::string get_display_language(const std::string& language) {
        if (language.empty()) {
            return "Code";
        }

        std::string lower = lower_copy(language);

        if (lower == "c") {
            return "C";
        }
        if (lower == "cpp" || lower == "c++" || lower == "cxx") {
            return "C++";
        }
        if (lower == "python" || lower == "py") {
            return "Python";
        }
        if (lower == "javascript" || lower == "js") {
            return "JavaScript";
        }
        if (lower == "typescript" || lower == "ts") {
            return "TypeScript";
        }
        if (lower == "csharp" || lower == "cs" || lower == "c#") {
            return "C#";
        }
        if (lower == "bash" || lower == "sh" || lower == "shell") {
            return "Shell";
        }
        if (lower == "html") {
            return "HTML";
        }
        if (lower == "css") {
            return "CSS";
        }
        if (lower == "json") {
            return "JSON";
        }
        if (lower == "xml") {
            return "XML";
        }
        if (lower == "lua") {
            return "Lua";
        }
        if (lower == "rust" || lower == "rs") {
            return "Rust";
        }
        if (lower == "java") {
            return "Java";
        }
        if (lower == "text" || lower == "plaintext" || lower == "txt") {
            return "Plain text";
        }

        return language;
    }

    bool is_mouse_event(const UIEvent& event) {
        return
            event.type == UIEvent::event_mouse_move ||
            event.type == UIEvent::event_mouse_down ||
            event.type == UIEvent::event_mouse_up ||
            event.type == UIEvent::event_mouse_wheel;
    }
}

CodeBlockView::CodeBlockView()
    : horizontal_scroll_bar(ScrollBar::horizontal),
      native_control_host(0),
      active_clipboard(0),
      header_height(24),
      block_padding(1),
      body_padding(8),
      block_spacing(0),
      scroll_bar_height(16),
      content_width(0),
      content_height(18),
      viewport_width(0),
      scroll_offset_x(0),
      layout_dirty(true) {

    get_style().background_color = Color(204, 204, 204);
    get_style().border_color = Color(154, 154, 154);
    get_style().border_width = 1;

    header_panel.get_style().background_color = Color(226, 226, 226);
    header_panel.get_style().border_color = Color(184, 184, 184);
    header_panel.get_style().border_width = 1;

    body_panel.get_style().background_color = Color(244, 244, 244);
    body_panel.get_style().border_width = 0;

    language_label.set_text("Code");
    language_label.set_horizontal_alignment(Label::align_left);
    language_label.get_style().foreground_color = Color(62, 62, 62);

    copy_button.set_text("Copy");
    copy_button.set_click_handler(CodeBlockView::on_copy_clicked, this);
    copy_button.get_style().background_color = Color(238, 238, 238);
    copy_button.get_style().border_color = Color(160, 160, 160);
    copy_button.get_style().border_width = 1;
    copy_button.get_style().foreground_color = Color(50, 50, 50);

    code_label.set_horizontal_alignment(Label::align_left);
    code_label.set_word_wrap(false);
    code_label.set_selectable(true);
    code_label.get_style().foreground_color = Color(35, 35, 35);

    horizontal_scroll_bar.set_line_step(24);
    horizontal_scroll_bar.set_value_changed_handler(
        CodeBlockView::on_scroll_changed,
        this
    );
    horizontal_scroll_bar.set_visible(false);

    header_panel.add_child(&language_label);
    header_panel.add_child(&copy_button);
}

CodeBlockView::~CodeBlockView() {
    detach_native_controls();
}

void CodeBlockView::set_code(
    const FormattedText& new_code,
    const char* new_language
) {
    source_code = new_code.get_text() == 0 ? "" : new_code.get_text();
    language = new_language == 0 ? "" : new_language;
    scroll_offset_x = 0;
    rebuild_formatted_code();
    update_language_label();
    layout_dirty = true;
    layout_children();
}

const char* CodeBlockView::get_code() const {
    return source_code.c_str();
}

const char* CodeBlockView::get_language() const {
    return language.c_str();
}

void CodeBlockView::attach_native_controls(
    NativeControlHost* control_host
) {
    if (native_control_host == control_host) {
        return;
    }

    detach_native_controls();
    native_control_host = control_host;

    if (native_control_host != 0) {
        native_control_host->attach_scroll_bar(&horizontal_scroll_bar);
        sync_native_scrollbar();
    }
}

void CodeBlockView::detach_native_controls() {
    if (native_control_host == 0) {
        return;
    }

    native_control_host->detach_scroll_bar(&horizontal_scroll_bar);
    native_control_host = 0;
}

int CodeBlockView::measure_height(
    int width,
    TextMetrics* text_metrics
) {
    if (width < 0) {
        width = 0;
    }

    viewport_width = width - (block_padding * 2) - (body_padding * 2);
    if (viewport_width < 1) {
        viewport_width = 1;
    }

    update_content_metrics(text_metrics);

    bool show_scrollbar = content_width > viewport_width;
    int body_height = body_padding + content_height + body_padding;

    if (show_scrollbar) {
        body_height += 4 + scroll_bar_height;
    }

    int total_height =
        block_padding +
        header_height +
        block_spacing +
        body_height +
        block_padding;

    if (total_height < 54) {
        total_height = 54;
    }

    return total_height;
}

void CodeBlockView::arrange(
    int x,
    int y,
    int width,
    int height,
    TextMetrics* text_metrics
) {
    if (width < 0) {
        width = 0;
    }
    if (height < 0) {
        height = 0;
    }

    set_bounds(x, y, width, height);

    if (text_metrics != 0 || layout_dirty) {
        update_content_metrics(text_metrics);
    }

    layout_children();
}

bool CodeBlockView::handle_event(const UIEvent& event) {
    if (!get_is_visible()) {
        return false;
    }

    bool mouse_event = is_mouse_event(event);
    if (mouse_event && !contains_point(event.x, event.y)) {
        return false;
    }

    active_clipboard = event.clipboard;
    bool handled = false;

    if (header_panel.handle_event(event)) {
        handled = true;
    }

    if (horizontal_scroll_bar.handle_event(event)) {
        handled = true;
    }

    int code_left = body_panel.get_x() + body_padding;
    int code_top = body_panel.get_y() + body_padding;
    int code_right = body_panel.get_x() + body_panel.get_width() - body_padding;
    int code_bottom = code_top + content_height;

    if (
        !mouse_event ||
        (
            event.x >= code_left &&
            event.x < code_right &&
            event.y >= code_top &&
            event.y < code_bottom
        )
    ) {
        if (code_label.handle_event(event)) {
            handled = true;
        }
    }

    active_clipboard = 0;
    return handled;
}

void CodeBlockView::render(ComponentRenderer& renderer) const {
    if (!get_is_visible()) {
        return;
    }

    renderer.render_panel(*this);
    header_panel.render(renderer);
    body_panel.render(renderer);

    int clip_x = body_panel.get_x() + body_padding;
    int clip_y = body_panel.get_y() + body_padding;
    int clip_width = body_panel.get_width() - (body_padding * 2);
    int clip_height = content_height;

    if (clip_width < 0) {
        clip_width = 0;
    }
    if (clip_height < 0) {
        clip_height = 0;
    }

    renderer.push_clip_rect(
        clip_x,
        clip_y,
        clip_width,
        clip_height
    );
    code_label.render(renderer);
    renderer.pop_clip_rect();

    horizontal_scroll_bar.render(renderer);
}

void CodeBlockView::on_copy_clicked(
    Button* button,
    void* context
) {
    (void)button;

    CodeBlockView* code_block = (CodeBlockView*)context;
    if (code_block != 0) {
        code_block->copy_all_code();
    }
}

void CodeBlockView::on_scroll_changed(
    ScrollBar* scroll_bar,
    int value,
    void* context
) {
    (void)scroll_bar;

    CodeBlockView* code_block = (CodeBlockView*)context;
    if (code_block == 0) {
        return;
    }

    code_block->scroll_offset_x = value;
    code_block->layout_children();
}

void CodeBlockView::rebuild_formatted_code() {
    TextFormat code_format(
        false,
        false,
        false,
        11,
        TextFormat::code_block
    );

    formatted_code.set_plain_text(source_code.c_str(), code_format);
    code_label.set_formatted_text(formatted_code);
}

void CodeBlockView::update_language_label() {
    std::string display = get_display_language(language);
    language_label.set_text(display.c_str());
}

void CodeBlockView::update_content_metrics(TextMetrics* text_metrics) {
    const char* text = formatted_code.get_text();
    int text_length = formatted_code.get_length();

    if (text_metrics != 0) {
        content_width = text_metrics->measure_formatted_text_width(
            text,
            text_length,
            formatted_code.get_format_data(),
            formatted_code.get_format_count()
        );
        content_height = text_metrics->measure_formatted_text_height(
            text,
            text_length,
            formatted_code.get_format_data(),
            formatted_code.get_format_count(),
            2
        );
    } else {
        int line_count = 1;
        int current_columns = 0;
        int maximum_columns = 0;

        for (int index = 0; index < text_length; ++index) {
            if (text[index] == '\n') {
                if (current_columns > maximum_columns) {
                    maximum_columns = current_columns;
                }
                current_columns = 0;
                ++line_count;
            } else if (text[index] == '\t') {
                current_columns += 4;
            } else {
                ++current_columns;
            }
        }

        if (current_columns > maximum_columns) {
            maximum_columns = current_columns;
        }

        content_width = maximum_columns * 7;
        content_height = line_count * 16 + (line_count - 1) * 2;
    }

    if (content_width < 1) {
        content_width = 1;
    }
    if (content_height < 18) {
        content_height = 18;
    }

    layout_dirty = text_metrics == 0;
}

void CodeBlockView::layout_children() {
    int inner_x = get_x() + block_padding;
    int inner_y = get_y() + block_padding;
    int inner_width = get_width() - (block_padding * 2);
    int inner_height = get_height() - (block_padding * 2);

    if (inner_width < 0) {
        inner_width = 0;
    }
    if (inner_height < 0) {
        inner_height = 0;
    }

    header_panel.set_bounds(
        inner_x,
        inner_y,
        inner_width,
        header_height
    );

    int copy_width = 52;
    int copy_margin = 4;
    copy_button.set_bounds(
        inner_x + inner_width - copy_width - copy_margin,
        inner_y + 3,
        copy_width,
        header_height - 6
    );

    int language_width = inner_width - copy_width - 20;
    if (language_width < 0) {
        language_width = 0;
    }

    language_label.set_bounds(
        inner_x + 8,
        inner_y + 2,
        language_width,
        header_height - 4
    );

    int body_y = inner_y + header_height + block_spacing;
    int body_height = inner_height - header_height - block_spacing;
    if (body_height < 0) {
        body_height = 0;
    }

    body_panel.set_bounds(
        inner_x,
        body_y,
        inner_width,
        body_height
    );

    viewport_width = inner_width - (body_padding * 2);
    if (viewport_width < 1) {
        viewport_width = 1;
    }

    bool show_scrollbar = content_width > viewport_width;
    int maximum_scroll = content_width - viewport_width;
    if (maximum_scroll < 0) {
        maximum_scroll = 0;
    }

    if (scroll_offset_x < 0) {
        scroll_offset_x = 0;
    }
    if (scroll_offset_x > maximum_scroll) {
        scroll_offset_x = maximum_scroll;
    }

    int code_width = content_width;
    if (code_width < viewport_width) {
        code_width = viewport_width;
    }

    code_label.set_bounds(
        inner_x + body_padding - scroll_offset_x,
        body_y + body_padding,
        code_width,
        content_height
    );

    horizontal_scroll_bar.set_visible(show_scrollbar);
    horizontal_scroll_bar.set_range(
        0,
        maximum_scroll,
        viewport_width
    );
    horizontal_scroll_bar.set_value(scroll_offset_x);

    if (show_scrollbar) {
        horizontal_scroll_bar.arrange(
            inner_x + body_padding,
            body_y + body_height - body_padding - scroll_bar_height,
            viewport_width,
            scroll_bar_height
        );
    } else {
        horizontal_scroll_bar.arrange(0, 0, 0, 0);
    }

    sync_native_scrollbar();
}

void CodeBlockView::sync_native_scrollbar() {
    if (native_control_host != 0) {
        native_control_host->sync_scroll_bar(&horizontal_scroll_bar);
    }
}

void CodeBlockView::copy_all_code() {
    if (active_clipboard == 0) {
        return;
    }

    MimeData data;
    data.set_text(source_code.c_str());
    active_clipboard->set_data(data);
}
