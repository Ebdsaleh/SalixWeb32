// =================================================================================
// Filename:    app/BrowserProbeView.cpp
// Author:      Ebdsaleh
// Description: Implements the application-level Browser Probe workspace.
// =================================================================================

#include <stdio.h>
#include <string>

#include "BrowserProbeView.h"
#include "web/platform/WebBackendCapabilities.h"
#include "web/platform/WebNavigationRequest.h"
#include "web/platform/WebPlatformHost.h"
#include "web/platform/WebSurfaceSnapshot.h"

namespace {
    const int probe_display_limit = 32 * 1024;

    const char* capability_flag(bool value) {
        return value ? "yes" : "no";
    }

    std::string make_probe_display_text(
        const std::string& source,
        const char* empty_text
    ) {
        if (source.empty()) {
            return empty_text == 0 ? "" : empty_text;
        }

        if ((int)source.size() <= probe_display_limit) {
            return source;
        }

        std::string display = source.substr(0, probe_display_limit);
        display +=
            "\n\n[Browser Probe display capped at 32 KiB. The backend snapshot "
            "retains the complete captured probe section.]";
        return display;
    }
}

BrowserProbeView::BrowserProbeView()
    : web_platform_host(0),
      probe_mode(probe_summary) {
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

    summary_button.set_click_handler(BrowserProbeView::on_probe_mode_clicked, this);
    headers_button.set_click_handler(BrowserProbeView::on_probe_mode_clicked, this);
    raw_button.set_click_handler(BrowserProbeView::on_probe_mode_clicked, this);
    extracted_button.set_click_handler(BrowserProbeView::on_probe_mode_clicked, this);

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
    add_child(&content_label);

    refresh_labels();
}

void BrowserProbeView::set_web_platform_host(WebPlatformHost* host) {
    web_platform_host = host;
    refresh_labels();
}

WebPlatformHost* BrowserProbeView::get_web_platform_host() {
    return web_platform_host;
}

const WebPlatformHost* BrowserProbeView::get_web_platform_host() const {
    return web_platform_host;
}

bool BrowserProbeView::navigate(const char* url) {
    if (web_platform_host == 0 || url == 0 || url[0] == '\0') {
        return false;
    }

    address_input.set_text(url);
    set_probe_mode(probe_summary);

    WebNavigationRequest request(url);
    bool result = web_platform_host->navigate(request);
    refresh_labels();
    return result;
}

void BrowserProbeView::update() {
    refresh_labels();
}

void BrowserProbeView::arrange(
    int x,
    int y,
    int width,
    int height
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
    const int mode_width = 86;

    int content_width = width - (padding * 2);
    if (content_width < 0) {
        content_width = 0;
    }

    int cursor_y = y + padding;

    title_label.set_bounds(
        x + padding,
        cursor_y,
        content_width,
        title_height
    );
    cursor_y += title_height + gap;

    int address_width = content_width - go_width - gap;
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
        content_width,
        backend_height
    );
    cursor_y += backend_height + gap;

    capability_label.set_bounds(
        x + padding,
        cursor_y,
        content_width,
        capability_height
    );
    cursor_y += capability_height + gap;

    status_label.set_bounds(
        x + padding,
        cursor_y,
        content_width,
        status_height
    );
    cursor_y += status_height + gap;

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
    cursor_y += mode_height + gap;

    int remaining_height = y + height - padding - cursor_y;
    if (remaining_height < 0) {
        remaining_height = 0;
    }

    content_label.set_bounds(
        x + padding,
        cursor_y,
        content_width,
        remaining_height
    );
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

void BrowserProbeView::set_probe_mode(ProbeMode new_mode) {
    probe_mode = new_mode;
    refresh_labels();
}

void BrowserProbeView::refresh_labels() {
    char backend_text[256];
    char capability_text[512];

    if (web_platform_host == 0 || !web_platform_host->has_backend()) {
        title_label.set_text("Salix Browser Probe");
        backend_label.set_text("Backend: none");
        capability_label.set_text("Capabilities: none");
        status_label.set_text("Status: no web backend selected");
        content_label.set_text(
            "Select a WebPlatformBackend to begin probing URL responses."
        );
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
        status_label.set_text("Status: backend did not provide a surface snapshot");
        content_label.set_text("");
        return;
    }

    if (!snapshot.title.empty()) {
        title_label.set_text(snapshot.title.c_str());
    } else {
        title_label.set_text("Salix Browser Probe");
    }

    std::string status_text("Status: ");
    status_text += snapshot.status.empty() ? "no status" : snapshot.status;
    status_label.set_text(status_text.c_str());

    refresh_probe_content(snapshot);
}

void BrowserProbeView::refresh_probe_content(
    const WebSurfaceSnapshot& snapshot
) {
    std::string display;

    switch (probe_mode) {
        case probe_headers:
            display = make_probe_display_text(
                snapshot.response_headers,
                "No HTTP response headers are available yet."
            );
            break;

        case probe_raw:
            display = make_probe_display_text(
                snapshot.raw_content,
                "No raw response body is available yet."
            );
            break;

        case probe_extracted:
            display = make_probe_display_text(
                snapshot.extracted_content,
                "No extracted document text is available yet."
            );
            break;

        case probe_summary:
        default:
            display = make_probe_display_text(
                snapshot.content,
                "No Browser Probe result is available yet."
            );
            break;
    }

    content_label.set_text(display.c_str());
}
