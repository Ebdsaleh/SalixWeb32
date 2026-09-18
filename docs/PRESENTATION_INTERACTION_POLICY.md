# Presentation-to-Interaction Policy

SalixWeb32 targets hardware where presenting every byte of every object at full fidelity
all the time can be unnecessarily expensive. Resource-aware presentation is therefore a
normal part of the framework, but reduced presentation must never silently remove the
user's access to the complete content.

## Core invariant

> Any reduced presentation must provide a clear path to the complete experience or the
> complete underlying data.

Truncation, summaries, thumbnails, collapsed sections, bounded previews, lazy
presentation, and similar techniques are **presentation policies**. They are not
permission to discard canonical data.

The framework should therefore prefer:

```text
complete canonical data/state
        |
        v
bounded or task-appropriate presentation
        |
        v
clear user interaction
        |
        v
full fidelity / full data
```

This keeps the legacy UI responsive while preserving user capability.

## Presentation and interaction must match the content

The route to full fidelity should use an interaction that is natural for the content
type rather than applying one generic "expand" action everywhere.

Examples:

```text
large text
    preview/snippet
        -> Expand / Open / Copy full / Export full

image
    thumbnail
        -> Preview
        -> Open original/full image

code
    bounded code surface
        -> scroll
        -> Copy complete source

attachment
    chip / filename / metadata
        -> Preview when supported
        -> Open actual file

diagnostics
    concise status
        -> Export complete report

conversation history
    visible viewport
        -> scroll/access complete canonical history
```

The UI may choose the cheapest useful initial representation for the target hardware,
but the user must not become stranded behind that representation.

## Canonical-data rule

When a presentation is bounded or transformed:

- retain the complete canonical data when the feature contract requires it,
- keep presentation-only wrapping/truncation out of the canonical representation,
- make full-data operations use canonical data rather than reconstructing it from the
  visible preview,
- do not make a Copy/Export/Open action depend on first rendering the full payload,
- do not perform a second network request merely to recover data that is already cached,
- make it clear when full data is unavailable because the backend itself never supplied
  it.

This is particularly important on the Pentium 4 target: avoiding expensive rendering
must not become artificial product limitation.

## Current examples

### Browser Probe Raw

Browser Probe retains the complete captured Raw response but displays only a small
hard-wrapped preview. The dedicated Copy action returns the complete cached Raw section,
and the Browser Diagnostic Report exports the complete Summary, Headers, Raw, and
Extracted data without forcing the full payload through the renderer.

```text
complete Raw response
        |
        +-- 1 KiB hard-wrapped on-screen preview
        |
        +-- Copy -> complete Raw
        |
        `-- Export Browser Diagnostic Report -> complete Raw
```

### Image attachments

Conversation image attachments use bounded thumbnails as the document presentation.
The thumbnail is not treated as the image itself.

```text
original image
        |
        +-- bounded thumbnail in conversation
        |
        +-- Preview -> internal image viewer
        |
        `-- Open -> original file
```

### Conversation and code

Conversation wrapping is presentation-only; canonical message text remains unchanged.
Code blocks preserve complete logical source lines and provide scrolling/copy behavior
instead of rewriting or truncating the source to fit the viewport.

## Failure mode to avoid

The following is not acceptable unless the product explicitly states that the backend
itself only possesses the reduced data:

```text
full content
    -> truncate for display
    -> discard the remainder
    -> no expand/open/copy/export path
```

Likewise, a thumbnail without a Preview/Open path, a summary with no way to reach the
full content, or a clipped diagnostic field with no complete export violates this
policy.

## Relationship to performance work

This policy does not excuse inefficient rendering. SalixWeb32 should still optimize the
generic renderer, layout, clipboard, and data paths.

The distinction is:

```text
performance optimization:
    reduce unnecessary work while preserving capability

product limitation:
    remove data/capability because presentation is expensive
```

SalixWeb32 should choose the first.

## Design shorthand

The project can summarize this policy as:

```text
Preserve everything required by the contract.
Render intelligently.
Reveal progressively.
Never strand the user behind the summary.
```
