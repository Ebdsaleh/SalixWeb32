// =================================================================================
// Filename:    framework/TabView.h
// Author:      Ebdsaleh
// Description: Declares a backend-neutral tabbed content container.
// =================================================================================
#pragma once

#include <string>
#include <vector>

#include "Panel.h"
#include "Button.h"

class UIEvent;

class TabView : public Panel {
    public:
        typedef void (*TabChangedHandler)(
            TabView* tab_view,
            int old_index,
            int new_index,
            void* context
        );

        TabView();
        virtual ~TabView();

        int add_tab(const char* title, Component* content);
        bool remove_tab(int index);

        int get_tab_count() const;
        const char* get_tab_title(int index) const;
        Component* get_tab_content(int index);
        const Component* get_tab_content(int index) const;

        int get_active_index() const;
        bool set_active_index(int index);

        void set_tab_changed_handler(
            TabChangedHandler handler,
            void* context
        );

        void arrange(int x, int y, int width, int height);

        int get_header_x() const;
        int get_header_y() const;
        int get_header_width() const;
        int get_header_height() const;

        int get_content_x() const;
        int get_content_y() const;
        int get_content_width() const;
        int get_content_height() const;

        void set_native_peer_active(bool active);
        bool get_native_peer_active() const;
        int get_revision() const;
        void notify_native_selection_changed(int index);

        virtual bool handle_event(const UIEvent& event);

    private:
        struct TabEntry {
            TabEntry()
                : button(0),
                  content(0) {
            }

            std::string title;
            Button* button;
            Component* content;
        };

        static void on_tab_button_clicked(
            Button* button,
            void* context
        );

        void activate_tab_from_button(Button* button);
        void update_tab_state();
        void layout_fallback_headers();

        std::vector<TabEntry> tabs;
        Panel content_panel;
        Panel header_panel;
        int active_index;
        int header_height;
        int preferred_tab_width;
        int minimum_tab_width;
        int tab_spacing;
        bool native_peer_active;
        int revision;
        TabChangedHandler tab_changed_handler;
        void* tab_changed_context;
};
