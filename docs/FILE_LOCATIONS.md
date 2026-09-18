# Persistent File Locations

SalixWeb32 must not use the process current working directory as implicit persistent
storage state.

On classic Win32, common file dialogs can change the process current directory unless
explicitly told not to. Diagnostics, settings, caches, downloads, exports, and future
application-owned data therefore use explicit roots that are resolved once at startup.

## Application roots

SalixWeb32 distinguishes these concepts:

```text
launch_directory
    process working directory captured at startup
    useful for development/local-config discovery

executable_root
    directory containing SalixWeb32.exe
    resolved with GetModuleFileNameA

user_data_root
    owner of writable persistent application state
```

The application-storage rule is:

```text
                    STANDARD                         PORTABLE (--portable)

executable_root     executable directory             executable directory

user_data_root      %APPDATA%\SalixWeb32            executable directory

settings.ini        user_data_root\settings.ini      user_data_root\settings.ini

Diagnostics         user_data_root\Diagnostics       user_data_root\Diagnostics
```

Standard mode never derives writable persistent storage from the executable directory or
from a file dialog. If `%APPDATA%\SalixWeb32` cannot be resolved or created, startup
reports that failure instead of silently changing the storage contract.

Portable mode deliberately opts into executable-directory storage:

```text
SalixWeb32.exe --portable
```

The mode is decided once at startup and remains immutable for that process.

## Attachment picker history is not a storage preference

The attachment picker has different semantics from Diagnostics.

Diagnostics answers:

> Where should SalixWeb32-owned diagnostic output be written?

The attachment picker answers:

> Where did the user browse most recently?

That second value is navigation history, not application-storage policy. SalixWeb32
therefore keeps it out of the Settings dialog.

The picker behavior is:

```text
settings.ini contains attachment_directory
        |
        +-- yes -> open from that remembered directory
        |
        `-- no  -> start at %USERPROFILE%
                       |
                       +-- unavailable -> captured launch directory
                       |
                       `-- unavailable -> executable directory
```

After a successful selection, the picker directory is written back to:

```ini
attachment_directory=X:\most\recent\folder
```

This happens automatically and best-effort. Attachment use must not fail merely because
recent-folder persistence fails.

There is intentionally no "always use this directory" control at this stage. Such an
override would compete with normal last-used-folder behavior and add friction. It can be
introduced later if a concrete workflow requires fixed-directory behavior.

## Standard mode

A normal launch stores Salix-owned writable state here:

```text
%APPDATA%\SalixWeb32\
    settings.ini
    Diagnostics\
```

`SalixWeb32.exe` itself may live anywhere. Moving or launching the executable from a
different folder does not redirect Standard-mode persistent state.

## Portable mode

Launching with `--portable` makes the executable directory the data root:

```text
<executable_root>\
    SalixWeb32.exe
    settings.ini
    Diagnostics\
```

The attachment picker still uses its own remembered navigation history. With no saved
history, its first-use location remains `%USERPROFILE%`; Portable mode changes
application-owned state placement, not the user's normal file-browsing starting point.

## Launch directory and development configuration

The launch directory remains useful, but it is not the application data root.

SalixWeb32 captures it once before any file dialog can run. Existing development lookup
for `salixweb32.local.ini` may use that captured location so Visual Studio .NET 2003
launches from `build\vs2003` continue to find the repo-local bridge configuration.

The live process current directory is never re-read later to decide where persistent
application data belongs.

## User-facing Settings

`Options -> Settings...` exposes:

```text
Application mode:
    Standard
or
    Portable (--portable)

User data / Data root:
    resolved user_data_root

Diagnostics folder:
    configurable absolute path

Preferences file:
    resolved settings.ini
```

The application mode is informational in Settings. It is not a checkbox because changing
storage roots in the middle of a running process would make ownership and migration
ambiguous.

`Restore Defaults` restores only the visible Diagnostics preference to
`<user_data_root>\Diagnostics`. It does not erase the attachment picker's automatic
recent-directory history.

## Preference and state persistence

`salixweb32.local.ini` remains the machine/development configuration layer for bridge
selection and companion endpoint settings.

`settings.ini` contains both explicit user preferences and small pieces of persistent UI
state:

```ini
diagnostics_directory=C:\...\Diagnostics
attachment_directory=X:\most\recent\folder
```

These keys deliberately have different semantics:

```text
diagnostics_directory
    user preference
    visible/editable in Options -> Settings...

attachment_directory
    automatic recent-navigation state
    not exposed as an editable setting
```

In Standard mode the file belongs under `%APPDATA%\SalixWeb32`. In Portable mode it
belongs beside the executable.

## File-dialog rule

The Win32 attachment picker uses `OFN_NOCHANGEDIR`.

The picker receives its remembered initial directory explicitly and reports the last
successful selection directory back to the application. SalixWeb32 persists that browse
history without changing any application storage root.

Therefore:

```text
open attachment from X:\RenderWare\Textures
        |
        +-- next attachment dialog reopens there
        |
        +-- settings.ini quietly records that recent folder
        |
        `-- diagnostics remain under the configured Diagnostics folder
```

The process current directory is not a communication channel between these features.

## Diagnostics rule

Diagnostic screenshot/report capture and Browser Diagnostic Report export receive the
configured Diagnostics directory explicitly.

They do not call `GetCurrentDirectoryA` to decide where output belongs.

Default:

```text
Standard:
    %APPDATA%\SalixWeb32\Diagnostics

Portable:
    <executable_root>\Diagnostics
```

The user may choose another absolute Diagnostics location through Settings. `Go to Files`
opens the directory that actually received the generated artifact.

Diagnostic reports may include the current **Attachment recent folder** for observability.
That does not make the value a user-facing application-storage setting.

## Future categories

Future persistent categories should be added beneath or explicitly derived from
`user_data_root` unless their semantics require a user-selected location:

```text
ApplicationPaths
    executable_root
    user_data_root
    settings_file
    diagnostics_root
    cache_root
    logs_root
    downloads_root
    exports_root
```

Navigation history such as a file picker's recent directory remains separate from these
application-owned storage categories.

## Design invariant

> Application-owned storage has explicit roots. File-picker navigation is remembered
> automatically, but it never determines where SalixWeb32 stores its own data.
