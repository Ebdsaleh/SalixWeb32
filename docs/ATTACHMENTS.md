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

## Composer attachment queue

Selected files now become visible first-class draft items before the message is sent.

The composer displays a compact attachment tray between the formatting toolbar and the multiline text input. Each queued file is represented by an `AttachmentChip` containing:

```text
[file-name.ext] [x]
```

The `x` button removes only that attachment from the pending draft. Removing the final attachment restores the composer to its normal no-attachment layout and returns empty-submit behavior to its normal state.

The tray remains a fixed single row so the existing conversation/composer split does not jump vertically while files are added. When all chips fit, they are shown directly. When the row overflows, small previous/next navigation buttons page through the queued attachments without allowing child controls to draw outside the composer bounds.

The composer continues to keep the canonical pending paths in its existing attachment vector. `AttachmentTray` is presentation/control state only; `MessageDraft` is still built from the canonical composer attachment collection at submit time.

Removal is deliberately deferred until the chip's mouse event has completely unwound. This avoids deleting the clicked chip while one of its own button callbacks is still active.

## Conversation presentation

Generic files continue to have a readable system representation.

Image files additionally receive an inline thumbnail below their system reference:

```text
System: renderware2.jpg

[ aspect-ratio-preserving thumbnail ]
```

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

Structured clipboard serialization and service/backend upload packaging remain follow-on work and are not claimed by this tranche.

## Win32 image services

The current platform implementation uses:

- GDI+ for decoding common image formats and producing bounded thumbnail pixels,
- GDI for drawing the already-decoded `RasterImage` in the normal component renderer,
- `ShellExecute` for `Open`,
- a lightweight native Win32 preview window for `Preview`.

These implementation details stay below the generic `DesktopServices`, `RasterImage`, and `ImageView` boundaries.

## Current Pentium 4 validation status

The September 16, 2026 Pentium 4 / Windows Server 2003 SP2 pass visibly confirmed the core image path:

- the native application menu is present,
- a JPEG can be attached and sent,
- the conversation renders the filename reference plus inline thumbnail,
- the internal SalixWeb32 Preview window opens the image,
- Open launches the system's Windows Picture and Fax Viewer for the same file,
- the image remains visibly aspect-ratio-correct in the conversation and preview surfaces.

Those observations validate the principal decode/render/Preview/Open path on Server 2003. The complete selection/copy edge cases, generic-file behavior, composer-chip tranche, and MiniXP repeat pass remain separate checklist items until explicitly exercised.

## Target validation checklist

1. Attach a JPEG or PNG from the local machine and confirm a filename chip appears in the composer before send.
2. Attach several files and confirm each receives an independent remove `x` control.
3. Remove one middle attachment and confirm the others remain queued in their original order.
4. Remove the final attachment and confirm the attachment tray disappears cleanly.
5. Queue enough files to overflow the row and confirm the previous/next tray controls expose every attachment without drawing outside the composer.
6. Send a draft containing attachments and confirm the tray clears after submission.
7. Confirm the conversation displays `System: filename.ext` and an inline thumbnail for an image attachment.
8. Confirm a landscape image, portrait image, and small image retain correct aspect ratio.
9. Confirm no thumbnail exceeds the 400x400 first-pass bound.
10. Resize the application narrower than a 400-pixel thumbnail and confirm the image remains inside the conversation viewport without distortion.
11. Right-click the thumbnail and verify `Preview`, `Open`, and `Copy Reference`.
12. Confirm Preview opens internally and preserves aspect ratio while its window is resized.
13. Confirm Open launches the machine's default application for that file type.
14. Drag-select conversation text across the image and copy it.
15. Paste into a plain text destination and confirm the image position becomes exactly `System: filename.ext` in source order.
16. Begin a selection on the image and drag upward/downward; confirm the image surrogate is selected atomically.
17. Attach a non-image file and confirm it still produces a useful system filename reference without a broken image placeholder.
18. Repeat the completed smoke pass under MiniXP after Server 2003 succeeds.
