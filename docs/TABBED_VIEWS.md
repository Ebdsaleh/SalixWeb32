# Tabbed Views

This document records the first SalixWeb32 tabbed-workspace tranche.

The goal is not merely to draw tab-shaped buttons. Tabs are a reusable framework concept with an optional native platform peer, a stable active-page model, keyboard navigation, and explicit lifecycle handling for native child controls owned by a tab page.

## Framework model

`framework/TabView` owns tab metadata and tab-header controls, but it does not own the page components supplied by the application.

The basic model is:

```text
TabView
    -> tab 0 title + page component
    -> tab 1 title + page component
    -> ...
```

Only the active page is visible. Switching tabs changes page visibility and raises a backend-neutral tab-changed callback.

The initial public operations include:

```text
add_tab(title, component)
remove_tab(index)
set_active_index(index)
get_active_index()
get_tab_count()
get_tab_title(index)
get_tab_content(index)
```

`TabView` also exposes separate header and content geometry. That allows a platform-native header to coexist with framework-rendered page content without forcing page layout into a Win32-specific control.

## Native Win32 tab header

When a `NativeControlHost` is available, Win32 attaches a real `SysTabControl32` header to the framework `TabView`.

The common-controls peer is implemented separately in:

```text
engine/platform/win32/Win32NativeControlHostTabs.cpp
```

The Visual C++ 7.1 project links `comctl32.lib`, and the peer initializes common controls before creating the tab HWND.

The native control is used only for the tab header. Page content remains part of the Salix component tree. This keeps the semantic split clear:

```text
TabView state
    -> active index
    -> titles
    -> page visibility

Win32 peer
    -> native tab chrome
    -> native mouse selection

Salix renderer
    -> active page contents
```

When the native peer is active, `TabView` hides its framework fallback header. The fallback exists so the control remains usable on a future backend that does not supply a native tab peer.

## Keyboard behavior

The first tranche supports familiar desktop tab cycling:

```text
Ctrl+Tab        -> next tab
Ctrl+Shift+Tab  -> previous tab
```

Cycling wraps at both ends.

Close/reopen/reorder shortcuts are intentionally not claimed by this tranche. They will be layered onto the same model after the basic tab lifecycle is target-validated.

## Initial shell integration

The existing shell is reorganized into two top-level workspace pages:

```text
Conversation
Runtime
```

### Conversation

The Conversation tab owns the existing conversation presentation and message composer. The old diagnostics sidebar is removed from the conversation page, giving the conversation and composer the full workspace width.

### Runtime

The Runtime tab owns the runtime/host/web-backend/service/client-size diagnostics that were previously shown in the sidebar.

This is deliberately useful rather than a fake tab demo: switching tabs changes which application surface owns the body of the window.

## Native child-control lifecycle

This tranche also establishes an important rule for future web/chat tabs.

Framework visibility alone is not enough for native child HWNDs. A hidden tab page may contain native combo boxes or scrollbars that would otherwise remain visible above the custom-rendered page.

`StatusView` therefore attaches native controls only for the active Conversation page:

```text
Conversation active
    -> conversation scrollbar attached
    -> composer scrollbars attached
    -> native combo boxes attached

Runtime active
    -> conversation/composer native peers detached
```

Returning to Conversation recreates/synchronizes those peers from the persistent framework state. Text, selections, scroll offsets, formatting, and message history remain in the framework objects and are not lost by switching tabs.

This explicit lifecycle is groundwork for future tabs containing `WebView`, multiple chat sessions, inspectors, downloads, diagnostics, or other surfaces with native/platform resources.

## Current limitations

The first tranche intentionally does not yet provide:

- a `+` new-tab button,
- close buttons,
- `Ctrl+W` close-tab handling,
- drag reordering,
- tab pinning,
- tab overflow scrolling/dropdown,
- per-tab icons,
- persistence/restoration across application launches,
- multiple independent conversation sessions.

Those are follow-on capabilities. The current goal is to validate the reusable tab model, native header, active-page switching, keyboard cycling, and native-child lifecycle on the real legacy targets first.

## Target validation checklist

This tranche is not target-validated until exercised on the real Pentium 4 systems.

1. Close Visual Studio before pulling because the `.vcproj` gains new source files and `comctl32.lib`.
2. Reopen the solution and perform Clean Solution -> Rebuild Solution under Visual C++ 7.1.
3. Confirm there are no new warnings or link errors involving common controls.
4. Launch on Windows Server 2003 SP2 and confirm a native Windows tab header appears below the Salix header.
5. Confirm the initial tabs are `Conversation` and `Runtime`.
6. Click Runtime and verify the conversation/composer disappear and diagnostics occupy the workspace.
7. Click Conversation and verify the entire conversation/composer state returns unchanged.
8. Confirm the composer native font/language/indent combos and scrollbars do not remain floating over the Runtime tab.
9. Return to Conversation and verify those native controls recreate at the correct locations and retain semantic state.
10. Verify the conversation native scrollbar likewise disappears/reappears correctly.
11. Press Ctrl+Tab repeatedly and confirm forward cycling wraps between the two tabs.
12. Press Ctrl+Shift+Tab and confirm reverse cycling.
13. Resize the window on both tabs and verify the native tab header and active page track the new client size.
14. Send formatted/list/code messages, switch to Runtime, return, and verify draft/history state has not been reset.
15. Exercise context menus, conversation selection, mouse-wheel scrolling, code blocks, and the composer after multiple tab switches to catch lifecycle regressions.
16. Repeat the smoke pass under MiniXP after Server 2003 succeeds.
