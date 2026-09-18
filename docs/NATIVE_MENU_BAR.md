# Native Application Menu Bar

This document records the SalixWeb32 native application-menu behavior.

## Architectural boundary

The application describes ordinary menu actions as backend-neutral `ApplicationCommand` identifiers. The current Win32 implementation uses a real Windows `HMENU` through `Win32MenuController`; `StatusView` receives semantic command events and remains unaware of Win32 menu handles or numeric native command IDs.

Platform-owned diagnostic capture is the deliberate exception: taking a screenshot of the real native application window belongs to the Win32 host/menu layer. The application view only supplies backend-neutral diagnostic text through `View::build_diagnostic_report()`.

This follows the project rule:

```text
application intent / diagnostic state
    -> framework view contract
    -> current platform implementation
```

The native menu and screenshot mechanism therefore remain Win32 implementation details rather than application dependencies.

## Menu structure

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
    ----------------
    Export Browser Diagnostic Report...
    Take Diagnostic Screenshot

Help
    About SalixWeb32
```

The Edit commands reuse the existing framework keyboard/character command paths rather than implementing a second editor command system. Undo, Cut, Copy, Paste, and Select All therefore retain the same active-control semantics as their keyboard equivalents.

`File -> Attach File...` uses the same `FileDialog`/composer attachment path used by the existing attachment button. `Ctrl+O` is intercepted by the Win32 menu controller and dispatches the same semantic Attach command. `Options` changes the active top-level `TabView` page through application commands.

## Browser diagnostic report

`Options -> Export Browser Diagnostic Report...` writes a timestamped text-only report:

```text
diagnostics/SalixWeb32-Browser-YYYYMMDD-HHMMSS-mmm.txt
```

The export contains Browser title/URL/backend/capability/status metadata followed by the
complete cached `Summary`, `Headers`, `Raw`, and `Extracted` sections. The action
does not perform another web request, does not use the clipboard, and does not render the
complete Raw payload. It exists specifically so target diagnostics can be captured in a
single repeatable operation instead of manually switching modes and pasting into Notepad.

After export, the same NT5-compatible result dialog offers `Go to Files` and `OK`.

## Diagnostic screenshot

`Options -> Take Diagnostic Screenshot` creates a timestamped pair in a local `diagnostics/` directory under the current working directory:

```text
diagnostics/SalixWeb32-YYYYMMDD-HHMMSS-mmm.bmp
diagnostics/SalixWeb32-YYYYMMDD-HHMMSS-mmm.txt
```

The BMP is a capture of the visible SalixWeb32 application window, including its real Win32 non-client area and native controls. The implementation intentionally uses GDI-era APIs that are available on the Windows Server 2003 target; no modern screenshot API or image codec is required.

The companion text report records the active top-level view plus useful application state. When Browser is active it includes:

- Browser title and URL,
- Browser Probe backend, capabilities, and status,
- selected Browser Probe result (`Summary`, `Headers`, `Raw`, or `Extracted`),
- the complete current probe output rather than only the visible scrolled region,
- runtime/client-size diagnostics.

When Runtime is active it includes the runtime diagnostic labels. When Conversation is active it records the current message count plus the common runtime/backend state.

After a successful capture, SalixWeb32 shows an NT5-compatible custom result dialog with two actions:

```text
[ Go to Files ] [ OK ]
```

`Go to Files` opens the generated diagnostics directory in the system shell and closes the result dialog. If Explorer cannot be opened, the result dialog stays open and reports the shell error instead. `OK` simply closes the dialog.

The generated `diagnostics/` directory is ignored by Git so captures can be copied over a network share or sneaker-netted without polluting the repository.

## Win32 implementation

`engine/application_hosts/Win32MenuController` owns the native menu bar. It subclasses the already-created application HWND only to intercept its own menu command IDs and the menu-owned `Ctrl+O` shortcut, forwarding every other window message to the original `Win32ApplicationHost` procedure.

`engine/application_hosts/Win32DiagnosticCapture.h` contains the small Win32/GDI capture helper. The helper asks the active application `View` for a text report, captures the visible window rectangle, writes a 24-bit BMP directly, and writes the text report beside it.

The success notification is implemented as a small owned Win32 window rather than a modern TaskDialog, because the Server 2003 target needs custom button text while remaining independent of Vista-era common controls. Folder opening uses the existing `shell32.lib` dependency through `ShellExecuteA`.

The controller does not replace the application host or its message pump.

## First-pass limitations

- The menu does not yet expose preferences/configuration pages.
- Menu item enable/disable state is not yet dynamically synchronized with control focus, edit-history availability, or clipboard contents.
- Only the Attach shortcut is owned by the menu controller; existing text shortcuts continue through the framework input path.
- Diagnostic capture records the visible window pixels. If another window is deliberately placed over SalixWeb32 at capture time, those visible pixels can appear in the BMP.
- BMP is used deliberately for NT5 simplicity and zero codec dependencies; PNG export can be added later if it becomes useful.

## Current target status

The diagnostic screenshot/report path has been exercised successfully on the primary
Windows Server 2003 / Pentium 4 target: SalixWeb32 created the timestamped BMP and text
report, and the Browser report correctly exposed backend state useful for diagnosing a
placeholder-vs-remote launch.

The later `Go to Files` action required a VC7.1 compatibility correction because the
legacy Platform SDK `ShellAPI.h` assumes Win32 base declarations have already been
introduced. The project now supplies the narrow `src/shellapi.h` compatibility shim
documented in `docs/BUILD_ENVIRONMENT.md`.

The Browser Diagnostic Report export has also been exercised successfully on the
primary target and produced a complete text report containing Summary, redacted Headers,
the full cached Raw response, and Extracted output without rendering the full Raw body.

The checklist below remains authoritative for features that have not yet been explicitly
recorded as target-validated, including the final `Go to Files` interaction if a
separate target confirmation has not been captured.

## Target validation

This tranche must be rebuilt under Visual C++ 7.1 and exercised on the real Pentium 4 under Windows Server 2003 SP2, then MiniXP.

Validation points:

1. The native `File / Edit / Options / Help` bar appears and uses the target OS chrome.
2. `File -> Attach File...` and `Ctrl+O` open the existing multi-file picker and update the composer attachment count.
3. `Edit -> Undo / Cut / Copy / Paste / Select All` follow the same active-control behavior as the corresponding keyboard shortcuts.
4. `Options -> Runtime Diagnostics` and `Options -> Conversation` switch the existing native tabs without losing state.
5. `Options -> Export Browser Diagnostic Report...` creates a timestamped text report containing complete cached Browser Probe data.
6. `Options -> Take Diagnostic Screenshot` creates both a timestamped `.bmp` and `.txt` under `diagnostics/`.
7. Confirm the capture result dialog shows both `Go to Files` and `OK`.
8. Click `Go to Files` and confirm Explorer opens the generated diagnostics directory and the result dialog closes.
9. Open the generated BMP and confirm it contains the complete visible SalixWeb32 window.
10. Open the generated TXT and confirm `Active view` matches the tab that was visible at capture time.
11. With Browser active, confirm the report contains the title, URL, backend/capability/status lines, selected Browser Probe mode, and that mode's complete output.
12. `Help -> About SalixWeb32` opens a normal native message box.
13. `File -> Exit` follows the normal application close/shutdown path.
14. Existing native combo boxes, scrollbars, tabs, context menus, and keyboard navigation remain operational.
