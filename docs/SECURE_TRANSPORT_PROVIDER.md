# Secure Transport Provider

SalixWeb32 must gain an authenticated/encrypted Conversation content path without forcing
the VC7.1 application binary to absorb a modern TLS implementation directly.

The chosen architecture is an **optional secure transport provider DLL** loaded through a
small versioned C ABI.

## Boundary

```text
SalixWeb32.exe
    Visual C++ 7.1 / NT 5.2 baseline
        |
        | flat C ABI only
        v
SalixSecureTransport.dll
    newer compiler/toolset allowed
    statically owns its CRT/dependencies
        |
        v
maintained TLS provider
        |
        v
authenticated/encrypted connection
        |
        v
modern companion
```

The provider is optional. Its absence must not stop the existing local placeholder,
Browser Probe, or content-free Conversation probe paths.

Its absence also must **never** make real Conversation content eligible for the current
plaintext bridge.

## Why a DLL boundary

A modern security library may require a newer C/C++ compiler than VC7.1 while still being
able to produce an x86 binary that runs on NT 5.2.

A DLL boundary lets SalixWeb32 preserve its authoritative legacy build while the
security-sensitive provider can use a newer toolchain.

The ABI deliberately avoids C++ objects, STL types, exceptions, RTTI, and cross-runtime
allocation ownership.

Provider-facing data must use:

```text
fixed-width/Win32-stable integer values
caller-owned byte buffers
explicit buffer lengths
status/error codes
flat exported C functions
```

No object allocated by one CRT may be freed by the other side.

## Provider discovery ABI

The first ABI revision is discovery-only:

```text
salix_secure_transport_get_abi_version()
salix_secure_transport_get_capabilities()
salix_secure_transport_get_provider_name()
```

ABI version:

```text
SALIX_SECURE_TRANSPORT_ABI_VERSION = 1
```

Required security capability classes are:

```text
TLS 1.2 or TLS 1.3
peer authentication
certificate pinning
```

Discovery does not authorize Conversation content. A later ABI revision will add the
actual connection/request primitives only after this loader boundary is validated on the
real target.

## Safe loading rule

On Win32, SalixWeb32 looks only for:

```text
<executable_directory>\SalixSecureTransport.dll
```

It constructs an absolute path and passes that path to `LoadLibraryA`.

It does not ask the normal process DLL search path to locate the provider by bare name.

The future provider build should statically link its C runtime and TLS dependency where
practical so that loading the provider does not introduce an uncontrolled dependency
search chain on NT 5.2.

## Fail-closed behavior

Provider states are intentionally non-fatal to the application:

```text
DLL absent
    -> application still runs
    -> real content remains blocked

DLL cannot load
    -> application still runs
    -> provider incompatible
    -> real content remains blocked

ABI version mismatch
    -> provider incompatible
    -> real content remains blocked

required security capability missing
    -> provider incompatible
    -> real content remains blocked

ABI discovery succeeds
    -> provider discovery ready
    -> Conversation content STILL remains blocked
```

The final state is important: discovering a TLS-capable DLL is not the same thing as
having an authenticated Conversation connection.

## Initial TLS provider candidate

The first implementation spike should use **Mbed TLS 3.6.x LTS**, beginning with the
current patched 3.6 release available at implementation time.

Reasons:

- it is a maintained 3.6 LTS line through March 2027,
- it implements TLS 1.2 and TLS 1.3,
- it is available under Apache-2.0 as one licensing option,
- upstream tests/builds with newer Microsoft Visual C++ toolchains,
- Visual Studio 2017 provides an XP platform toolset capable of targeting Windows XP /
  Windows Server 2003,
- isolating it behind this DLL means the main VC7.1 ABI does not change if the provider
  later needs to move to another library or toolchain.

This is a **target compatibility candidate**, not a declaration that the final DLL has
already been proven on Server 2003.

The next provider spike must demonstrate:

```text
VS2017 XP-toolset x86 build
load on Windows Server 2003 SP2
no post-NT5 imported APIs
static CRT ownership
Mbed TLS initialization
entropy/RNG initialization
TLS 1.2 or TLS 1.3 client handshake
peer certificate validation
hostname verification
certificate/public-key pinning
repeatable connect/disconnect
clean error reporting
acceptable P4 CPU/RAM cost
```

If the maintained Mbed TLS branch cannot meet those requirements without an unreasonable
port, the ABI remains stable and another mature provider can replace it.

## Credential/session rule

The provider is not a credential vault.

The intended secure Conversation architecture keeps provider/service credentials and web
session material on the modern companion wherever practical.

The P4 should send only the semantic content actually required for a request, over the
authenticated/encrypted channel, after the Conversation security profile permits it.

Credentials and session-state capabilities remain disabled until a separate requirement
demonstrates that they must cross this boundary.

## Provider-absent target result

The provider-absent state has now been validated on the real Server 2003 / Pentium 4
target.

The diagnostic report recorded:

```text
Secure transport: Salix Secure Transport Provider |
not installed | real content remains blocked
```

while the existing Conversation profile remained probe-only/plaintext with all sensitive
data classes denied and Browser Probe remained operational.

This proves provider discovery is optional and fail-closed before cryptographic code is
introduced.

## ABI-test provider

Before introducing Mbed TLS, SalixWeb32 now includes a deliberately non-secure
cross-toolchain test DLL:

```text
src/security/providers/abi_test/SalixSecureTransportAbiTest.c
src/security/providers/abi_test/SalixSecureTransport.def
build/secure_transport/SalixSecureTransport.vcxproj
tools/build_secure_transport_provider.bat
```

The project targets:

```text
Win32 / x86
v141_xp
static CRT
C ABI
WINVER/_WIN32_WINNT = 0x0502
```

The module-definition file exports the ABI names without x86 C-name decoration so the
VC7.1 executable can resolve exactly:

```text
salix_secure_transport_get_abi_version
salix_secure_transport_get_capabilities
salix_secure_transport_get_provider_name
```

The test provider returns ABI version 1 and the provider name:

```text
Salix Secure Transport ABI Test Provider
```

but intentionally returns:

```text
capabilities = 0
```

It therefore **must not** become ready and **must not** authorize Conversation content.

A successful target result is:

```text
Secure transport: Salix Secure Transport ABI Test Provider |
ABI 1 loaded | required TLS/authentication/pinning capabilities missing |
real content remains blocked
```

This result proves all of the following independently of a cryptographic library:

- a newer-toolchain x86 DLL can load on the Server 2003 target,
- the VC7.1 executable can resolve and call the flat C ABI,
- the ABI version matches,
- the provider identity crosses the ABI correctly,
- the capability gate rejects an intentionally insufficient provider,
- failure remains non-fatal and fail-closed.

Only after this test is green should Mbed TLS be introduced behind the provider boundary.

## Building the ABI-test provider

Run on the modern development/companion machine from a Visual Studio Developer Command
Prompt:

```bat
tools\build_secure_transport_provider.bat
```

The script rebuilds:

```text
build\secure_transport\SalixSecureTransport.vcxproj
Configuration: Release
Platform:      Win32
Toolset:       v141_xp
```

and writes:

```text
build\secure_transport\bin\Release\SalixSecureTransport.dll
```

When `dumpbin` is available, the helper also prints the DLL exports and direct
dependencies.

If MSBuild reports that `v141_xp` is unavailable, install that XP-compatible toolset
rather than silently switching the test to a newer non-XP platform toolset.

For the target test, copy only the resulting DLL beside the P4's
`SalixWeb32.exe`. Do not place it in PATH or System32; provider discovery is explicitly
executable-directory scoped.
