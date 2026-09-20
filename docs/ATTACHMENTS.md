# Attachments

This document records the SalixWeb32 first-class attachment work.

## Design goal

Attachments are semantic conversation content, not filenames painted as incidental status strings.

The current model introduces `app/Attachment` metadata and keeps attachment information in `MessageDraft`. The conversation can therefore choose an attachment-specific presentation while preserving a useful plain-text representation.

The guiding rule is:

> Rich presentation may be platform-specific, but every rich conversation object must have a useful plain-text surrogate and a stable semantic position in the conversation stream.

## Attachment metadata

The first-pass `Attachment` model records:

```text
path
file name
kind
    generic file
    image file
```

Image classification is currently extension-based for formats supported by the Win32 image path (`bmp`, `gif`, `jpg/jpeg`, `png`, `tif/tiff`). The model is intentionally independent of GDI+; platform image decoding belongs below `DesktopServices`.

## Native multi-file selection

One press of the composer `+` button opens the Win32 file picker once and may return multiple attachment paths in the same selection operation.

The Win32 provider deliberately uses the Explorer-style common dialog with both:

```text
OFN_EXPLORER
OFN_ALLOWMULTISELECT
```

so ordinary Windows selection gestures remain native rather than being reimplemented by SalixWeb32:

```text
Ctrl+Click   -> add/remove individual non-contiguous files
Shift+Click  -> select a contiguous range
```

Every path returned by that one dialog invocation is forwarded together to the composer and becomes an independent pending attachment chip. The application does not impose a one-file-per-`+` restriction.

## Composer attachment queue

Selected files become visible first-class draft items before the message is sent.

The composer displays a compact attachment tray between the formatting toolbar and the multiline text input. Each queued file is represented by an `AttachmentChip` containing:

```text
[file-name.ext] [x]
```

The `x` button removes only that attachment from the pending draft. Removing the final attachment restores the composer to its normal no-attachment layout and returns empty-submit behavior to its normal state.

The tray remains a fixed single row so the existing conversation/composer split does not jump vertically while files are added. When all chips fit, they are shown directly. When the row overflows, small `<` and `>` controls expose the hidden attachments without allowing child controls to draw outside the composer bounds.

The arrows describe the direction the visible chip strip moves:

```text
<  -> shift visible chips left, revealing later attachments
>  -> shift visible chips right, revealing earlier attachments
```

This is intentionally based on visible movement rather than abstract previous/next collection traversal.

The current arrow presentation is functionally accepted but not considered final UX. The target pass showed that the interaction still feels visually unusual even though the requested direction is now correct. Replacing or redesigning the overflow presentation is explicitly deferred until the later UI-polish pass, after the backend path is working.

The composer continues to keep the canonical pending paths in its existing attachment vector. `AttachmentTray` is presentation/control state only; `MessageDraft` is still built from the canonical composer attachment collection at submit time.

Removal is deliberately deferred until the chip's mouse event has completely unwound. This avoids deleting the clicked chip while one of its own button callbacks is still active.

## Presentation-to-interaction rule

Attachments follow the same progressive-disclosure invariant as the rest of SalixWeb32:
the lightweight conversation representation is not the endpoint when fuller content is
available.

For images:

```text
original image
    -> bounded conversation thumbnail
    -> Preview in SalixWeb32
    -> Open original file when requested
```

For generic files, the chip/filename/metadata representation must retain a clear Open or
equivalent full-data path when the platform/backend can provide it.

See `docs/PRESENTATION_INTERACTION_POLICY.md`.

## Conversation presentation

Generic files keep a readable conversation representation with the semantic sender role.

```text
You: notes.txt
Remote: result.txt
```

Image files additionally receive an inline thumbnail below their role-bearing reference:

```text
You: renderware2.jpg

[ aspect-ratio-preserving thumbnail ]
```

Files returned by the remote conversation backend use the Remote role rather than being
presented as system status.

The first-pass thumbnail bounding box is:

```text
maximum width  = 400 px
maximum height = 400 px
```

Images smaller than the bounding box are not deliberately enlarged. Images larger than the bound are scaled down while preserving aspect ratio. If the conversation viewport becomes narrower than the decoded thumbnail, display layout scales the thumbnail down again while preserving aspect ratio.

The thumbnail pixels are stored in backend-neutral `RasterImage` data and displayed by framework `ImageView`. The Win32 renderer uses ordinary GDI to present those pixels. GDI+ is confined to the Win32 desktop-services layer for image decoding/scaling and the standalone preview window.

## Attachment actions

Right-clicking an image thumbnail exposes attachment-specific actions:

- `Preview` opens an internal SalixWeb32 image-preview window.
- `Open` asks Windows to open the original file using the machine's configured default application.
- `Copy Reference` copies the plain contextual surrogate described below.

`Preview` and `Open` are intentionally distinct operations.

## Image preview zoom

The internal Win32 preview window supports mouse-wheel zoom while the preview window is active.

The initial state is the existing aspect-ratio-preserving fit-to-window presentation. Wheel input applies a zoom multiplier to that fitted size:

```text
Wheel up    -> zoom in
Wheel down  -> zoom out
```

Each wheel notch changes the scale by a factor of `1.25`. The first-pass zoom range is clamped to:

```text
minimum: 25% of fitted size
maximum: 800% of fitted size
```

The image remains centered and aspect ratio is preserved at every zoom level. Resizing the preview recalculates the fit-to-window baseline while retaining the current zoom multiplier. When zoomed beyond the client area, normal Win32 clipping shows the centered portion of the image; panning is a future enhancement rather than being coupled into this first zoom pass.

## Plain selection and copy

An image thumbnail is treated as an atomic semantic object when document-wide drag selection crosses it.

The normal/plain clipboard representation of the attachment is:

```text
System: filename.ext
```

For example, the visual conversation:

```text
You: Hey, check this out!

System: renderware2.jpg
[thumbnail]

Remote: That is the image I meant.
```

must produce a plain copied selection in the same stream order:

```text
You: Hey, check this out!
System: renderware2.jpg
Remote: That is the image I meant.
```

The thumbnail itself is not serialized as meaningless binary data into `text/plain`. Its textual surrogate anchors the receiving human or LLM to the image's identity and position.

Selection that begins or ends on the image forces that attachment's surrogate to be selected as a whole rather than allowing a half-selected image object.

## Future structured clipboard pass

The current tranche intentionally implements the plain/default representation first.

A later tranche will add a Salix-specific structured clipboard representation so that:

```text
Ctrl+V
    -> normal/plain text representation

Ctrl+Shift+V
    -> keep formatting + attachment semantics
```

The intended rich message model is ordered content, conceptually:

```text
TextSegment
AttachmentSegment
TextSegment
AttachmentSegment
...
```

rather than a body string plus an unrelated unordered attachment bag. This will allow an outgoing message to retain semantics equivalent to:

```text
Hey, check this out! <attachment: renderware2.jpg> What do you think?
```

without requiring XML-like markup to become the internal storage model.

Structured clipboard serialization remains follow-on work. Service/backend file
packaging is now implemented as a separate bounded relay tranche described below.

## Bounded Conversation file relay — dev candidate

The first real file-transport candidate extends the existing semantic attachment model
through the provider-neutral Conversation backend rather than teaching the application
about browser DOM upload controls.

Current path:

```text
MessageDraft / ConversationRequest
        |
        v
RemoteConversationBackend
        |
        | SALIX-CONVERSATION/1 bounded attachment framing
        v
salix_bridge.py
        |
        | localhost JSON/base64
        v
salix_chat_session.py
        |
        v
normal LibreWolf WebExtension
        |
        +-- outgoing files -> visible ChatGPT composer
        |
        `-- assistant-returned files -> semantic attachment events
                |
                v
ConversationView
```

The first candidate accepts text, image, and generic files. Known text/source extensions
receive an appropriate MIME type where possible; unknown files remain
`application/octet-stream`.

The transport is deliberately bounded for target validation:

```text
maximum attachments per request: 8
maximum bytes per attachment:     2 MB
maximum total attachment bytes:   4 MB
```

These are relay limits, not framework-model limits.

The remote security profile explicitly changes from `attachments no` to
`attachments yes` only when the companion advertises the matching capability.
Credentials and browser session state remain disabled. The content-free probe remains
attachment-free.

Assistant-returned file bytes are converted into semantic `attachment` events by
`RemoteConversationBackend`. The application then stores them beneath the selected
Salix data root:

```text
Standard:
    %APPDATA%\SalixWeb32\Received

Portable:
    <executable_root>\Received
```

Received names are sanitized and collisions receive a unique filename instead of
overwriting an existing file. Conversation presentation keeps sender ownership:

```text
outgoing attachment -> You:
returned attachment -> Remote:
```

Image attachments continue to use the existing thumbnail/Preview/Open behavior. Generic
and text files retain an Open path through the platform `DesktopServices` provider.

The active reverse-file candidate uses LibreWolf relay extension `0.2.7`. Because it is
still loaded as a temporary development extension, it must be reloaded after pulling this
candidate before file-relay validation.

Version `0.2.1` remains the validated outgoing-file baseline: it performs a verified
Send-control handoff after file upload so an injected attachment is not left waiting in
the browser composer.

Version `0.2.2` adds returned-file capture for ChatGPT `sandbox:` attachment links.
Those links are not directly fetchable by the content script. The extension now lets
normal LibreWolf initiate the authenticated download, observes the completed browser
download through the WebExtension downloads API, and passes only the completed local
file path to the localhost broker. The broker reads and bounds the file bytes before
returning them through the existing semantic attachment pipeline. Browser credentials,
cookies, and session material still never enter the Salix protocol. Temporary modern-side
download files are removed after the broker accepts the relay result.

The first `0.2.2` target retest still produced `files 0`. Timing/log evidence showed no
download-capture timeout, which means the returned file control was not discovered at
all. ChatGPT can render the file card outside the narrow
`[data-message-author-role='assistant']` content node.

Version `0.2.3` therefore scans the full assistant turn container, accepts file-like
anchors/buttons/data-link controls, briefly waits for a file control when the response
text mentions a filename, and returns explicit capture telemetry to the localhost broker.
This retest remains pending.

The `0.2.3` target retest proved discovery progressed further: ChatGPT's returned file
card was opened automatically and the browser subsequently raised its native **Save As**
dialog. That is not acceptable relay behavior; the user must not manually choose a
download path.

Version `0.2.4` separates preview controls from real Download controls. It prefers
explicit download actions, can open the file preview only when needed to reveal the
Download control, and hands any browser-created interactive download to a managed
WebExtension download with `saveAs:false`. The original interactive download is
cancelled/erased and the managed file is written only to the temporary relay download
area before broker packaging/cleanup.

The `0.2.4` target retest still produced LibreWolf's native Save As dialog. The key
mistake was architectural: the extension still clicked ChatGPT's Download control before
attempting the managed handoff, so the interactive browser download had already begun.

Version `0.2.5` no longer clicks the Download control. It extracts the HTTP(S) URL from
the preview Download control (including enclosing/nested anchors and data URL attributes)
and asks the background extension to download that URL directly with `saveAs:false`.
No filename is supplied to `downloads.download()`; the browser response determines the
temporary local filename. If no usable HTTP(S) URL is exposed, the relay reports telemetry
instead of opening another Save dialog. The extension also attempts to close the preview
after capture so the visible browser returns to the conversation.

The `0.2.5` target retest confirmed the Save As regression was removed, but the returned
file still did not reach Salix. Capture telemetry showed two file candidates, one preview
open, one explicit Download control, and **zero** DOM-exposed HTTP(S) download URLs. The
P4 correctly remained at `files 0`; the absence of the `Received` directory is expected
because Salix creates that directory lazily only when a real semantic attachment event is
stored.

Version `0.2.6` adds a bounded Firefox `webRequest` interception fallback for this
JavaScript-only Download control. The extension arms interception only for an active
returned-file capture, clicks the explicit Download control, cancels the actual
ChatGPT/oaiusercontent file-content request before the browser can present Save As, then
replays that captured signed URL through `downloads.download(..., saveAs:false)`. The
listener is limited to ChatGPT and oaiusercontent HTTPS hosts and remains inactive outside
the bounded capture window.

The `0.2.6` target retest crossed the hard reverse-transport boundary. The browser
interceptor captured the file-content request without a Save As dialog, the broker
reported two returned attachments, Salix reported `files 2`, and the P4 automatically
created its application-owned `Received` directory. Both resulting files contained the
exact expected 425-byte test payload.

The remaining defects were metadata/deduplication only:

- the signed content endpoint exposed the local leaf name `content`, so the semantic
  `.txt` filename/extension was lost,
- two DOM controls representing the same assistant file were independently captured,
  creating `content` and `content(1)`,
- the missing `.txt` extension prevented useful browser/native Open behavior.

Version `0.2.7` carries the semantic filename discovered from the assistant file card
(or response filename fallback) through preview/download capture, deduplicates browser
controls by semantic attachment name, suppresses same-name/same-payload duplicates in the
localhost broker as a defense-in-depth check, and restores MIME inference from the
semantic filename when the temporary browser download is generically named.

The `0.2.7` target retest is green. The companion collected exactly one returned
attachment from two browser candidates, skipped the duplicate candidate, the bridge
returned `attachments=1`, and the P4 reported `files 1`. Salix stored exactly one
`SalixWeb32_reverse_filename_0_2_7.txt` file in the application-owned `Received`
directory, Windows recognized it as a Text Document, and Open launched Notepad.

The basic bidirectional text-file relay is therefore validated end-to-end on the real P4.
The existing bounds remain intentional: at most 8 attachments, 2 MB per file, 4 MB total.
A later attempt to send a larger evidence bundle in one message was correctly rejected by
the client at the 8-attachment boundary; this is expected policy enforcement rather than
a transfer failure.

## Win32 image services

The current platform implementation uses:

- GDI+ for decoding common image formats and producing bounded thumbnail pixels,
- GDI for drawing the already-decoded `RasterImage` in the normal component renderer,
- `ShellExecute` for `Open`,
- a lightweight native Win32 preview window for `Preview`.

These implementation details stay below the generic `DesktopServices`, `RasterImage`, and `ImageView` boundaries.

## Current Pentium 4 validation status

The September 16, 2026 Pentium 4 / Windows Server 2003 SP2 passes confirmed the core image and composer-attachment paths.

Observed green on the target:

- the native application menu is present,
- JPEG attachment and send path works,
- conversation filename reference plus inline thumbnail renders correctly,
- internal SalixWeb32 Preview opens the image,
- Open launches Windows Picture and Fax Viewer for the same file,
- image aspect ratio remains correct in conversation and preview surfaces,
- one `+` dialog can queue multiple files using normal Windows multi-selection,
- pending files appear as independent removable composer chips,
- overflow navigation remains bounded inside the composer,
- the revised `<` / `>` movement direction functions as specified,
- Preview mouse-wheel zoom builds and works after the VC7.1 `WM_MOUSEWHEEL` compatibility guard was restored.

The overflow arrows remain a known **presentation-polish** item rather than a functional blocker. Their replacement/design should be revisited after the backend is working rather than expanding this UI tranche further.

The complete image-selection/plain-copy edge cases remain separate checklist items until
explicitly exercised. Native P4 outgoing file relay and remote-to-P4 returned text-file
relay are now both validated end-to-end through the visible-browser companion path. MiniXP compatibility is deferred until the Server 2003
feature set is complete and the MiniXP environment is usable again.

## Target validation checklist

1. Press `+` once, use Ctrl+Click to select several non-contiguous files, and confirm all selected files appear as independent composer chips from that one dialog invocation.
2. Press `+` once, use Shift+Click to select a contiguous file range, and confirm the complete selected range is queued.
3. Remove one middle attachment and confirm the others remain queued in their original order.
4. Remove the final attachment and confirm the attachment tray disappears cleanly.
5. Queue enough files to overflow the row and confirm `<` visibly shifts the chip strip left to reveal later attachments.
6. Confirm `>` visibly shifts the chip strip right to reveal earlier attachments.
7. Confirm neither navigation direction allows chips to draw outside the composer.
8. Send a draft containing attachments and confirm the tray clears after submission.
9. Confirm the conversation displays `System: filename.ext` and an inline thumbnail for an image attachment.
10. Confirm a landscape image, portrait image, and small image retain correct aspect ratio.
11. Confirm no thumbnail exceeds the 400x400 first-pass bound.
12. Resize the application narrower than a 400-pixel thumbnail and confirm the image remains inside the conversation viewport without distortion.
13. Right-click the thumbnail and verify `Preview`, `Open`, and `Copy Reference`.
14. In Preview, wheel upward several notches and confirm the image grows while remaining centered and undistorted.
15. Wheel downward and confirm the image shrinks, respects the lower clamp, and remains aspect-ratio-correct.
16. Resize the Preview window after changing zoom and confirm the current zoom multiplier survives the resize.
17. Confirm Open launches the machine's default application for that file type.
18. Drag-select conversation text across the image and copy it.
19. Paste into a plain text destination and confirm the image position becomes exactly `System: filename.ext` in source order.
20. Begin a selection on the image and drag upward/downward; confirm the image surrogate is selected atomically.
21. Attach a non-image text file and confirm it produces a useful `You:` filename
    reference without a broken image placeholder.
22. With the attachment-relay candidate active, send a small text file from the P4 and
    confirm it appears in the visible ChatGPT composer/thread.
23. Confirm the diagnostic security profile reports attachments enabled while credentials
    and session forwarding remain disabled.
24. Have the remote side return a small text file and confirm it appears in the native
    conversation as `Remote: filename.ext`.
25. Confirm the received file is stored beneath the active Salix `Received` directory,
    opens through the platform default application, and does not overwrite an existing
    same-named file.
26. Send/receive a small image and confirm the existing thumbnail/Preview/Open behavior
    still works for transported files.
27. Confirm files above the per-file or total relay limits fail cleanly rather than being
    partially forwarded.
28. MiniXP is not an acceptance target for this tranche.
