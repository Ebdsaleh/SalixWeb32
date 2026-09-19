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
    +-- RemoteConversationBackend
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

Attachment paths are semantic request input only. Neither the local placeholder nor the
initial remote probe transmits them. A real backend must define upload/security behavior
before attachments leave the legacy machine.

## Security profile and dispatch boundary

Every conversation backend now exposes a machine-readable
`ConversationSecurityProfile`.

The profile separates two decisions:

```text
dispatch mode
    blocked
    probe-only
    content

transport security
    none
    local-process
    plaintext
    trusted-lan
    authenticated-encrypted
```

It also declares whether text, attachments, credentials, and session state are permitted
for a real content request.

`ConversationServiceHost` is the enforcement point. A content request is handed to a
backend only when:

- the backend declares `dispatch = content`,
- the transport is an allowed content transport for that backend,
- and every data class present in the request is explicitly allowed.

Ordinary `plaintext` remains ineligible for content dispatch. The explicit
`trusted-lan` transport exists only for narrow development scenarios where the user has
chosen that local topology and the profile still enumerates which data classes may cross.

Probe-only dispatch is physically separate from content dispatch:

```text
ConversationRequest
    |
    v
ConversationServiceHost
    |
    +-- probe-only -> submit_probe(request_id)
    |                 ConversationRequest is NOT passed
    |
    `-- content    -> security profile gate
                       |
                       `-> submit_request(request, request_id)
```

The local placeholder backend is content/local-process. The current remote browser-relay
backend is content/trusted-lan with text enabled and attachments, credentials, and
session state disabled. The older probe-only/plaintext method remains available as a
content-free diagnostic endpoint.

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

The provider/backend does not render Markdown itself. Salix accumulates canonical
`text_delta` content on the application thread and may coalesce presentation work across
all events already available in one backend update.

This behavior is now validated on the real Pentium 4 for the completed-response relay:
27 semantic deltas were preserved but drained into one native Conversation presentation
update. The backend contract therefore retains incremental semantics without requiring
one expensive Markdown/layout rebuild per synthetic delta. Future true streaming remains
incremental because only events that have actually arrived can be drained in a given
update.

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

## Remote semantic probe and browser relay

When normal bridge configuration selects the remote web backend, the composition root
also selects `RemoteConversationBackend`.

It uses a dedicated `Win32HttpTransport` + `Win32NetworkRequestExecutor` pair so a
Browser Probe request and a Conversation request do not share one single-flight worker.

### Content-free probe

The previously validated diagnostic request remains available at:

```text
POST /v1/conversation/probe
```

It contains only the generated request ID and fixed zero-forwarding metadata. It is
useful for regression-testing the protocol without sending a draft.

### LibreWolf browser relay

The current functional baseline uses:

```text
POST /v1/conversation/message
```

with a length-framed request:

```text
SALIX-CONVERSATION/1
mode=browser_relay
request_id=<Salix-generated ID>
text_forwarded=1
attachments_forwarded=0
credentials_forwarded=0
session_forwarded=0
text_len=<UTF-8 byte count>

<message text>
```

`salix_bridge.py` is still the P4-facing listener. It forwards the message over
localhost to `salix_chat_session.py`, which is now a broker rather than a browser
automation process.

A small WebExtension runs inside the user's normal LibreWolf process. The user
authenticates normally in that browser and opens the desired ChatGPT conversation there.
The extension receives message text from the localhost broker, enters it into the visible
ChatGPT composer, waits for the rendered assistant message to stabilize, and returns that
rendered text to the broker.

The broker/extension path does not expose credentials, cookies, or browser session
storage.

The bridge then frames:

```text
request_started
message_started
text_delta ...
message_completed
```

for the existing native Conversation path.

The current HTTP transport still receives the complete bridge response before
`RemoteConversationBackend` releases at most one semantic event per application update.
This gives native incremental presentation but is not yet byte-streaming transport from
the browser while generation is in progress.

### Capability negotiation

Remote Conversation readiness is not inferred from generic bridge reachability. During
initialization the backend queues:

```text
GET /v1/health
```

and requires:

```text
status=ok
conversation_relay=enabled
conversation_protocol=SALIX-CONVERSATION/1
conversation_mode=browser_relay
conversation_text_forwarding=enabled
conversation_attachment_forwarding=disabled
conversation_credential_forwarding=disabled
conversation_session_forwarding=disabled
conversation_transport_security=trusted_lan
conversation_browser_session=ready
```

The final field becomes `ready` only when the localhost broker is reachable, the
WebExtension has sent a recent heartbeat, and the ChatGPT composer is visible. Until then
the remote backend remains unavailable for content dispatch.

## Security boundary

The browser-relay baseline deliberately sends message text over the user's trusted
development LAN. The transport is therefore labelled `trusted_lan`; it is not presented
as authenticated/encrypted.

Only text is enabled in the current remote security profile. The following remain
disabled:

- attachment paths and file contents,
- ChatGPT credentials or MFA material,
- authorization headers,
- browser cookies,
- browser/session storage.

Authentication and service-session ownership stay inside the visible LibreWolf process
on the modern machine.

The older `/v1/conversation/probe` path remains stricter: it sends no request content at
all and keeps its original probe-only/plaintext framing for regression testing.

A future native or encrypted transport can use the same
`ConversationServiceBackend`/event contract without changing the Conversation UI.

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

The local placeholder contract is now target-green. The next target pass validates the
remote `SALIX-CONVERSATION/1` probe between the P4 and companion while verifying that
the companion log receives `POST /v1/conversation/probe` and no draft/attachment data.
