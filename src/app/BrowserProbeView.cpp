// =================================================================================
// Filename:    app/BrowserProbeView.cpp
// Author:      Ebdsaleh
// Description: Implements the application-level Browser Probe workspace.
// =================================================================================

#include <stdio.h>
#include <string>

#include "BrowserProbeView.h"
#include "framework/Clipboard.h"
#include "framework/MimeData.h"
#include "framework/NativeControlHost.h"
#include "framework/TextMetrics.h"
#include "framework/TextWrapLayout.h"
#include "framework/UIEvent.h"
#include "framework/rendering/ComponentRenderer.h"
#include "web/platform/WebBackendCapabilities.h"
#include "web/platform/WebNavigationRequest.h"
#include "web/platform/WebPlatformHost.h"
#include "web/platform/WebSurfaceSnapshot.h"

namespace {
    const int probe_display_limit = 32 * 1024;
    const int raw_probe_display_limit = 1024;
    const int raw_probe_columns = 96;

    const char* capability_flag(bool value) {
        return value ? "yes" : "no";
    }

    bool is_mouse_event(const UIEvent& event) {
        return
            event.type == UIEvent::event_mouse_move ||
            event.type == UIEvent::event_mouse_down ||
            event.type == UIEvent::event_mouse_up ||
            event.type == UIEvent::event_mouse_wheel;
    }

    std::string normalize_probe_text(const std::string& source) {
        std::string normalized;
        normalized.reserve(source.size());

        for (int index = 0; index < (int)source.size(); ++index) {
            if (source[index] != '\r') {
                normalized += source[index];
            }
        }

        return normalized;
    }

    std::string make_probe_display_text(
        const std::string& source,
        bool raw_mode
    ) {
        int display_limit = raw_mode
            ? raw_probe_display_limit
            : probe_display_limit;

        int copy_count = (int)source.size();
        if (copy_count > display_limit) {
            copy_count = display_limit;
        }

        std::string display;
        display.reserve(
            (std::string::size_type)copy_count +
            (raw_mode ? 160U : 96U)
        );

        int column = 0;

        for (int index = 0; index < copy_count; ++index) {
            char value = source[index];

            if (value == '\r') {
                continue;
            }

            if (
                raw_mode &&
                value != '\n' &&
                column >= raw_probe_columns
            ) {
                display += '\n';
                column = 0;
            }

            display += value;

            if (value == '\n') {
                column = 0;
            } else if (value == '\t') {
                column += 4;
            } else {
                ++column;
            }
        }

        if ((int)source.size() > display_limit) {
            if (raw_mode) {
                display +=
                    "\n\n[Raw preview capped at 1 KiB for the legacy renderer. "
                    "Copy still exports the complete captured Raw section.]";
            } else {
                display +=
                    "\n\n[Browser Probe display capped at 32 KiB. Use Copy to "
                    "copy the complete captured section.]";
            }
        }

        return display;
    }

    int estimate_wrapped_height(const char* text, int width) {
        if (text == 0 || text[0] == '\0') {
            return 18;
        }

        int columns_per_line = width / 7;
        if (columns_per_line < 1) {
            columns_per_line = 1;
        }

        int line_count = 1;
        int column = 0;

        for (int index = 0; text[index] != '\0'; ++index) {
            char value = text[index];

            if (value == '\n') {
                ++line_count;
                column = 0;
                continue;
            }

            int advance = value == '\t' ? 4 : 1;
            column += advance;

            while (column > columns_per_line) {
                ++line_count;
                column -= columns_per_line;
            }
        }

        return line_count * 18;
    }
}

BrowserProbeView::BrowserProbeView()
    : web_platform_host(0),
      probe_mode(probe_summary),
      observed_surface_revision(0),
      vertical_scroll_bar(ScrollBar::vertical),
      native_control_host(0),
      active_clipboard(0),
      output_x(0),
      output_y(0),
      output_width(0),
      output_height(0),
      content_height(18),
      scroll_offset_y(0),
      scroll_bar_width(16),
      line_step_pixels(24),
      layout_dirty(true) {
    get_style().background_color = Color(248, 250, 252);
    get_style().border_color = Color(168, 194, 216);
    get_style().border_width = 1;

    title_label.set_text("Salix Browser Probe");
    title_label.get_style().foreground_color = Color(35, 76, 112);

    backend_label.get_style().foreground_color = Color(48, 76, 101);
    capability_label.get_style().foreground_color = Color(73, 105, 133);
    status_label.get_style().foreground_color = Color(73, 105, 133);
    content_label.get_style().foreground_color = Color(45, 55, 65);

    title_label.set_horizontal_alignment(Label::align_left);
    backend_label.set_horizontal_alignment(Label::align_left);
    capability_label.set_horizontal_alignment(Label::align_left);
    status_label.set_horizontal_alignment(Label::align_left);
    content_label.set_horizontal_alignment(Label::align_left);

    capability_label.set_word_wrap(true);
    status_label.set_word_wrap(true);
    content_label.set_word_wrap(true);
    content_label.set_selectable(true);

    address_input.set_multiline(false);
    address_input.set_max_length(2048);
    address_input.set_text("https://www.chatgpt.com/");

    go_button.set_text("Go");
    go_button.set_click_handler(BrowserProbeView::on_go_clicked, this);

    summary_button.set_text("Summary");
    headers_button.set_text("Headers");
    raw_button.set_text("Raw");
    extracted_button.set_text("Extracted");
    copy_button.set_text("Copy");

    summary_button.set_click_handler(BrowserProbeView::on_probe_mode_clicked, this);
    headers_button.set_click_handler(BrowserProbeView::on_probe_mode_clicked, this);
    raw_button.set_click_handler(BrowserProbeView::on_probe_mode_clicked, this);
    extracted_button.set_click_handler(BrowserProbeView::on_probe_mode_clicked, this);
    copy_button.set_click_handler(BrowserProbeView::on_copy_clicked, this);

    vertical_scroll_bar.set_line_step(line_step_pixels);
    vertical_scroll_bar.set_value_changed_handler(
        BrowserProbeView::on_scroll_changed,
        this
    );
    vertical_scroll_bar.set_visible(false);

    add_child(&title_label);
    add_child(&address_input);
    add_child(&go_button);
    add_child(&backend_label);
    add_child(&capability_label);
    add_child(&status_label);
    add_child(&summary_button);
    add_child(&headers_button);
    add_child(&raw_button);
    add_child(&extracted_button);
    add_child(&copy_button);
    add_child(&content_label);
    add_child(&vertical_scroll_bar);

    refresh_labels();
}

BrowserProbeView::~BrowserProbeView() {
    detach_native_controls();
}

void BrowserProbeView::set_web_platform_host(WebPlatformHost* host) {
    web_platform_host = host;
    observed_surface_revision = 0;
    refresh_labels();
}

WebPlatformHost* BrowserProbeView::get_web_platform_host() {
    return web_platform_host;
}

const WebPlatformHost* BrowserProbeView::get_web_platform_host() const {
    return web_platform_host;
}

const char* BrowserProbeView::get_current_output_text() const {
    return get_active_output_text().c_str();
}

bool BrowserProbeView::navigate(const char* url) {
    if (web_platform_host == 0 || url == 0 || url[0] == '\0') {
        return false;
    }

    address_input.set_text(url);
    set_probe_mode(probe_summary);

    WebNavigationRequest request(url);
    bool result = web_platform_host->navigate(request);

    unsigned long revision = web_platform_host->get_surface_revision();
    if (revision != observed_surface_revision) {
        refresh_labels();
    } else if (result) {
        // Remote navigation is queued.  Keep this as view state until the
        // backend publishes a new surface revision on completion.
        status_label.set_text("Status: Browser Probe request in progress...");
    }

    return result;
}

void BrowserProbeView::update() {
    if (web_platform_host == 0 || !web_platform_host->has_backend()) {
        return;
    }

    if (
        web_platform_host->get_surface_revision() !=
        observed_surface_revision
    ) {
        refresh_labels();
    }
}

void BrowserProbeView::attach_native_controls(
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

void BrowserProbeView::detach_native_controls() {
    if (native_control_host == 0) {
        return;
    }

    native_control_host->detach_scroll_bar(&vertical_scroll_bar);
    native_control_host = 0;
}

void BrowserProbeView::arrange(
    int x,
    int y,
    int width,
    int height,
    TextMetrics* text_metrics
) {
    set_bounds(x, y, width, height);

    const int padding = 12;
    const int title_height = 22;
    const int address_height = 28;
    const int backend_height = 20;
    const int capability_height = 36;
    const int status_height = 34;
    const int mode_height = 26;
    const int gap = 5;
    const int go_width = 54;
    const int copy_width = 58;
    const int maximum_mode_width = 86;

    int available_width = width - (padding * 2);
    if (available_width < 0) {
        available_width = 0;
    }

    int cursor_y = y + padding;

    title_label.set_bounds(
        x + padding,
        cursor_y,
        available_width,
        title_height
    );
    cursor_y += title_height + gap;

    int address_width = available_width - go_width - gap;
    if (address_width < 0) {
        address_width = 0;
    }

    address_input.set_bounds(
        x + padding,
        cursor_y,
        address_width,
        address_height
    );
    go_button.set_bounds(
        x + padding + address_width + gap,
        cursor_y,
        go_width,
        address_height
    );
    cursor_y += address_height + gap;

    backend_label.set_bounds(
        x + padding,
        cursor_y,
        available_width,
        backend_height
    );
    cursor_y += backend_height + gap;

    capability_label.set_bounds(
        x + padding,
        cursor_y,
        available_width,
        capability_height
    );
    cursor_y += capability_height + gap;

    status_label.set_bounds(
        x + padding,
        cursor_y,
        available_width,
        status_height
    );
    cursor_y += status_height + gap;

    int mode_area_width = available_width - copy_width - gap;
    if (mode_area_width < 0) {
        mode_area_width = 0;
    }

    int mode_width = (mode_area_width - (gap * 3)) / 4;
    if (mode_width > maximum_mode_width) {
        mode_width = maximum_mode_width;
    }
    if (mode_width < 0) {
        mode_width = 0;
    }

    summary_button.set_bounds(
        x + padding,
        cursor_y,
        mode_width,
        mode_height
    );
    headers_button.set_bounds(
        x + padding + mode_width + gap,
        cursor_y,
        mode_width,
        mode_height
    );
    raw_button.set_bounds(
        x + padding + ((mode_width + gap) * 2),
        cursor_y,
        mode_width,
        mode_height
    );
    extracted_button.set_bounds(
        x + padding + ((mode_width + gap) * 3),
        cursor_y,
        mode_width,
        mode_height
    );
    copy_button.set_bounds(
        x + padding + available_width - copy_width,
        cursor_y,
        copy_width,
        mode_height
    );
    cursor_y += mode_height + gap;

    output_x = x + padding;
    output_y = cursor_y;
    output_width = available_width;
    output_height = y + height - padding - cursor_y;

    if (output_width < 0) {
        output_width = 0;
    }
    if (output_height < 0) {
        output_height = 0;
    }

    update_content_metrics(text_metrics);
    layout_output();
}

bool BrowserProbeView::handle_event(const UIEvent& event) {
    if (!get_is_visible()) {
        return false;
    }

    bool mouse_event = is_mouse_event(event);
    if (mouse_event && !contains_point(event.x, event.y)) {
        return false;
    }

    active_clipboard = event.clipboard;

    if (layout_dirty && event.text_metrics != 0) {
        update_content_metrics(event.text_metrics);
        layout_output();
    }

    if (
        event.type == UIEvent::event_mouse_wheel &&
        is_output_point(event.x, event.y) &&
        event.wheel_delta != 0
    ) {
        int notches = event.wheel_delta / 120;
        if (notches == 0) {
            notches = event.wheel_delta > 0 ? 1 : -1;
        }

        scroll_pixels(-notches * line_step_pixels * 3);
        active_clipboard = 0;
        return true;
    }

    bool handled = false;

    if (vertical_scroll_bar.handle_event(event)) {
        handled = true;
    }
    if (go_button.handle_event(event)) {
        handled = true;
    }
    if (summary_button.handle_event(event)) {
        handled = true;
    }
    if (headers_button.handle_event(event)) {
        handled = true;
    }
    if (raw_button.handle_event(event)) {
        handled = true;
    }
    if (extracted_button.handle_event(event)) {
        handled = true;
    }
    if (copy_button.handle_event(event)) {
        handled = true;
    }
    if (address_input.handle_event(event)) {
        handled = true;
    }

    bool allow_content_event = !mouse_event || is_output_point(event.x, event.y);

    if (
        !allow_content_event &&
        content_label.get_is_focused() &&
        event.left_button_down &&
        (
            event.type == UIEvent::event_mouse_move ||
            event.type == UIEvent::event_mouse_up
        )
    ) {
        allow_content_event = true;
    }

    if (allow_content_event && content_label.handle_event(event)) {
        handled = true;
    }

    active_clipboard = 0;
    return handled;
}

void BrowserProbeView::render(ComponentRenderer& renderer) const {
    if (!get_is_visible()) {
        return;
    }

    renderer.render_panel(*this);

    title_label.render(renderer);
    address_input.render(renderer);
    go_button.render(renderer);
    backend_label.render(renderer);
    capability_label.render(renderer);
    status_label.render(renderer);
    summary_button.render(renderer);
    headers_button.render(renderer);
    raw_button.render(renderer);
    extracted_button.render(renderer);
    copy_button.render(renderer);

    int viewport_width = content_label.get_width();
    if (viewport_width < 0) {
        viewport_width = 0;
    }

    renderer.push_clip_rect(
        output_x,
        output_y,
        viewport_width,
        output_height
    );
    content_label.render(renderer);
    renderer.pop_clip_rect();

    vertical_scroll_bar.render(renderer);
}

void BrowserProbeView::on_go_clicked(Button* button, void* context) {
    (void)button;

    BrowserProbeView* view = (BrowserProbeView*)context;
    if (view == 0) {
        return;
    }

    view->navigate(view->address_input.get_text());
}

void BrowserProbeView::on_probe_mode_clicked(Button* button, void* context) {
    BrowserProbeView* view = (BrowserProbeView*)context;
    if (view == 0 || button == 0) {
        return;
    }

    if (button == &view->summary_button) {
        view->set_probe_mode(probe_summary);
    } else if (button == &view->headers_button) {
        view->set_probe_mode(probe_headers);
    } else if (button == &view->raw_button) {
        view->set_probe_mode(probe_raw);
    } else if (button == &view->extracted_button) {
        view->set_probe_mode(probe_extracted);
    }
}

void BrowserProbeView::on_copy_clicked(Button* button, void* context) {
    (void)button;

    BrowserProbeView* view = (BrowserProbeView*)context;
    if (view != 0) {
        view->copy_current_output();
    }
}

void BrowserProbeView::on_scroll_changed(
    ScrollBar* scroll_bar,
    int value,
    void* context
) {
    (void)scroll_bar;

    BrowserProbeView* view = (BrowserProbeView*)context;
    if (view == 0) {
        return;
    }

    view->scroll_offset_y = value;
    view->layout_output();
}

void BrowserProbeView::set_probe_mode(ProbeMode new_mode) {
    if (probe_mode == new_mode) {
        return;
    }

    probe_mode = new_mode;
    refresh_probe_content();
}

void BrowserProbeView::refresh_labels() {
    char backend_text[256];
    char capability_text[512];

    if (web_platform_host == 0 || !web_platform_host->has_backend()) {
        observed_surface_revision = 0;
        title_label.set_text("Salix Browser Probe");
        backend_label.set_text("Backend: none");
        capability_label.set_text("Capabilities: none");
        status_label.set_text("Status: no web backend selected");

        summary_output_text =
            "Select a WebPlatformBackend to begin probing URL responses.";
        headers_output_text =
            "No HTTP response headers are available yet.";
        raw_output_text =
            "No raw response body is available yet.";
        extracted_output_text =
            "No extracted document text is available yet.";
        refresh_probe_content();
        return;
    }

    WebBackendCapabilities capabilities;
    web_platform_host->get_capabilities(capabilities);

    sprintf(
        backend_text,
        "Backend: %s | family: %s | lifecycle: %s",
        web_platform_host->get_backend_name(),
        get_web_backend_family_name(
            web_platform_host->get_backend_family()
        ),
        web_platform_host->get_is_initialized()
            ? "initialized"
            : "stopped"
    );
    backend_label.set_text(backend_text);

    sprintf(
        capability_text,
        "Capabilities: navigation %s | network %s | HTML runtime %s | JS %s | "
        "WebSocket %s | upload %s",
        capability_flag(capabilities.navigation),
        capability_flag(capabilities.network),
        capability_flag(capabilities.html),
        capability_flag(capabilities.javascript),
        capability_flag(capabilities.websocket),
        capability_flag(capabilities.file_upload)
    );
    capability_label.set_text(capability_text);

    WebSurfaceSnapshot snapshot;
    if (!web_platform_host->get_surface_snapshot(snapshot)) {
        title_label.set_text("Salix Browser Probe");
        status_label.set_text(
            "Status: backend did not provide a surface snapshot"
        );
        summary_output_text.clear();
        headers_output_text.clear();
        raw_output_text.clear();
        extracted_output_text.clear();
        set_output_text("");
        return;
    }

    observed_surface_revision =
        web_platform_host->get_surface_revision();

    if (!snapshot.title.empty()) {
        title_label.set_text(snapshot.title.c_str());
    } else {
        title_label.set_text("Salix Browser Probe");
    }

    std::string status_text("Status: ");
    status_text += snapshot.status.empty()
        ? "no status"
        : snapshot.status;
    status_label.set_text(status_text.c_str());

    // Cache only the four presentation sections.  Do not retain another full
    // WebSurfaceSnapshot in the view; the backend remains the canonical owner.
    cache_probe_outputs(snapshot);
    refresh_probe_content();
}

void BrowserProbeView::cache_probe_outputs(
    const WebSurfaceSnapshot& snapshot
) {
    summary_output_text = normalize_probe_text(
        snapshot.content.empty()
            ? "No Browser Probe result is available yet."
            : snapshot.content
    );

    headers_output_text = normalize_probe_text(
        snapshot.response_headers.empty()
            ? "No HTTP response headers are available yet."
            : snapshot.response_headers
    );

    raw_output_text = snapshot.raw_content.empty()
        ? "No raw response body is available yet."
        : snapshot.raw_content;

    extracted_output_text = normalize_probe_text(
        snapshot.extracted_content.empty()
            ? "No extracted document text is available yet."
            : snapshot.extracted_content
    );
}

const std::string& BrowserProbeView::get_active_output_text() const {
    switch (probe_mode) {
        case probe_headers:
            return headers_output_text;

        case probe_raw:
            return raw_output_text;

        case probe_extracted:
            return extracted_output_text;

        case probe_summary:
        default:
            return summary_output_text;
    }
}

void BrowserProbeView::refresh_probe_content() {
    set_output_text(get_active_output_text());
}

void BrowserProbeView::set_output_text(const std::string& text) {
    bool raw_mode = probe_mode == probe_raw;
    std::string new_display = make_probe_display_text(
        text,
        raw_mode
    );

    content_label.set_word_wrap(!raw_mode);

    if (new_display == displayed_output_text) {
        return;
    }

    displayed_output_text = new_display;
    content_label.set_text(displayed_output_text.c_str());
    scroll_offset_y = 0;
    layout_dirty = !raw_mode;

    update_content_metrics(0);
    layout_output();
}

void BrowserProbeView::copy_current_output() {
    if (active_clipboard == 0) {
        return;
    }

    const std::string& output = get_active_output_text();

    MimeData data;
    data.set_text(
        output.c_str(),
        (int)output.size()
    );
    active_clipboard->set_data(data);
}

void BrowserProbeView::scroll_pixels(int pixel_count) {
    if (pixel_count == 0) {
        return;
    }

    int maximum_scroll = content_height - output_height;
    if (maximum_scroll < 0) {
        maximum_scroll = 0;
    }

    scroll_offset_y += pixel_count;
    if (scroll_offset_y < 0) {
        scroll_offset_y = 0;
    }
    if (scroll_offset_y > maximum_scroll) {
        scroll_offset_y = maximum_scroll;
    }

    vertical_scroll_bar.set_value(scroll_offset_y);
    layout_output();
}

void BrowserProbeView::update_content_metrics(TextMetrics* text_metrics) {
    int viewport_width = output_width;
    if (viewport_width < 1) {
        viewport_width = 1;
    }

    const char* text = content_label.get_text();
    int text_length = text == 0 ? 0 : (int)displayed_output_text.size();

    if (probe_mode == probe_raw) {
        content_height = estimate_wrapped_height(
            text,
            viewport_width
        );

        if (content_height < 18) {
            content_height = 18;
        }

        layout_dirty = false;
        return;
    }

    if (text_metrics != 0) {
        content_height = TextWrapLayout::measure_height(
            text,
            text_length,
            content_label.get_format_data(),
            content_label.get_format_count(),
            viewport_width,
            2,
            text_metrics
        );
    } else {
        content_height = estimate_wrapped_height(text, viewport_width);
    }

    if (content_height > output_height && output_width > scroll_bar_width + 4) {
        viewport_width = output_width - scroll_bar_width - 4;

        if (text_metrics != 0) {
            content_height = TextWrapLayout::measure_height(
                text,
                text_length,
                content_label.get_format_data(),
                content_label.get_format_count(),
                viewport_width,
                2,
                text_metrics
            );
        } else {
            content_height = estimate_wrapped_height(text, viewport_width);
        }
    }

    if (content_height < 18) {
        content_height = 18;
    }

    layout_dirty = text_metrics == 0;
}

void BrowserProbeView::layout_output() {
    bool show_scrollbar = content_height > output_height && output_height > 0;

    int viewport_width = output_width;
    if (show_scrollbar) {
        viewport_width -= scroll_bar_width + 4;
    }
    if (viewport_width < 0) {
        viewport_width = 0;
    }

    int maximum_scroll = content_height - output_height;
    if (maximum_scroll < 0) {
        maximum_scroll = 0;
    }

    if (scroll_offset_y < 0) {
        scroll_offset_y = 0;
    }
    if (scroll_offset_y > maximum_scroll) {
        scroll_offset_y = maximum_scroll;
    }

    content_label.set_bounds(
        output_x,
        output_y - scroll_offset_y,
        viewport_width,
        content_height
    );

    vertical_scroll_bar.set_visible(show_scrollbar);
    vertical_scroll_bar.set_range(
        0,
        maximum_scroll,
        output_height > 0 ? output_height : 1
    );
    vertical_scroll_bar.set_value(scroll_offset_y);

    if (show_scrollbar) {
        vertical_scroll_bar.arrange(
            output_x + viewport_width + 4,
            output_y,
            scroll_bar_width,
            output_height
        );
    } else {
        vertical_scroll_bar.arrange(0, 0, 0, 0);
    }

    sync_native_scrollbar();
}

void BrowserProbeView::sync_native_scrollbar() {
    if (native_control_host != 0) {
        native_control_host->sync_scroll_bar(&vertical_scroll_bar);
    }
}

bool BrowserProbeView::is_output_point(int x, int y) const {
    return
        x >= output_x &&
        x < output_x + content_label.get_width() &&
        y >= output_y &&
        y < output_y + output_height;
}
