// =================================================================================
// Filename:    framework/WebView.cpp
// Author:      Ebdsaleh
// Description: Implements the backend-neutral semantic WebView component.
// =================================================================================

#include <stdio.h>
#include <string>

#include "WebView.h"
#include "UIEvent.h"
#include "web/platform/WebBackendCapabilities.h"
#include "web/platform/WebInputEvent.h"
#include "web/platform/WebNavigationRequest.h"
#include "web/platform/WebPlatformHost.h"
#include "web/platform/WebSurfaceSnapshot.h"

namespace {
    WebInputEvent::Type map_web_input_type(UIEvent::Type type) {
        switch (type) {
            case UIEvent::event_mouse_move:
                return WebInputEvent::input_mouse_move;
            case UIEvent::event_mouse_down:
                return WebInputEvent::input_mouse_down;
            case UIEvent::event_mouse_up:
                return WebInputEvent::input_mouse_up;
            case UIEvent::event_mouse_wheel:
                return WebInputEvent::input_mouse_wheel;
            case UIEvent::event_key_down:
                return WebInputEvent::input_key_down;
            case UIEvent::event_key_up:
                return WebInputEvent::input_key_up;
            case UIEvent::event_character:
                return WebInputEvent::input_character;
            default:
                return WebInputEvent::input_none;
        }
    }

    const char* capability_flag(bool value) {
        return value ? "yes" : "no";
    }
}

WebView::WebView()
    : web_platform_host(0) {
    get_style().background_color = Color(248, 250, 252);
    get_style().border_color = Color(168, 194, 216);
    get_style().border_width = 1;

    title_label.set_text("WebView");
    title_label.get_style().foreground_color = Color(35, 76, 112);

    backend_label.get_style().foreground_color = Color(48, 76, 101);
    address_label.get_style().foreground_color = Color(48, 76, 101);
    capability_label.get_style().foreground_color = Color(73, 105, 133);
    status_label.get_style().foreground_color = Color(73, 105, 133);
    content_label.get_style().foreground_color = Color(45, 55, 65);

    title_label.set_horizontal_alignment(Label::align_left);
    backend_label.set_horizontal_alignment(Label::align_left);
    address_label.set_horizontal_alignment(Label::align_left);
    capability_label.set_horizontal_alignment(Label::align_left);
    status_label.set_horizontal_alignment(Label::align_left);
    content_label.set_horizontal_alignment(Label::align_left);

    capability_label.set_word_wrap(true);
    status_label.set_word_wrap(true);
    content_label.set_word_wrap(true);

    add_child(&title_label);
    add_child(&backend_label);
    add_child(&address_label);
    add_child(&capability_label);
    add_child(&status_label);
    add_child(&content_label);

    refresh_labels();
}

void WebView::set_web_platform_host(WebPlatformHost* host) {
    web_platform_host = host;
    refresh_labels();
}

WebPlatformHost* WebView::get_web_platform_host() {
    return web_platform_host;
}

const WebPlatformHost* WebView::get_web_platform_host() const {
    return web_platform_host;
}

bool WebView::navigate(const char* url) {
    if (web_platform_host == 0) {
        return false;
    }

    WebNavigationRequest request(url);
    return web_platform_host->navigate(request);
}

void WebView::update() {
    refresh_labels();
}

void WebView::arrange(
    int x,
    int y,
    int width,
    int height
) {
    set_bounds(x, y, width, height);

    const int padding = 14;
    const int title_height = 24;
    const int line_height = 22;
    const int capability_height = 42;
    const int status_height = 42;
    const int gap = 4;

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

    backend_label.set_bounds(
        x + padding,
        cursor_y,
        content_width,
        line_height
    );
    cursor_y += line_height + gap;

    address_label.set_bounds(
        x + padding,
        cursor_y,
        content_width,
        line_height
    );
    cursor_y += line_height + gap;

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

bool WebView::handle_event(const UIEvent& event) {
    if (!get_is_visible() || web_platform_host == 0) {
        return false;
    }

    WebInputEvent::Type input_type = map_web_input_type(event.type);
    if (input_type == WebInputEvent::input_none) {
        return Panel::handle_event(event);
    }

    bool is_mouse_event =
        event.type == UIEvent::event_mouse_move ||
        event.type == UIEvent::event_mouse_down ||
        event.type == UIEvent::event_mouse_up ||
        event.type == UIEvent::event_mouse_wheel;

    if (is_mouse_event && !contains_point(event.x, event.y)) {
        return false;
    }

    WebInputEvent web_event(input_type);
    web_event.x = event.x - get_x();
    web_event.y = event.y - get_y();
    web_event.key_code = (int)event.key_code;
    web_event.character_code = event.character_code;
    web_event.wheel_delta = event.wheel_delta;
    web_event.shift_down = event.shift_down;
    web_event.control_down = event.control_down;
    web_event.alt_down = event.alt_down;
    web_event.left_button_down = event.left_button_down;

    return web_platform_host->handle_input(web_event);
}

void WebView::refresh_labels() {
    char backend_text[256];
    char capability_text[512];

    if (web_platform_host == 0 || !web_platform_host->has_backend()) {
        backend_label.set_text("Backend: none");
        address_label.set_text("Address: about:blank");
        capability_label.set_text("Capabilities: none");
        status_label.set_text("Status: no web backend selected");
        content_label.set_text(
            "A WebPlatformBackend can be supplied without changing application code."
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
        "Capabilities: navigation %s | surface %s | pointer %s | keyboard %s | "
        "network %s | HTML %s | JS %s | WebSocket %s | upload %s",
        capability_flag(capabilities.navigation),
        capability_flag(capabilities.surface_snapshot),
        capability_flag(capabilities.pointer_input),
        capability_flag(capabilities.keyboard_input),
        capability_flag(capabilities.network),
        capability_flag(capabilities.html),
        capability_flag(capabilities.javascript),
        capability_flag(capabilities.websocket),
        capability_flag(capabilities.file_upload)
    );
    capability_label.set_text(capability_text);

    WebSurfaceSnapshot snapshot;
    if (!web_platform_host->get_surface_snapshot(snapshot)) {
        address_label.set_text("Address: unavailable");
        status_label.set_text("Status: backend did not provide a surface snapshot");
        content_label.set_text("");
        return;
    }

    std::string address_text("Address: ");
    if (snapshot.address.empty()) {
        address_text += "about:blank";
    } else {
        address_text += snapshot.address;
    }
    address_label.set_text(address_text.c_str());

    std::string status_text("Status: ");
    if (snapshot.status.empty()) {
        status_text += "no status";
    } else {
        status_text += snapshot.status;
    }
    status_label.set_text(status_text.c_str());

    if (!snapshot.title.empty()) {
        title_label.set_text(snapshot.title.c_str());
    } else {
        title_label.set_text("WebView");
    }

    content_label.set_text(snapshot.content.c_str());
}
