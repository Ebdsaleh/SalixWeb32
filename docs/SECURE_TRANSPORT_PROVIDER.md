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

## Current validation target

For this tranche no secure DLL is expected to exist yet.

The real P4 should therefore report:

```text
Secure transport: Salix Secure Transport Provider |
not installed | real content remains blocked
```

The existing Conversation probe must remain:

```text
mode probe-only
transport plaintext
text no
attachments no
credentials no
session no
```

and Browser Probe must remain operational.

That proves provider discovery is optional and fail-closed before cryptographic code is
introduced.
