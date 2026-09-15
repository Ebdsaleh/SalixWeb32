# Modern Web Platform Capability Inventory

This is a planning inventory, not a promise that every item will be implemented.

## Transport and resource loading

- DNS
- TCP sockets
- proxy support
- HTTP/1.1
- HTTP/2
- HTTP/3 / QUIC where required
- redirects
- content negotiation
- compression
- caching
- MIME handling
- URL parsing
- download/upload
- cookies

## Security

- TLS 1.2 / TLS 1.3
- certificate chains
- trust store
- hostname validation
- origins
- same-origin policy
- CORS
- CSP
- mixed-content rules
- secure contexts
- permissions
- iframe/document sandboxing
- credential/cookie isolation
- process/content isolation where possible

## Document

- HTML tokenizer/parser
- DOM tree
- forms
- links
- navigation/history
- document lifecycle
- custom elements
- Shadow DOM
- templates

## Styling

- CSS tokenization/parsing
- selectors
- specificity
- cascade
- inheritance
- computed styles
- media queries
- custom properties
- pseudo classes/elements

## Layout

- block layout
- inline layout
- positioning
- overflow
- scrolling
- flexbox
- grid
- transforms
- viewport units
- responsive layout

## Graphics

- text
- fonts
- font loading
- Unicode/bidi/text shaping
- raster images
- SVG
- Canvas
- compositing
- clipping
- opacity
- animation
- WebGL / WebGPU if required

## Scripting

- modern ECMAScript
- garbage collection
- promises
- async/await
- modules
- microtasks
- timers
- host bindings
- WebAssembly where required

## Browser APIs / application platform

- events
- `fetch`
- XHR
- Streams
- WebSocket
- Server-Sent Events
- clipboard
- file input / File API
- URL API
- history
- localStorage
- sessionStorage
- IndexedDB
- Cache API
- workers
- service workers
- notifications where needed
- drag/drop where needed
- accessibility tree
- keyboard/mouse/focus/IME

## Principle

Only implement what a concrete compatibility target requires unless the feature belongs in a reusable foundation.

The project should prefer porting or adapting mature implementations for cryptography, JavaScript execution, complex text shaping, and other high-risk/high-complexity areas.
