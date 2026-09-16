# Attachments

This document records the first SalixWeb32 first-class attachment tranche.

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

Composer-inline attachment objects, structured clipboard serialization, and service/backend upload packaging are follow-on work and are not claimed by this tranche.

## Win32 image services

The current platform implementation uses:

- GDI+ for decoding common image formats and producing bounded thumbnail pixels,
- GDI for drawing the already-decoded `RasterImage` in the normal component renderer,
- `ShellExecute` for `Open`,
- a lightweight native Win32 preview window for `Preview`.

These implementation details stay below the generic `DesktopServices`, `RasterImage`, and `ImageView` boundaries.

## Target validation checklist

This tranche is not target-validated until rebuilt and exercised on the real Pentium 4.

1. Attach a JPEG or PNG from the local machine and send the draft.
2. Confirm the conversation displays `System: filename.ext` and an inline thumbnail.
3. Confirm a landscape image, portrait image, and small image retain correct aspect ratio.
4. Confirm no thumbnail exceeds the 400x400 first-pass bound.
5. Resize the application narrower than a 400-pixel thumbnail and confirm the image remains inside the conversation viewport without distortion.
6. Right-click the thumbnail and verify `Preview`, `Open`, and `Copy Reference`.
7. Confirm Preview opens internally and preserves aspect ratio while its window is resized.
8. Confirm Open launches the machine's default application for that file type.
9. Drag-select conversation text across the image and copy it.
10. Paste into a plain text destination and confirm the image position becomes exactly `System: filename.ext` in source order.
11. Begin a selection on the image and drag upward/downward; confirm the image surrogate is selected atomically.
12. Attach a non-image file and confirm it still produces a useful system filename reference without a broken image placeholder.
13. Repeat the smoke pass under Windows Server 2003 SP2 and MiniXP.
