# UI layout diagnostics

The login and game UI dispatchers render elements through `UIElement::draw_checked`.
This establishes a scoped drawing boundary for every floating `UIWindow`, using
its current position and dimensions. Custom `draw` overrides are included without
needing to call a validation function themselves.

The check covers visible button hitboxes (including disabled buttons), texture
rectangles, rendered text glyphs, and filled rectangles. Texture rectangles use
the resolved canvas origin and scale; vertically clipped images and text are
checked after clipping. Invisible and zero-area content is ignored. Texture
checks describe the axis-aligned rectangle before any rotation.

An overflow writes a diagnostic to stderr, which appears in the browser console
in WASM builds. Coordinates in the message are relative to the owning window:

```text
[UI layout] STATSINFO button #5 bounds (207,367 .. 219,379) exceed window bounds (0,0 .. 212,318). Check canvas origins, control offsets and window dimensions.
```

Warnings are deduplicated per window instance and button/texture ID. Text and
filled rectangles each report their first overflow per window. Moving a window
or rendering another frame does not repeat the same warning. Diagnostics leave
rendering and input unchanged so an error remains visible and inspectable.

Map rendering, independently drawn tooltips, and docked/full-screen `UIElement`s
have no window boundary. A scoped boundary is restored when a nested render ends,
including during exception unwinding. This keeps normal offscreen world drawing
out of the UI diagnostics.

When adding or expanding a panel, keep `dimension` synchronized with the full
visible layout and call `keep_on_screen` after resizing. NX canvas origins already
encode many control positions; do not add those positions a second time. If a
window deliberately paints outside its input rectangle (for example, a shadow),
override `draw_bounds` to describe that larger drawing area. Interactive extensions
must also participate in input routing; enlarging the diagnostic boundary alone
does not make them clickable.

## Tests

Run the character-stats regression from the repository root:

```bash
./scripts/run_tests.sh --assets -- -R character_stats
```

The test covers panel dimensions, button artwork and hitboxes, stat updates, AP
packet encoding, overflow detection, warning deduplication, and nested drawing
scopes. It uses the production panel, window input dispatcher, texture loading,
button drawing, and stat model. Graphics submission, text layout, sound, settings
persistence, and network transport are replaced by test doubles, so no GPU or
Cosmic server is needed.

The `--assets` option requires the local NX set and native graphics dependencies
listed in the [testing guide](testing.md#tests-using-nx-assets). Assets remain
read-only. When local dependencies are unavailable, use:

```bash
./scripts/docker_run_tests.sh --assets -- -R character_stats
```

The default local Debug run writes diagnostic captures under
`build/tests/native-Debug-assets/work/character_stats_test/artifacts/`:

- `stats-collapsed.ppm` and `stats-expanded.ppm` capture panel artwork.
- `stats-collapsed.tsv` and `stats-expanded.tsv` record text positions and content.

Captures are generated automatically; no output-path argument is needed. They
support manual inspection rather than automated pixel comparisons. Docker and
other build variants use the directories described in the
[reporting guide](testing.md#build-directories-and-reports).

See [floating-window tests](ui-windows.md#tests) for the self-contained dragging
and screen-clamping checks.
