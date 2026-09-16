# Native Application Menu Bar

This document records the first SalixWeb32 native application-menu tranche.

## Architectural boundary

The application describes menu actions as backend-neutral `ApplicationCommand` identifiers. The current Win32 implementation uses a real Windows `HMENU` through `Win32MenuController`; `StatusView` receives semantic command events and remains unaware of Win32 menu handles or numeric native command IDs.

This follows the project rule:

```text
application intent
    -> framework command
    -> current platform implementation
```

The native menu is therefore a Win32 implementation detail rather than an application dependency.

## Initial menu structure

```text
File
    Attach File...      Ctrl+O
    ----------------
    Exit

Edit
    Undo                Ctrl+Z
    ----------------
    Cut                 Ctrl+X
    Copy                Ctrl+C
    Paste               Ctrl+V
    ----------------
    Select All          Ctrl+A

Options
    Conversation
    Runtime Diagnostics

Help
    About SalixWeb32
```

The Edit commands reuse the existing framework keyboard/character command paths rather than implementing a second editor command system. Undo, Cut, Copy, Paste, and Select All therefore retain the same active-control semantics as their keyboard equivalents.

`File -> Attach File...` uses the same `FileDialog`/composer attachment path used by the existing attachment button. `Ctrl+O` is intercepted by the Win32 menu controller and dispatches the same semantic Attach command. `Options` changes the active top-level `TabView` page through application commands.

## Win32 implementation

`engine/application_hosts/Win32MenuController` owns the native menu bar. It subclasses the already-created application HWND only to intercept its own menu command IDs and the menu-owned `Ctrl+O` shortcut, forwarding every other window message to the original `Win32ApplicationHost` procedure.

The controller does not replace the application host or its message pump.

## First-pass limitations

- The menu does not yet expose preferences/configuration pages.
- Menu item enable/disable state is not yet dynamically synchronized with control focus, edit-history availability, or clipboard contents.
- Only the new Attach shortcut is owned by the menu controller; existing text shortcuts continue through the framework input path.

## Target validation

This tranche must be rebuilt under Visual C++ 7.1 and exercised on the real Pentium 4 under Windows Server 2003 SP2, then MiniXP.

Validation points:

1. The native `File / Edit / Options / Help` bar appears and uses the target OS chrome.
2. `File -> Attach File...` and `Ctrl+O` open the existing multi-file picker and update the composer attachment count.
3. `Edit -> Undo / Cut / Copy / Paste / Select All` follow the same active-control behavior as the corresponding keyboard shortcuts.
4. `Options -> Runtime Diagnostics` and `Options -> Conversation` switch the existing native tabs without losing state.
5. `Help -> About SalixWeb32` opens a normal native message box.
6. `File -> Exit` follows the normal application close/shutdown path.
7. Existing native combo boxes, scrollbars, tabs, context menus, and keyboard navigation remain operational.
