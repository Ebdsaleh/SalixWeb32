# Conversation Scrolling

This document records the conversation-history scrolling tranche that follows the code-composer viewport work. The new conversation scrolling behavior still requires target validation on Visual C++ 7.1 / Windows Server 2003 SP2 and MiniXP.

## Motivation

The first append-only `ConversationView` used two framework buttons (`^` and `v`) to move one historical message at a time. That was useful while the message model was being established, but it is not a suitable long-term interaction model for SaaS/LLM conversations.

The conversation surface now uses the same backend-neutral `ScrollBar` abstraction introduced for the composer viewport. On Win32, `Win32NativeControlHost` gives that abstraction a real child-window `SCROLLBAR` peer.

This keeps application code independent from HWNDs while allowing Windows Server 2003 / XP to provide the platform-native scrollbar chrome and interaction behavior.

## Scroll model

Conversation history currently scrolls by **message entry**, not by raw pixels. The scrollbar value represents the index of the first visible message.

This preserves the existing append-only model and variable-height message rows while avoiding mutations to message text, formatting, Markdown source, selections, or clipboard state.

When the total historical-message height exceeds the visible conversation area:

- the native vertical scrollbar becomes visible,
- the old dedicated `^` / `v` conversation buttons are no longer used,
- line-step scroll requests move one message,
- page-step requests move by the native scrollbar page amount,
- dragging the thumb selects a new first-visible message index,
- appending a new message continues to follow the newest visible history.

When all messages fit, the scrollbar is hidden.

## Mouse wheel

`UIEvent` now has a backend-neutral `event_mouse_wheel` type and `wheel_delta` value.

The Win32 application host translates `WM_MOUSEWHEEL` into this event after converting the screen-coordinate cursor position from `lParam` into client coordinates.

When the pointer is over `ConversationView`, one wheel notch moves three message entries. Positive wheel motion moves toward older history and negative wheel motion moves toward newer history.

The wheel event is routed through the normal framework view/component event path; the application layer does not depend on Win32 wheel constants.

## Native-control lifecycle

`ConversationView` now participates in the same native-control lifecycle as `MessageComposer`:

```text
StatusView
    |
    +-- ConversationView
    |       |
    |       +-- framework::ScrollBar
    |                 |
    |                 +-- Win32 SCROLLBAR peer
    |
    +-- MessageComposer
            |
            +-- native combo/scroll peers
```

`StatusView::attach_native_control_host()` attaches both surfaces. Detach/shutdown destroys the native peer without changing the framework conversation model.

The framework-drawn scrollbar fallback remains available when no native peer exists.

## Selection and rendering

Scrolling changes only which message labels are visible and where those labels are arranged. Each historical message remains its own selectable read-only `Label`, so the existing word selection, discontinuous selection, subtractive selection, and clipboard semantics remain attached to the message itself.

Markdown presentation and graphical emoticons are unaffected by the scroll state.

## Current limit

The current history viewport scrolls between complete message entries. A **single message whose rendered row is taller than the entire conversation viewport** is not yet internally pixel-scrollable. Rich SaaS responses will eventually require a true clipped pixel viewport / wrapped block layout so one very large response can be traversed independently.

That work should be implemented as conversation-layout infrastructure rather than by mutating or splitting the canonical message source.

## Previous code-composer target observation

The 2026-09-16 Windows Server 2003 target screenshot after commit `4a1d8b8` visibly confirms the code-composer controls and native composer viewport are present and running on the Pentium 4 target: the `<code />` toggle, native indentation combo, horizontal scrollbar, vertical scrollbar, and a submitted multiline code example are all visible. MiniXP validation for that tranche is still pending unless separately recorded later.

## Validation checklist

Before marking this conversation-scrolling tranche validated:

1. Build and link with Visual C++ 7.1.
2. Send enough messages to overflow the conversation area.
3. Verify a native Windows vertical scrollbar appears instead of the old `^` / `v` buttons.
4. Exercise scrollbar arrows, track/page movement, and thumb dragging.
5. Verify newly submitted messages still auto-follow to the newest history.
6. Put the pointer over conversation history and verify the mouse wheel moves through older/newer messages.
7. Scroll to an older message, select/copy text, and verify selection remains attached to the correct message.
8. Exercise multiline Markdown, graphical emoticons, and mixed font sizes while scrolling.
9. Resize the window repeatedly and verify scrollbar range/visibility update correctly.
10. Repeat under Windows Server 2003 SP2 and MiniXP.
