// =================================================================================
// Filename:    engine/platform/win32/Win32NativeControlHost.h
// Author:      Ebdsaleh
// Description: Declares Win32 native-control peers used by framework components.
// =================================================================================
#pragma once

#include <windows.h>
#include <vector>

#include "framework/NativeControlHost.h"

class ComboBox;
class ContextMenu;
class ScrollBar;

class Win32NativeControlHost : public NativeControlHost {
    public:
        Win32NativeControlHost();
        virtual ~Win32NativeControlHost();

        bool initialize(HWND parent_window, HINSTANCE instance_handle);
        void shutdown();

        virtual bool attach_combo_box(ComboBox* combo_box);
        virtual void detach_combo_box(ComboBox* combo_box);
        virtual void sync_combo_box(ComboBox* combo_box);

        virtual bool attach_scroll_bar(ScrollBar* scroll_bar);
        virtual void detach_scroll_bar(ScrollBar* scroll_bar);
        virtual void sync_scroll_bar(ScrollBar* scroll_bar);

        virtual int show_context_menu(
            const ContextMenu& menu,
            int client_x,
            int client_y
        );

        bool handle_command(WPARAM w_param, LPARAM l_param);
        bool handle_scroll(UINT message, WPARAM w_param, LPARAM l_param);

    private:
        struct ComboPeer {
            ComboPeer();

            ComboBox* combo_box;
            HWND window_handle;
            int control_id;
            int item_count;
        };

        struct ScrollPeer {
            ScrollPeer();

            ScrollBar* scroll_bar;
            HWND window_handle;
            int control_id;

            bool geometry_valid;
            int last_x;
            int last_y;
            int last_width;
            int last_height;

            bool visibility_valid;
            bool last_visible;

            bool scroll_info_valid;
            int last_minimum;
            int last_maximum;
            int last_page_size;
            int last_value;

            bool enabled_valid;
            bool last_enabled;
        };

        int find_combo_peer(ComboBox* combo_box) const;
        int find_combo_peer(HWND window_handle) const;
        void populate_combo_peer(ComboPeer& peer);

        int find_scroll_peer(ScrollBar* scroll_bar) const;
        int find_scroll_peer(HWND window_handle) const;

        HWND parent_window;
        HINSTANCE instance_handle;
        std::vector<ComboPeer> combo_peers;
        std::vector<ScrollPeer> scroll_peers;
        int next_control_id;
        bool is_initialized;
};
