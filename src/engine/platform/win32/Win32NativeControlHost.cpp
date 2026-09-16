// =================================================================================
// Filename:    engine/platform/win32/Win32NativeControlHost.cpp
// Author:      Ebdsaleh
// Description: Implements native Win32 peers for framework controls.
// =================================================================================

#include "Win32NativeControlHost.h"
#include "framework/ComboBox.h"
#include "framework/ScrollBar.h"

Win32NativeControlHost::ComboPeer::ComboPeer()
    : combo_box(0),
      window_handle(NULL),
      control_id(0),
      item_count(0) {
}

Win32NativeControlHost::ScrollPeer::ScrollPeer()
    : scroll_bar(0),
      window_handle(NULL),
      control_id(0) {
}

Win32NativeControlHost::Win32NativeControlHost()
    : parent_window(NULL),
      instance_handle(NULL),
      next_control_id(4000),
      is_initialized(false) {
}

Win32NativeControlHost::~Win32NativeControlHost() {
    shutdown();
}

bool Win32NativeControlHost::initialize(
    HWND new_parent_window,
    HINSTANCE new_instance_handle
) {
    if (
        is_initialized ||
        new_parent_window == NULL ||
        new_instance_handle == NULL
    ) {
        return false;
    }

    parent_window = new_parent_window;
    instance_handle = new_instance_handle;
    next_control_id = 4000;
    is_initialized = true;
    return true;
}

void Win32NativeControlHost::shutdown() {
    if (!is_initialized) {
        return;
    }

    for (int index = 0; index < (int)combo_peers.size(); ++index) {
        ComboPeer& peer = combo_peers[index];

        if (peer.combo_box != 0) {
            peer.combo_box->set_native_peer_active(false);
        }

        if (peer.window_handle != NULL && IsWindow(peer.window_handle)) {
            DestroyWindow(peer.window_handle);
        }

        peer.combo_box = 0;
        peer.window_handle = NULL;
    }

    for (int index = 0; index < (int)scroll_peers.size(); ++index) {
        ScrollPeer& peer = scroll_peers[index];

        if (peer.scroll_bar != 0) {
            peer.scroll_bar->set_native_peer_active(false);
        }

        if (peer.window_handle != NULL && IsWindow(peer.window_handle)) {
            DestroyWindow(peer.window_handle);
        }

        peer.scroll_bar = 0;
        peer.window_handle = NULL;
    }

    combo_peers.clear();
    scroll_peers.clear();
    parent_window = NULL;
    instance_handle = NULL;
    next_control_id = 4000;
    is_initialized = false;
}

bool Win32NativeControlHost::attach_combo_box(ComboBox* combo_box) {
    if (!is_initialized || combo_box == 0) {
        return false;
    }

    if (find_combo_peer(combo_box) >= 0) {
        return true;
    }

    ComboPeer peer;
    peer.combo_box = combo_box;
    peer.control_id = next_control_id++;

    peer.window_handle = CreateWindowExA(
        0,
        "COMBOBOX",
        "",
        WS_CHILD |
            WS_VSCROLL |
            WS_TABSTOP |
            CBS_DROPDOWNLIST |
            CBS_HASSTRINGS,
        0,
        0,
        1,
        120,
        parent_window,
        (HMENU)(INT_PTR)peer.control_id,
        instance_handle,
        NULL
    );

    if (peer.window_handle == NULL) {
        return false;
    }

    SendMessageA(
        peer.window_handle,
        WM_SETFONT,
        (WPARAM)GetStockObject(DEFAULT_GUI_FONT),
        MAKELPARAM(TRUE, 0)
    );

    combo_peers.push_back(peer);
    combo_box->set_native_peer_active(true);

    int peer_index = find_combo_peer(combo_box);
    if (peer_index >= 0) {
        populate_combo_peer(combo_peers[peer_index]);
        sync_combo_box(combo_box);
    }

    return true;
}

void Win32NativeControlHost::detach_combo_box(ComboBox* combo_box) {
    int peer_index = find_combo_peer(combo_box);
    if (peer_index < 0) {
        return;
    }

    ComboPeer peer = combo_peers[peer_index];

    if (peer.combo_box != 0) {
        peer.combo_box->set_native_peer_active(false);
    }

    if (peer.window_handle != NULL && IsWindow(peer.window_handle)) {
        DestroyWindow(peer.window_handle);
    }

    combo_peers.erase(combo_peers.begin() + peer_index);
}

void Win32NativeControlHost::sync_combo_box(ComboBox* combo_box) {
    int peer_index = find_combo_peer(combo_box);
    if (peer_index < 0 || combo_box == 0) {
        return;
    }

    ComboPeer& peer = combo_peers[peer_index];

    if (peer.window_handle == NULL || !IsWindow(peer.window_handle)) {
        return;
    }

    if (peer.item_count != combo_box->get_item_count()) {
        populate_combo_peer(peer);
    }

    int control_height = combo_box->get_height();
    int dropdown_height = combo_box->get_item_count() * 20 + 8;

    if (dropdown_height < 80) {
        dropdown_height = 80;
    }

    if (control_height < 1) {
        control_height = 1;
    }

    int total_height = control_height + dropdown_height;

    MoveWindow(
        peer.window_handle,
        combo_box->get_x(),
        combo_box->get_y(),
        combo_box->get_width(),
        total_height,
        TRUE
    );

    EnableWindow(
        peer.window_handle,
        combo_box->get_is_enabled() ? TRUE : FALSE
    );

    ShowWindow(
        peer.window_handle,
        combo_box->get_is_visible() ? SW_SHOW : SW_HIDE
    );

    int selected_index = combo_box->get_selected_index();
    int native_selected_index = (int)SendMessageA(
        peer.window_handle,
        CB_GETCURSEL,
        0,
        0
    );

    if (selected_index != native_selected_index) {
        SendMessageA(
            peer.window_handle,
            CB_SETCURSEL,
            (WPARAM)selected_index,
            0
        );
    }
}

bool Win32NativeControlHost::attach_scroll_bar(ScrollBar* scroll_bar) {
    if (!is_initialized || scroll_bar == 0) {
        return false;
    }

    if (find_scroll_peer(scroll_bar) >= 0) {
        return true;
    }

    ScrollPeer peer;
    peer.scroll_bar = scroll_bar;
    peer.control_id = next_control_id++;

    DWORD style = WS_CHILD;
    if (scroll_bar->get_orientation() == ScrollBar::vertical) {
        style |= SBS_VERT;
    } else {
        style |= SBS_HORZ;
    }

    peer.window_handle = CreateWindowExA(
        0,
        "SCROLLBAR",
        "",
        style,
        0,
        0,
        1,
        1,
        parent_window,
        (HMENU)(INT_PTR)peer.control_id,
        instance_handle,
        NULL
    );

    if (peer.window_handle == NULL) {
        return false;
    }

    scroll_peers.push_back(peer);
    scroll_bar->set_native_peer_active(true);
    sync_scroll_bar(scroll_bar);
    return true;
}

void Win32NativeControlHost::detach_scroll_bar(ScrollBar* scroll_bar) {
    int peer_index = find_scroll_peer(scroll_bar);
    if (peer_index < 0) {
        return;
    }

    ScrollPeer peer = scroll_peers[peer_index];

    if (peer.scroll_bar != 0) {
        peer.scroll_bar->set_native_peer_active(false);
    }

    if (peer.window_handle != NULL && IsWindow(peer.window_handle)) {
        DestroyWindow(peer.window_handle);
    }

    scroll_peers.erase(scroll_peers.begin() + peer_index);
}

void Win32NativeControlHost::sync_scroll_bar(ScrollBar* scroll_bar) {
    int peer_index = find_scroll_peer(scroll_bar);
    if (peer_index < 0 || scroll_bar == 0) {
        return;
    }

    ScrollPeer& peer = scroll_peers[peer_index];
    if (peer.window_handle == NULL || !IsWindow(peer.window_handle)) {
        return;
    }

    MoveWindow(
        peer.window_handle,
        scroll_bar->get_x(),
        scroll_bar->get_y(),
        scroll_bar->get_width(),
        scroll_bar->get_height(),
        TRUE
    );

    ShowWindow(
        peer.window_handle,
        scroll_bar->get_is_visible() ? SW_SHOW : SW_HIDE
    );

    SCROLLINFO scroll_info;
    ZeroMemory(&scroll_info, sizeof(scroll_info));
    scroll_info.cbSize = sizeof(scroll_info);
    scroll_info.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
    scroll_info.nMin = scroll_bar->get_minimum();
    scroll_info.nMax = scroll_bar->get_maximum() +
        scroll_bar->get_page_size() - 1;
    scroll_info.nPage = (UINT)scroll_bar->get_page_size();
    scroll_info.nPos = scroll_bar->get_value();

    SetScrollInfo(
        peer.window_handle,
        SB_CTL,
        &scroll_info,
        TRUE
    );

    EnableWindow(
        peer.window_handle,
        scroll_bar->get_maximum() > scroll_bar->get_minimum()
            ? TRUE
            : FALSE
    );
}

bool Win32NativeControlHost::handle_command(
    WPARAM w_param,
    LPARAM l_param
) {
    HWND source_window = (HWND)l_param;
    int peer_index = find_combo_peer(source_window);

    if (peer_index < 0) {
        return false;
    }

    ComboPeer& peer = combo_peers[peer_index];
    int notification_code = HIWORD(w_param);

    if (notification_code == CBN_SELCHANGE) {
        int selected_index = (int)SendMessageA(
            peer.window_handle,
            CB_GETCURSEL,
            0,
            0
        );

        if (peer.combo_box != 0 && selected_index != CB_ERR) {
            peer.combo_box->notify_native_selection_changed(selected_index);
        }

        return true;
    }

    if (notification_code == CBN_CLOSEUP) {
        if (parent_window != NULL && IsWindow(parent_window)) {
            SetFocus(parent_window);
        }
        return true;
    }

    if (notification_code == CBN_DROPDOWN) {
        return true;
    }

    return false;
}

bool Win32NativeControlHost::handle_scroll(
    UINT message,
    WPARAM w_param,
    LPARAM l_param
) {
    HWND source_window = (HWND)l_param;
    int peer_index = find_scroll_peer(source_window);

    if (peer_index < 0) {
        return false;
    }

    ScrollPeer& peer = scroll_peers[peer_index];
    if (peer.scroll_bar == 0) {
        return false;
    }

    ScrollBar* scroll_bar = peer.scroll_bar;
    int new_value = scroll_bar->get_value();
    int request = LOWORD(w_param);

    switch (request) {
        case SB_LINEUP:
            new_value -= scroll_bar->get_line_step();
            break;

        case SB_LINEDOWN:
            new_value += scroll_bar->get_line_step();
            break;

        case SB_PAGEUP:
            new_value -= scroll_bar->get_page_size();
            break;

        case SB_PAGEDOWN:
            new_value += scroll_bar->get_page_size();
            break;

        case SB_TOP:
            new_value = scroll_bar->get_minimum();
            break;

        case SB_BOTTOM:
            new_value = scroll_bar->get_maximum();
            break;

        case SB_THUMBPOSITION:
        case SB_THUMBTRACK: {
            SCROLLINFO scroll_info;
            ZeroMemory(&scroll_info, sizeof(scroll_info));
            scroll_info.cbSize = sizeof(scroll_info);
            scroll_info.fMask = SIF_TRACKPOS;

            if (GetScrollInfo(
                    peer.window_handle,
                    SB_CTL,
                    &scroll_info
                )) {
                new_value = scroll_info.nTrackPos;
            } else {
                new_value = HIWORD(w_param);
            }
            break;
        }

        case SB_ENDSCROLL:
            return true;

        default:
            return false;
    }

    (void)message;
    scroll_bar->notify_native_value_changed(new_value);
    sync_scroll_bar(scroll_bar);
    return true;
}

int Win32NativeControlHost::find_combo_peer(ComboBox* combo_box) const {
    for (int index = 0; index < (int)combo_peers.size(); ++index) {
        if (combo_peers[index].combo_box == combo_box) {
            return index;
        }
    }

    return -1;
}

int Win32NativeControlHost::find_combo_peer(HWND window_handle) const {
    for (int index = 0; index < (int)combo_peers.size(); ++index) {
        if (combo_peers[index].window_handle == window_handle) {
            return index;
        }
    }

    return -1;
}

void Win32NativeControlHost::populate_combo_peer(ComboPeer& peer) {
    if (
        peer.combo_box == 0 ||
        peer.window_handle == NULL ||
        !IsWindow(peer.window_handle)
    ) {
        return;
    }

    SendMessageA(peer.window_handle, CB_RESETCONTENT, 0, 0);

    int item_count = peer.combo_box->get_item_count();

    for (int index = 0; index < item_count; ++index) {
        SendMessageA(
            peer.window_handle,
            CB_ADDSTRING,
            0,
            (LPARAM)peer.combo_box->get_item_text(index)
        );
    }

    peer.item_count = item_count;

    SendMessageA(
        peer.window_handle,
        CB_SETCURSEL,
        (WPARAM)peer.combo_box->get_selected_index(),
        0
    );
}

int Win32NativeControlHost::find_scroll_peer(ScrollBar* scroll_bar) const {
    for (int index = 0; index < (int)scroll_peers.size(); ++index) {
        if (scroll_peers[index].scroll_bar == scroll_bar) {
            return index;
        }
    }

    return -1;
}

int Win32NativeControlHost::find_scroll_peer(HWND window_handle) const {
    for (int index = 0; index < (int)scroll_peers.size(); ++index) {
        if (scroll_peers[index].window_handle == window_handle) {
            return index;
        }
    }

    return -1;
}
