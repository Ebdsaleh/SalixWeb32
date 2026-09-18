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

Structured clipboard serialization and service/backend upload packaging remain follow-on work and are not claimed by this tranche.

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

The complete image-selection/plain-copy edge cases, generic-file behavior, and latest MiniXP repeat pass remain separate checklist items until explicitly exercised.

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
21. Attach a non-image file and confirm it still produces a useful system filename reference without a broken image placeholder.
22. Repeat the completed smoke pass under MiniXP after Server 2003 succeeds.
