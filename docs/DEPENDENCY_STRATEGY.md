# Dependency Strategy

SalixWeb32 is intended to become increasingly self-contained, but **functional modern communication comes before reimplementing every subsystem in-house**.

The working rule is:

```text
bootstrap with a proven dependency
        -> hide it behind a Salix contract
        -> validate it on the real Pentium 4
        -> ship useful functionality
        -> replace selected internals later when doing so has a clear benefit
```

This keeps the project moving without allowing third-party APIs to become permanent application architecture by accident.

## Architecture rule

Application/framework code must depend on Salix-owned interfaces rather than a third-party library directly.

Examples:

```text
application
    -> JsonDocument / JsonReader
        -> temporary JSON provider

web/network
    -> NetworkTransport / HttpClient
        -> temporary HTTP/TLS provider

web/security
    -> TlsSession / CertificateVerifier
        -> temporary TLS provider
```

If a dependency is replaced later, the application-facing contract should remain stable.

Do not scatter `openssl/*`, JSON-provider headers, curl handles, or another provider's concrete objects throughout application code.

## What should be borrowed first

The first bootstrap candidates are the expensive or security-sensitive pieces whose reimplementation would delay the actual SalixWeb32 goal:

- TLS and cryptographic primitives,
- X.509/certificate parsing and verification,
- JSON parsing/serialization,
- compression/decoding where modern services require it,
- possibly a mature HTTP implementation if a target-compatible candidate proves substantially cheaper than extending Salix's own HTTP layer.

The project can progressively replace transport, HTTP, URL, cache, cookie, document, and other modules with Salix-owned implementations after the complete service path works.

## Cryptography exception

"Eventually in-house" does **not** mean inventing cryptographic algorithms.

Salix may eventually own the surrounding TLS/session/certificate architecture, adapters, policy, storage, diagnostics, and protocol integration, but cryptographic primitives should continue to come from a well-reviewed cryptographic implementation.

Do not implement AES, ChaCha20, RSA, ECC, hashing, signature verification, or random-number generation algorithms from scratch for production use.

## Legacy compiler rule

Every dependency has two separate compatibility questions:

1. Can its source be built with the SalixWeb32 toolchain or another deliberately selected compatible toolchain?
2. Does the resulting x86 binary actually run correctly on Windows Server 2003 SP2 / NT 5.2?

A library is not accepted merely because it supports Win32 in general.

The real Pentium 4 remains authoritative.

If a dependency cannot sensibly compile inside the VC7.1 solution, a narrow C ABI DLL/static-library boundary may be used, provided:

- the produced binary genuinely runs on NT 5.2,
- CRT ownership is explicit,
- allocation ownership does not cross incompatible CRT boundaries accidentally,
- the Salix-facing ABI uses simple POD buffers/lengths/status codes,
- the dependency remains replaceable.

## JSON bootstrap note

`nlohmann/json` is an excellent modern C++ library but is **not a direct SalixWeb32 candidate under VC7.1** because current nlohmann/json targets C++11 and later.

For the legacy client, prefer a provider that can survive the actual compiler/runtime target. Current candidates include:

- a small portable ANSI-C JSON parser such as `json-parser`,
- RapidJSON only if a real VC7.1 build proves its nominal C++03 compatibility is sufficient,
- a later Salix-owned parser once service functionality is already working.

Whatever provider wins the bootstrap test should sit behind a Salix JSON contract rather than leaking into application/service code.

## TLS bootstrap note

OpenSSL is a valid candidate family, but version selection cannot be based on name recognition alone.

Modern OpenSSL releases target substantially newer Windows/toolchain baselines than VC7.1/Server 2003. Old OpenSSL branches are useful compatibility references but must not become the production authentication path merely because they build: unsupported/EOL crypto libraries are not acceptable for credentials or sensitive sessions.

The TLS qualification tranche should therefore compare maintained candidates and build strategies, including:

- current OpenSSL if a compatible NT5.2 build can be produced without weakening security,
- wolfSSL because its C/embedded-oriented design and Win32 portability may fit the legacy target,
- other maintained TLS libraries only if they satisfy the same target/security requirements.

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

For the blitz path we should not insist on completing a perfect browser-grade HTTP stack before testing real HTTPS. Two approaches may be evaluated in parallel:

```text
A. Salix HTTP framing + qualified TLS provider
B. target-compatible mature HTTP/TLS dependency behind Salix HttpClient
```

Whichever reaches a safe, maintainable ChatGPT/service proof first can bootstrap the product. The other path can continue later.

Current upstream libcurl releases have moved beyond the Server 2003 / VC7.1 baseline, so libcurl must be treated as a version/toolchain qualification problem rather than assumed to be drop-in.

## Third-party source layout

Vendored dependencies should live under a clear boundary such as:

```text
third_party/
    json_parser/
    tls_provider/
    ...
```

Each dependency should carry:

- exact upstream project/version/commit,
- license text,
- local patches kept small and documented,
- target build instructions,
- feature configuration,
- reason for inclusion,
- replacement status/plan where relevant.

Do not silently copy library source into Salix-owned directories.

## Selection criteria

A bootstrap dependency is accepted only when it scores well on the things that matter to this project:

```text
NT 5.2 x86 runtime viability
security maintenance
small/controllable feature set
C or VC7.1-compatible interface where possible
memory footprint on Pentium 4 / 2 GB
license compatibility
clear source provenance
ability to disable unused algorithms/features
stable error reporting
replaceable Salix-facing boundary
```

"Fewest dependencies" is useful, but not if it forces Salix to reinvent security-critical code prematurely.

## Practical development order

The current priority is:

```text
1. prove direct modern TLS on the P4
2. prove one real modern HTTPS request
3. add JSON provider behind a Salix contract
4. prove a small service request/response payload
5. establish authentication/session handling
6. connect MessageComposer/ConversationView to the service layer
7. add attachment upload/download
8. only then start replacing bootstrap dependencies where useful
```

The already validated remote bridge remains available as a test oracle and fallback while the native path is built.

## Guiding principle

> Borrow implementation to get functional; own the architecture from day one.

That lets SalixWeb32 become increasingly self-contained without turning "eventually in-house" into a blocker for the first usable modern-communications milestone.
