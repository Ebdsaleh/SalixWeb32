// =================================================================================
// Filename:    framework/NativeControlHost.h
// Author:      Ebdsaleh
// Description: Declares the backend-neutral host contract for native UI peers.
// =================================================================================
#pragma once

class ComboBox;
class ContextMenu;
class ScrollBar;
class TabView;

class NativeControlHost {
    public:
        virtual ~NativeControlHost() {}

        virtual bool attach_combo_box(ComboBox* combo_box) = 0;
        virtual void detach_combo_box(ComboBox* combo_box) = 0;
        virtual void sync_combo_box(ComboBox* combo_box) = 0;

        virtual bool attach_scroll_bar(ScrollBar* scroll_bar) = 0;
        virtual void detach_scroll_bar(ScrollBar* scroll_bar) = 0;
        virtual void sync_scroll_bar(ScrollBar* scroll_bar) = 0;

        virtual bool attach_tab_view(TabView* tab_view) = 0;
        virtual void detach_tab_view(TabView* tab_view) = 0;
        virtual void sync_tab_view(TabView* tab_view) = 0;

        virtual int show_context_menu(
            const ContextMenu& menu,
            int client_x,
            int client_y
        ) = 0;
};
