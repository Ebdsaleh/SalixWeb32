# Persistent File Locations

SalixWeb32 must not use the process current working directory as implicit persistent
storage state.

On classic Win32, common file dialogs can change the process current directory unless
explicitly told not to. Diagnostics, settings, caches, downloads, exports, and future
application-owned data therefore use explicit roots that are resolved once at startup.

## Application roots

SalixWeb32 distinguishes three concepts:

```text
launch_directory
    process working directory captured at startup
    useful for development/local-config discovery and the first attachment browse

executable_root
    directory containing SalixWeb32.exe
    resolved with GetModuleFileNameA

user_data_root
    owner of writable persistent application state
```

The storage rule is:

```text
                    STANDARD                         PORTABLE (--portable)

executable_root     executable directory             executable directory

user_data_root      %APPDATA%\SalixWeb32            executable directory

settings.ini        user_data_root\settings.ini      user_data_root\settings.ini

Diagnostics         user_data_root\Diagnostics       user_data_root\Diagnostics

Attachment browser  independent remembered directory independent remembered directory
```

Standard mode never derives writable persistent storage from the executable directory or
from a file dialog. If `%APPDATA%\SalixWeb32` cannot be resolved/created, startup reports
that failure instead of silently changing the storage contract.

Portable mode deliberately opts into executable-directory storage:

```text
SalixWeb32.exe --portable
```

The mode is decided once at startup and remains immutable for that process.

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

This is intentional portable behavior, not a fallback used by Standard mode.

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

Attachment browser folder:
    configurable remembered browse location

Preferences file:
    resolved settings.ini
```

The mode is informational in Settings. It is not a checkbox because changing storage
roots in the middle of a running process would make ownership and migration ambiguous.

`Restore Defaults` restores Diagnostics to `<user_data_root>\Diagnostics`. The
attachment browser returns to the captured launch directory (or executable directory if
no launch directory is available).

## Preference persistence

User-facing preferences are intentionally separate from `salixweb32.local.ini`.

`salixweb32.local.ini` remains the machine/development configuration layer for bridge
selection and companion endpoint settings.

`settings.ini` stores user-facing location state:

```ini
diagnostics_directory=C:\...\Diagnostics
attachment_directory=X:\...
```

In Standard mode that file belongs under `%APPDATA%\SalixWeb32`. In Portable mode it
belongs beside the executable.

## File-dialog rule

The Win32 attachment picker uses `OFN_NOCHANGEDIR`.

The picker receives its initial directory explicitly and reports the last successful
selection directory back to the application. SalixWeb32 persists that browse directory
without changing any application storage root.

Therefore:

```text
open attachment from X:\RenderWare\Textures
        |
        +-- next attachment dialog may reopen there
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

Each feature asks for the directory it owns. Nothing asks "what folder are we in right
now?"

## Design invariant

> Standard mode stores SalixWeb32 state under `%APPDATA%\SalixWeb32`. Portable mode
> stores it beside the executable. File dialogs never determine application storage
> locations.
