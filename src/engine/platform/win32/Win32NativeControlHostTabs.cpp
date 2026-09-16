// =================================================================================
// Filename:    engine/platform/win32/Win32NativeControlHostTabs.cpp
// Author:      Ebdsaleh
// Description: Implements the native Win32 peer for framework TabView headers.
// =================================================================================

#include <commctrl.h>

#include "Win32NativeControlHost.h"
#include "framework/TabView.h"

Win32NativeControlHost::TabPeer::TabPeer()
    : tab_view(0),
      window_handle(NULL),
      previous_window_proc(0),
      control_id(0),
      revision(-1),
      geometry_valid(false),
      last_x(0),
      last_y(0),
      last_width(0),
      last_height(0),
      visibility_valid(false),
      last_visible(false),
      last_active_index(-1) {
}

bool Win32NativeControlHost::attach_tab_view(TabView* tab_view) {
    if (!is_initialized || tab_view == 0) {
        return false;
    }

    if (find_tab_peer(tab_view) >= 0) {
        return true;
    }

    InitCommonControls();

    TabPeer peer;
    peer.tab_view = tab_view;
    peer.control_id = next_control_id++;

    peer.window_handle = CreateWindowExA(
        0,
        "SysTabControl32",
        "",
        WS_CHILD |
            WS_CLIPSIBLINGS |
            WS_TABSTOP |
            TCS_TABS |
            TCS_SINGLELINE,
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

    SendMessageA(
        peer.window_handle,
        WM_SETFONT,
        (WPARAM)GetStockObject(DEFAULT_GUI_FONT),
        MAKELPARAM(TRUE, 0)
    );

    SetWindowLongA(
        peer.window_handle,
        GWL_USERDATA,
        (LONG)this
    );

    peer.previous_window_proc = (WNDPROC)SetWindowLongA(
        peer.window_handle,
        GWL_WNDPROC,
        (LONG)Win32NativeControlHost::tab_window_proc
    );

    tab_peers.push_back(peer);
    tab_view->set_native_peer_active(true);

    int peer_index = find_tab_peer(tab_view);
    if (peer_index >= 0) {
        populate_tab_peer(tab_peers[peer_index]);
        sync_tab_view(tab_view);
    }

    return true;
}

void Win32NativeControlHost::detach_tab_view(TabView* tab_view) {
    int peer_index = find_tab_peer(tab_view);
    if (peer_index < 0) {
        return;
    }

    TabPeer peer = tab_peers[peer_index];

    if (peer.tab_view != 0) {
        peer.tab_view->set_native_peer_active(false);
    }

    if (peer.window_handle != NULL && IsWindow(peer.window_handle)) {
        if (peer.previous_window_proc != 0) {
            SetWindowLongA(
                peer.window_handle,
                GWL_WNDPROC,
                (LONG)peer.previous_window_proc
            );
        }

        SetWindowLongA(peer.window_handle, GWL_USERDATA, 0);
        DestroyWindow(peer.window_handle);
    }

    tab_peers.erase(tab_peers.begin() + peer_index);
}

void Win32NativeControlHost::sync_tab_view(TabView* tab_view) {
    int peer_index = find_tab_peer(tab_view);
    if (peer_index < 0 || tab_view == 0) {
        return;
    }

    TabPeer& peer = tab_peers[peer_index];
    if (peer.window_handle == NULL || !IsWindow(peer.window_handle)) {
        return;
    }

    if (peer.revision != tab_view->get_revision()) {
        populate_tab_peer(peer);
    }

    int x = tab_view->get_header_x();
    int y = tab_view->get_header_y();
    int width = tab_view->get_header_width();
    int height = tab_view->get_header_height();

    if (
        !peer.geometry_valid ||
        peer.last_x != x ||
        peer.last_y != y ||
        peer.last_width != width ||
        peer.last_height != height
    ) {
        MoveWindow(
            peer.window_handle,
            x,
            y,
            width,
            height,
            TRUE
        );

        peer.last_x = x;
        peer.last_y = y;
        peer.last_width = width;
        peer.last_height = height;
        peer.geometry_valid = true;
    }

    bool visible = tab_view->get_is_visible();
    if (!peer.visibility_valid || peer.last_visible != visible) {
        ShowWindow(
            peer.window_handle,
            visible ? SW_SHOW : SW_HIDE
        );
        peer.last_visible = visible;
        peer.visibility_valid = true;
    }

    EnableWindow(
        peer.window_handle,
        tab_view->get_tab_count() > 0 ? TRUE : FALSE
    );

    int active_index = tab_view->get_active_index();
    int native_index = (int)SendMessageA(
        peer.window_handle,
        TCM_GETCURSEL,
        0,
        0
    );

    if (active_index != native_index) {
        SendMessageA(
            peer.window_handle,
            TCM_SETCURSEL,
            (WPARAM)active_index,
            0
        );
    }

    peer.last_active_index = active_index;
}

LRESULT CALLBACK Win32NativeControlHost::tab_window_proc(
    HWND window_handle,
    UINT message,
    WPARAM w_param,
    LPARAM l_param
) {
    Win32NativeControlHost* host =
        (Win32NativeControlHost*)GetWindowLongA(
            window_handle,
            GWL_USERDATA
        );

    int peer_index = host == 0
        ? -1
        : host->find_tab_peer(window_handle);

    WNDPROC previous_window_proc = 0;
    if (peer_index >= 0) {
        previous_window_proc =
            host->tab_peers[peer_index].previous_window_proc;
    }

    LRESULT result = previous_window_proc != 0
        ? CallWindowProcA(
            previous_window_proc,
            window_handle,
            message,
            w_param,
            l_param
        )
        : DefWindowProcA(
            window_handle,
            message,
            w_param,
            l_param
        );

    if (
        host != 0 &&
        peer_index >= 0 &&
        message == WM_LBUTTONUP
    ) {
        TabPeer& peer = host->tab_peers[peer_index];
        int selected_index = (int)SendMessageA(
            peer.window_handle,
            TCM_GETCURSEL,
            0,
            0
        );

        if (
            peer.tab_view != 0 &&
            selected_index >= 0 &&
            selected_index < peer.tab_view->get_tab_count()
        ) {
            peer.tab_view->notify_native_selection_changed(selected_index);
            host->sync_tab_view(peer.tab_view);

            if (
                host->parent_window != NULL &&
                IsWindow(host->parent_window)
            ) {
                SetFocus(host->parent_window);
                InvalidateRect(host->parent_window, NULL, FALSE);
            }
        }
    }

    return result;
}

int Win32NativeControlHost::find_tab_peer(TabView* tab_view) const {
    for (int index = 0; index < (int)tab_peers.size(); ++index) {
        if (tab_peers[index].tab_view == tab_view) {
            return index;
        }
    }

    return -1;
}

int Win32NativeControlHost::find_tab_peer(HWND window_handle) const {
    for (int index = 0; index < (int)tab_peers.size(); ++index) {
        if (tab_peers[index].window_handle == window_handle) {
            return index;
        }
    }

    return -1;
}

void Win32NativeControlHost::populate_tab_peer(TabPeer& peer) {
    if (
        peer.tab_view == 0 ||
        peer.window_handle == NULL ||
        !IsWindow(peer.window_handle)
    ) {
        return;
    }

    SendMessageA(peer.window_handle, TCM_DELETEALLITEMS, 0, 0);

    int tab_count = peer.tab_view->get_tab_count();

    for (int index = 0; index < tab_count; ++index) {
        TCITEMA item;
        ZeroMemory(&item, sizeof(item));
        item.mask = TCIF_TEXT;
        item.pszText = (LPSTR)peer.tab_view->get_tab_title(index);

        SendMessageA(
            peer.window_handle,
            TCM_INSERTITEMA,
            (WPARAM)index,
            (LPARAM)&item
        );
    }

    peer.revision = peer.tab_view->get_revision();
    peer.last_active_index = -1;

    SendMessageA(
        peer.window_handle,
        TCM_SETCURSEL,
        (WPARAM)peer.tab_view->get_active_index(),
        0
    );
}
