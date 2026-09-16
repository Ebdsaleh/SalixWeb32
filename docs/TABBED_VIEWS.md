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

Close/reopen/reorder shortcuts are intentionally not claimed by this tranche. They will be layered onto the same model after the basic tab lifecycle is stable.

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

## VC7.1 / Platform SDK compatibility note

The first real Pentium 4 rebuild exposed a header-order dependency in the Visual Studio .NET 2003 Platform SDK. `Win32NativeControlHostTabs.cpp` originally included `commctrl.h` before any header that supplied the base Win32 declarations. On that SDK, `commctrl.h`/`PrSht.h` expect types and macros such as `HRESULT`, `CALLBACK`, and `UINT` to have already been declared by `windows.h`.

The failing translation unit therefore produced a cascade beginning with:

```text
CommCtrl.h(30): error C2146: missing ';' before identifier 'HRESULT'
CommCtrl.h(30): error C2501: 'HRESULT' missing storage-class or type specifiers
PrSht.h(97): error C2065: 'CALLBACK' undeclared identifier
PrSht.h(97): error C2065: 'LPFNPSPCALLBACKA' undeclared identifier
PrSht.h(97): error C2501: 'UINT' missing storage-class or type specifiers
```

This was not a broken tab API and did not indicate missing common-controls libraries. The project already links `comctl32.lib`. The compatibility fix is simply to include `Win32NativeControlHost.h` first (which includes `windows.h`) and include `commctrl.h` afterwards.

That ordering rule should be preserved for future Win32 common-control translation units targeting the VC7.1-era SDK.

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

Those remain follow-on capabilities rather than requirements of the validated base tab model.

## Target validation status

**Validated successfully on the real Pentium 4 under both Windows Server 2003 SP2 and MiniXP.**

The validation pass confirmed:

1. Visual C++ 7.1 clean rebuild succeeds after the common-controls include-order correction.
2. The native Windows tab header appears correctly.
3. The initial `Conversation` and `Runtime` pages switch correctly by mouse.
4. Conversation/composer state survives switching away and back.
5. Composer native font/language/indent combo boxes and scrollbars detach from the inactive Conversation page instead of floating over Runtime.
6. Native controls recreate/synchronize correctly when returning to Conversation.
7. The conversation native scrollbar follows the same lifecycle.
8. `Ctrl+Tab` and `Ctrl+Shift+Tab` cycle the pages correctly.
9. Page layout and native tab geometry track resizing.
10. Existing conversation formatting, lists, code blocks, context menus, selection, and scrolling remain functional after tab switching.
11. The same architecture and executable behavior remain sound under MiniXP as well as Server 2003 SP2.

This validation closes the first tabbed-workspace tranche and establishes it as a stable baseline for later application-shell work.
