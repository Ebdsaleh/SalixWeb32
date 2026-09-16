// =================================================================================
// Filename:    framework/TabView.cpp
// Author:      Ebdsaleh
// Description: Implements a backend-neutral tabbed content container.
// =================================================================================

#include "TabView.h"
#include "UIEvent.h"

TabView::TabView()
    : active_index(-1),
      header_height(28),
      preferred_tab_width(132),
      minimum_tab_width(72),
      tab_spacing(2),
      native_peer_active(false),
      revision(0),
      tab_changed_handler(0),
      tab_changed_context(0) {

    get_style().background_color = Color(232, 241, 249);
    get_style().border_width = 0;

    header_panel.get_style().background_color = Color(214, 235, 249);
    header_panel.get_style().border_color = Color(121, 171, 211);
    header_panel.get_style().border_width = 1;

    content_panel.get_style().background_color = Color(255, 255, 255);
    content_panel.get_style().border_color = Color(168, 194, 216);
    content_panel.get_style().border_width = 1;

    add_child(&content_panel);
    add_child(&header_panel);
}

TabView::~TabView() {
    for (int index = 0; index < (int)tabs.size(); ++index) {
        if (tabs[index].button != 0) {
            header_panel.remove_child(tabs[index].button);
            delete tabs[index].button;
            tabs[index].button = 0;
        }

        if (tabs[index].content != 0) {
            content_panel.remove_child(tabs[index].content);
            tabs[index].content = 0;
        }
    }

    tabs.clear();
}

int TabView::add_tab(const char* title, Component* content) {
    if (content == 0) {
        return -1;
    }

    Button* button = new Button();
    if (button == 0) {
        return -1;
    }

    TabEntry entry;
    entry.title = title == 0 ? "" : title;
    entry.button = button;
    entry.content = content;

    button->set_text(entry.title.c_str());
    button->set_click_handler(TabView::on_tab_button_clicked, this);

    if (!header_panel.add_child(button)) {
        delete button;
        return -1;
    }

    if (!content_panel.add_child(content)) {
        header_panel.remove_child(button);
        delete button;
        return -1;
    }

    tabs.push_back(entry);
    ++revision;

    if (active_index < 0) {
        active_index = 0;
    }

    update_tab_state();
    layout_fallback_headers();
    return (int)tabs.size() - 1;
}

bool TabView::remove_tab(int index) {
    if (index < 0 || index >= (int)tabs.size()) {
        return false;
    }

    int old_active_index = active_index;
    TabEntry entry = tabs[index];

    if (entry.button != 0) {
        header_panel.remove_child(entry.button);
        delete entry.button;
    }

    if (entry.content != 0) {
        content_panel.remove_child(entry.content);
        entry.content->set_visible(false);
    }

    tabs.erase(tabs.begin() + index);
    ++revision;

    if (tabs.empty()) {
        active_index = -1;
    } else if (active_index > index) {
        --active_index;
    } else if (active_index >= (int)tabs.size()) {
        active_index = (int)tabs.size() - 1;
    }

    update_tab_state();
    layout_fallback_headers();

    if (
        tab_changed_handler != 0 &&
        old_active_index != active_index
    ) {
        tab_changed_handler(
            this,
            old_active_index,
            active_index,
            tab_changed_context
        );
    }

    return true;
}

int TabView::get_tab_count() const {
    return (int)tabs.size();
}

const char* TabView::get_tab_title(int index) const {
    if (index < 0 || index >= (int)tabs.size()) {
        return "";
    }

    return tabs[index].title.c_str();
}

Component* TabView::get_tab_content(int index) {
    if (index < 0 || index >= (int)tabs.size()) {
        return 0;
    }

    return tabs[index].content;
}

const Component* TabView::get_tab_content(int index) const {
    if (index < 0 || index >= (int)tabs.size()) {
        return 0;
    }

    return tabs[index].content;
}

int TabView::get_active_index() const {
    return active_index;
}

bool TabView::set_active_index(int index) {
    if (index < 0 || index >= (int)tabs.size()) {
        return false;
    }

    if (active_index == index) {
        update_tab_state();
        return true;
    }

    int old_index = active_index;
    active_index = index;
    update_tab_state();

    if (tab_changed_handler != 0) {
        tab_changed_handler(
            this,
            old_index,
            active_index,
            tab_changed_context
        );
    }

    return true;
}

void TabView::set_tab_changed_handler(
    TabChangedHandler handler,
    void* context
) {
    tab_changed_handler = handler;
    tab_changed_context = context;
}

void TabView::arrange(int x, int y, int width, int height) {
    if (width < 0) {
        width = 0;
    }

    if (height < 0) {
        height = 0;
    }

    set_bounds(x, y, width, height);

    int actual_header_height = header_height;
    if (actual_header_height > height) {
        actual_header_height = height;
    }

    header_panel.set_bounds(
        x,
        y,
        width,
        actual_header_height
    );

    int content_height = height - actual_header_height;
    if (content_height < 0) {
        content_height = 0;
    }

    content_panel.set_bounds(
        x,
        y + actual_header_height,
        width,
        content_height
    );

    for (int index = 0; index < (int)tabs.size(); ++index) {
        if (tabs[index].content != 0) {
            tabs[index].content->set_bounds(
                content_panel.get_x(),
                content_panel.get_y(),
                content_panel.get_width(),
                content_panel.get_height()
            );
        }
    }

    layout_fallback_headers();
}

int TabView::get_header_x() const {
    return header_panel.get_x();
}

int TabView::get_header_y() const {
    return header_panel.get_y();
}

int TabView::get_header_width() const {
    return header_panel.get_width();
}

int TabView::get_header_height() const {
    return header_panel.get_height();
}

int TabView::get_content_x() const {
    return content_panel.get_x();
}

int TabView::get_content_y() const {
    return content_panel.get_y();
}

int TabView::get_content_width() const {
    return content_panel.get_width();
}

int TabView::get_content_height() const {
    return content_panel.get_height();
}

void TabView::set_native_peer_active(bool active) {
    native_peer_active = active;
    header_panel.set_visible(!native_peer_active);
}

bool TabView::get_native_peer_active() const {
    return native_peer_active;
}

int TabView::get_revision() const {
    return revision;
}

void TabView::notify_native_selection_changed(int index) {
    set_active_index(index);
}

bool TabView::handle_event(const UIEvent& event) {
    if (!get_is_visible()) {
        return false;
    }

    if (
        event.type == UIEvent::event_key_down &&
        event.control_down &&
        event.key_code == UIEvent::key_tab &&
        tabs.size() > 1
    ) {
        int new_index = active_index;

        if (event.shift_down) {
            --new_index;
            if (new_index < 0) {
                new_index = (int)tabs.size() - 1;
            }
        } else {
            ++new_index;
            if (new_index >= (int)tabs.size()) {
                new_index = 0;
            }
        }

        set_active_index(new_index);
        return true;
    }

    return Panel::handle_event(event);
}

void TabView::on_tab_button_clicked(
    Button* button,
    void* context
) {
    TabView* tab_view = (TabView*)context;
    if (tab_view != 0) {
        tab_view->activate_tab_from_button(button);
    }
}

void TabView::activate_tab_from_button(Button* button) {
    for (int index = 0; index < (int)tabs.size(); ++index) {
        if (tabs[index].button == button) {
            set_active_index(index);
            return;
        }
    }
}

void TabView::update_tab_state() {
    for (int index = 0; index < (int)tabs.size(); ++index) {
        bool active = index == active_index;

        if (tabs[index].content != 0) {
            tabs[index].content->set_visible(active);
        }

        if (tabs[index].button != 0) {
            tabs[index].button->get_style().background_color = active
                ? Color(255, 255, 255)
                : Color(220, 235, 246);
            tabs[index].button->get_style().border_color = active
                ? Color(100, 152, 194)
                : Color(154, 188, 214);
            tabs[index].button->get_style().border_width = 1;
        }
    }
}

void TabView::layout_fallback_headers() {
    int count = (int)tabs.size();
    if (count <= 0) {
        return;
    }

    int available_width = header_panel.get_width() - 4;
    if (available_width < 0) {
        available_width = 0;
    }

    int tab_width = preferred_tab_width;
    int required_width =
        (tab_width * count) + (tab_spacing * (count - 1));

    if (required_width > available_width && count > 0) {
        tab_width = (
            available_width - (tab_spacing * (count - 1))
        ) / count;

        if (tab_width < minimum_tab_width) {
            tab_width = minimum_tab_width;
        }
    }

    int current_x = header_panel.get_x() + 2;
    int button_height = header_panel.get_height() - 4;
    if (button_height < 1) {
        button_height = 1;
    }

    for (int index = 0; index < count; ++index) {
        if (tabs[index].button != 0) {
            tabs[index].button->set_bounds(
                current_x,
                header_panel.get_y() + 2,
                tab_width,
                button_height
            );
        }

        current_x += tab_width + tab_spacing;
    }
}
