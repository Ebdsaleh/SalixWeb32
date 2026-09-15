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

class Win32NativeControlHost : public NativeControlHost {
    public:
        Win32NativeControlHost();
        virtual ~Win32NativeControlHost();

        bool initialize(HWND parent_window, HINSTANCE instance_handle);
        void shutdown();

        virtual bool attach_combo_box(ComboBox* combo_box);
        virtual void detach_combo_box(ComboBox* combo_box);
        virtual void sync_combo_box(ComboBox* combo_box);

        bool handle_command(WPARAM w_param, LPARAM l_param);

    private:
        struct ComboPeer {
            ComboPeer();

            ComboBox* combo_box;
            HWND window_handle;
            int control_id;
            int item_count;
        };

        int find_combo_peer(ComboBox* combo_box) const;
        int find_combo_peer(HWND window_handle) const;
        void populate_combo_peer(ComboPeer& peer);

        HWND parent_window;
        HINSTANCE instance_handle;
        std::vector<ComboPeer> combo_peers;
        int next_control_id;
        bool is_initialized;
};
