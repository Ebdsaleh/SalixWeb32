# Dependency Strategy

SalixWeb32 is intended to become increasingly self-contained, but **functional modern communication comes before reimplementing every subsystem in-house**.

The working rule is:

```text
adopt proven open-source source code
        -> build it for the Salix target
        -> port/fork it when the upstream build no longer targets NT 5.2 / VC7.1
        -> hide it behind a Salix contract
        -> validate it on the real Pentium 4
        -> ship useful functionality
        -> progressively replace or internalize pieces where useful
```

The important distinction is that an upstream project's published compiler/OS support matrix is **not** a declaration that its source code is unusable on SalixWeb32. It describes what upstream currently builds, tests, and supports. SalixWeb32 is explicitly willing to maintain target-specific build glue and source patches where the engineering cost is justified.

The source is the starting material. If a dependency is otherwise a strong architectural fit, failure to build unchanged with VC7.1 is a porting problem to investigate, not an automatic rejection criterion.

## Primary product direction

The remote bridge is useful validation scaffolding, a development oracle, and potentially a supported optional backend. It is **not a substitute for the native SalixWeb32 communications path**.

The primary native goal remains:

```text
Pentium 4 / Windows Server 2003
        -> Salix DNS/socket layer
        -> Salix HTTP layer
        -> qualified TLS provider
        -> modern HTTPS
        -> service protocol / authentication
        -> https://chatgpt.com and other modern services
```

SalixWeb32 on the legacy machine is intended to communicate, authenticate, transport, parse, and translate the data required by the selected modern service. The project may use open-source libraries internally to reach that goal without surrendering ownership of the surrounding architecture.

## Architecture rule

Application/framework code must depend on Salix-owned interfaces rather than a third-party library directly.

Examples:

```text
application
    -> JsonDocument / JsonReader
        -> vendored or Salix-ported JSON provider

web/network
    -> NetworkTransport / HttpClient
        -> Salix implementation and/or vendored provider

web/security
    -> TlsSession / CertificateVerifier
        -> Salix-ported TLS/crypto provider
```

If a dependency is replaced, forked, substantially patched, or eventually reimplemented, the application-facing contract should remain stable.

Do not scatter `openssl/*`, JSON-provider headers, curl handles, or another provider's concrete objects throughout application code.

## Source-first rule

When evaluating an open-source dependency, use this order:

```text
1. inspect the current source and license
2. try the least-invasive target build
3. isolate compiler/build-system failures
4. patch compatibility locally
5. fork when the patch set becomes a maintained Salix port
6. expose only a narrow Salix-facing API
7. validate on the real P4
```

Do not discard a library merely because its current upstream CI no longer includes Windows Server 2003, Visual C++ 7.1, or NT5.

Likewise, do not blindly force a dependency onto the target when the required port would cost more than a simpler implementation. The decision is engineering-cost based, not support-matrix based.

## What should be reused first

The first reuse candidates are expensive, mature, or security-sensitive pieces whose complete reimplementation would delay the actual SalixWeb32 goal:

- TLS protocol machinery and cryptographic primitives,
- X.509/certificate parsing and verification,
- JSON parsing/serialization,
- compression/decoding where modern services require it,
- selected URL/HTTP helpers when they reduce risk or implementation time.

The surrounding socket, HTTP, session, diagnostics, service, document, and application architecture remains Salix-owned.

## Cryptography exception

"Eventually in-house" does **not** mean casually inventing cryptographic algorithms.

Salix can own the TLS/session/certificate architecture, adapters, policy, storage, diagnostics, protocol integration, and even maintain a port/fork of a cryptographic library. The underlying cryptographic algorithms should continue to come from implementations that are well reviewed and testable.

Do not invent AES, ChaCha20, RSA, ECC, hashing, signature verification, or random-number-generation algorithms merely to eliminate a dependency.

## Legacy compiler and runtime rule

Every dependency has several separate compatibility questions:

1. Can the source be made to compile for x86 using VC7.1, another suitable compiler, or a small compatibility layer?
2. If not unchanged, how large is the required Salix port/fork?
3. Does the resulting binary actually run correctly on Windows Server 2003 SP2 / NT 5.2?
4. Does it avoid importing post-NT5 APIs accidentally?
5. Can ownership, allocation, and ABI boundaries remain explicit?
6. Is the resulting implementation maintainable enough for the benefit it provides?

A library is not accepted merely because it says "Win32", and it is not rejected merely because upstream no longer advertises NT5.

The real Pentium 4 remains authoritative.

If a dependency cannot sensibly compile inside the VC7.1 solution, a narrow C ABI DLL/static-library boundary may be used, provided:

- the produced binary genuinely runs on NT 5.2,
- CRT ownership is explicit,
- allocation ownership does not cross incompatible CRT boundaries accidentally,
- the Salix-facing ABI uses simple POD buffers/lengths/status codes,
- the dependency remains replaceable,
- the source/build recipe is reproducible inside the project.

## JSON strategy

Modern JSON libraries are candidates based on source-port cost, not merely their advertised minimum language standard.

For example, if a desired library depends heavily on C++11 syntax that VC7.1 cannot parse, Salix has three legitimate choices:

```text
A. port/fork the library for the legacy compiler
B. compile a compatible provider behind a narrow ABI boundary
C. choose a simpler parser whose source already fits the target better
```

The correct choice is whichever minimizes total technical debt while getting the service path functional.

Whatever provider wins the first implementation must sit behind a Salix JSON contract rather than leaking into application/service code.

## TLS strategy

OpenSSL is a first-class candidate precisely because we have the source code.

If the desired maintained OpenSSL branch does not build unchanged for VC7.1 / NT 5.2, the next step is to determine **why**:

- build-system assumptions,
- compiler-language features,
- CRT/runtime assumptions,
- threading primitives,
- entropy/platform code,
- assembly configuration,
- post-NT5 Win32 API imports,
- certificate-store integration,
- or another isolated compatibility layer.

We then patch or fork the smallest justified surface rather than retreating to an obsolete TLS stack merely because it happens to compile unchanged.

Other libraries such as wolfSSL remain useful comparison candidates, but the selection rule is not "which upstream still lists Server 2003". The selection rule is:

> Which source base gives SalixWeb32 the safest, smallest, most maintainable path to modern TLS on the P4 after we account for the port we are willing to own?

The winning provider must prove on the P4:

```text
DNS/connect
TLS 1.2 or newer as required by the target service
SNI
modern cipher negotiation
certificate chain verification
hostname verification
CA trust loading
clean failure diagnostics
repeatable connection/shutdown
```

Only after those pass should authentication tokens, cookies, or private service traffic be allowed through that path.

## HTTP strategy

Salix already owns a small backend-neutral request/response model and a first Winsock transport.

The preferred direction is to continue growing the Salix HTTP/network stack while borrowing mature lower-level pieces where they save time or reduce security risk.

A mature HTTP implementation may still be imported or ported if it clearly accelerates the first functional service path, but it must remain behind Salix contracts and must not dictate application architecture.

## Third-party and fork layout

Vendored dependencies should live under a clear boundary such as:

```text
third_party/
    openssl/
    json_provider/
    ...
```

A maintained Salix-specific port/fork may additionally carry project-owned build/compatibility material, for example:

```text
ports/
    openssl_nt5/
        patches/
        build/
        README.md
```

Each dependency should record:

- exact upstream project/version/commit,
- license text,
- local patches,
- why each patch exists,
- target build instructions,
- feature configuration,
- NT5-specific compatibility decisions,
- known upstream divergence,
- update/rebase procedure.

Do not silently copy library source into Salix-owned directories and lose provenance.

## Selection criteria

A dependency or fork is accepted when it performs well on the things that matter to this project:

```text
NT 5.2 x86 runtime viability
modern protocol/security capability
source quality and auditability
port/fork size
memory footprint on Pentium 4 / 2 GB
license compatibility
clear source provenance
ability to disable unused features
stable error reporting
reproducible builds
replaceable Salix-facing boundary
```

"Upstream does not support VC7.1" is a maintenance-cost signal, not a veto.

## Practical development order

The current priority is:

```text
1. qualify and, if necessary, port/fork a modern TLS source base
2. prove direct modern TLS from the P4
3. prove one real modern HTTPS request
4. integrate JSON behind a Salix contract
5. prove a small service request/response payload
6. establish authentication/session handling
7. connect MessageComposer/ConversationView to the service layer
8. add attachment upload/download
9. progressively internalize networking modules where it improves Salix
```

The already validated remote bridge remains useful as a test oracle, comparison backend, and recovery path while the direct native path is built.

## Guiding principle

> Use open source as source material, not as an architectural dependency.

SalixWeb32 can build, port, fork, patch, and eventually replace whatever pieces are necessary. The objective is to get modern communication working on the actual legacy target without sacrificing ownership of the architecture or waiting until every subsystem has been reinvented first.
