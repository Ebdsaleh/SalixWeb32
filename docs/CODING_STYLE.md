# SalixWeb32 Coding Style

SalixWeb32 follows the established style used by the wider Salix C/C++ projects where practical, while remaining compatible with Visual C++ 7.1 for the initial legacy target.

## Naming

Use **PascalCase** for types:

```cpp
class ApplicationRuntime;
class ServiceRegistry;
class Win32ApplicationHost;
```

Use **snake_case** for functions, methods, variables, parameters, and data members:

```cpp
bool initialize();
void update_all();
int service_count;
HINSTANCE instance_handle;
ApplicationRuntime* application_runtime;
```

Use descriptive names instead of unnecessary abbreviations.

## Files

C++ class filenames use the class name:

```text
ApplicationRuntime.h
ApplicationRuntime.cpp
Win32ApplicationHost.h
Win32ApplicationHost.cpp
```

Directories use lowercase descriptive names.

## Compatibility baseline

Until the baseline target changes, production code must compile with Visual C++ 7.1 / Visual Studio .NET 2003.

Do not use language features unavailable to that compiler, including modern C++ conveniences such as:

```text
nullptr
make_unique
auto type deduction
range-for
override/final keywords
lambda expressions
C++11 enum classes
```

Prefer explicit, unsurprising code over compatibility shims when the simpler form is adequate.

## Layout

Opening braces follow declarations and control statements on the same line, consistent with the existing Salix C++ style:

```cpp
bool ApplicationRuntime::initialize() {
    if (is_initialized) {
        return true;
    }

    return false;
}
```

Indent with four spaces in source files. Keep lifecycle and ownership behavior explicit.

## Architecture

Application-specific code must not reach through abstraction boundaries to call backend implementation details directly. In particular, product code must not become coupled to Gecko, Chromium, Python, or a specific Win32 renderer.
