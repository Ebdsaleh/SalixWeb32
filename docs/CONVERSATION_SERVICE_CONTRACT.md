# Conversation Service Contract

This document defines the backend-neutral boundary between the native SalixWeb32
conversation application and a modern conversational service.

The contract exists so `ConversationView`, Markdown presentation, attachments, and
native Win32 interaction do not depend on one provider, one API, one browser runtime, or
one companion implementation.

## Boundary

```text
MessageComposer
    |
    v
MessageDraft
    |
    | application converts UI state
    v
ConversationRequest
    |
    v
ConversationServiceHost
    |
    v
ConversationServiceBackend
    |
    +-- PlaceholderConversationBackend
    +-- future RemoteConversationBackend
    +-- future API/service backend
    +-- future web-session compatibility backend
    `-- future local-model backend

semantic events
    |
    v
ConversationServiceHost
    |
    v
StatusView / application thread
    |
    v
ConversationView
```

The service layer does not receive framework widgets and does not return provider DOM
objects, HTML surfaces, Python objects, or browser-runtime types.

## Request model

`ConversationRequest` currently carries:

```text
text
attachment paths
```

This is deliberately small. Provider-specific request options should not be added to the
generic request unless they represent a reusable Salix conversation concept.

Attachment paths are semantic request input only. The current placeholder backend does
not transmit them. A real backend must define upload/security behavior before attachments
leave the legacy machine.

## Event model

A backend returns semantic events:

```text
request_started
message_started
text_delta
message_completed
request_failed
```

Every event carries a Salix request ID so the application can correlate a stream with the
request that created it.

The important rule is that the application receives **meaning**, not a remote UI:

```text
good:
    text_delta("Hello")
    message_completed

not the service contract:
    browser screenshot
    DOM node pointer
    provider-specific React object
    Playwright object
```

A future backend may internally use an API, browser session, translator, native TLS, or
another implementation. Those details stop at the backend boundary.

## Main-thread presentation discipline

The conversation contract follows the same rule as the web/network path:

> Background or companion work produces data; native presentation is applied on the
> application thread.

The current placeholder backend is local and synchronous internally, but it still emits
events through the same queue-style contract that a future asynchronous remote backend
will use.

`StatusView` drains those events from the ordinary application/render path and updates
`ConversationView`. No backend receives a pointer to the conversation widgets.

## Streaming presentation

The native application accumulates `text_delta` payloads as canonical response text.

The first non-empty delta creates one remote conversation message. Later deltas update
that same message in place:

```text
message_started
    |
text_delta("Hello")
    |
    +-- create Remote message: "Hello"
    |
text_delta(", world")
    |
    +-- update same Remote message: "Hello, world"
    |
message_completed
```

The message is reparsed through the existing native Markdown/block presentation path as
its canonical source grows. The provider/backend does not render Markdown itself.

This first implementation favors correctness and architectural proof. If very high-rate
real streams later make reparsing every delta wasteful on the Pentium 4, the application
can coalesce deltas before presentation without changing the backend contract.

## Placeholder backend

`PlaceholderConversationBackend` performs no network access.

It exists to validate:

- request handoff from the existing composer,
- generated request IDs,
- semantic event ordering,
- text-delta accumulation,
- in-place native remote-message updates,
- Markdown rendering of service text,
- runtime lifecycle composition.

Its response explicitly states that no external conversation service was contacted.

This is the conversation equivalent of the earlier placeholder Web backend: prove the
contract before provider credentials, authentication, or protocol-specific code is added.

## Security boundary

The current Browser Probe LAN transport remains plaintext and is **not** approved for
credentials, session cookies, private conversation traffic, or attachment uploads.

Creating the semantic conversation contract does not change that rule.

A real remote conversation backend must establish an appropriate credential/session
boundary before private content is transmitted.

## Provider independence

The long-term application-facing shape is:

```text
ConversationServiceBackend
    +-- RemoteConversationBackend
    +-- OpenAI/API adapter
    +-- ChatGPT web-session adapter
    +-- Gemini/service adapter
    `-- local/offline adapter
```

These are examples of backend families, not commitments that all will be implemented.

The native Conversation workspace should not need to know which one is active.

## Presentation-to-interaction relationship

The service contract carries canonical semantic content. Presentation policies may later
choose to lazily render, collapse, summarize, or bound very large responses on constrained
hardware, but the complete canonical content must remain available according to
`docs/PRESENTATION_INTERACTION_POLICY.md`.

## Target validation

On the real VC7.1 / Windows Server 2003 Pentium 4 target:

1. Clean/Rebuild `Debug | Win32` with zero errors and zero warnings.
2. Launch SalixWeb32 and open `Conversation`.
3. Confirm the hint names `Placeholder Conversation Backend` and reports the semantic
   contract ready.
4. Send the default `Hello from Pentium 4` message.
5. Confirm the local user message is retained.
6. Confirm one Remote message appears from the placeholder semantic event stream.
7. Confirm its Markdown bold heading renders through the normal native Markdown path.
8. Confirm the completed text states that no external conversation service was contacted.
9. Send another message after completion and confirm a second request is accepted.
10. Confirm Browser Probe and Runtime workspaces remain operational.
11. Confirm attachments still render locally and do not leave the machine through this
    placeholder backend.

The next tranche after this contract is target-green should introduce a remote
conversation backend/protocol proof without weakening the existing plaintext-LAN
credential restriction.
