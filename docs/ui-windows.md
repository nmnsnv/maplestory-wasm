# Floating windows

Every floating panel and dialog derives from `UIWindow`. Docked HUD elements
(chat, status bar, buff icons) and full-screen scenes derive from `UIElement`.

`UIWindow` owns title-bar dragging, pointer capture, screen-edge clamping, and
movement notifications. Its `send_cursor` and `remove_cursor` entry points are
final so individual controls cannot accidentally bypass an active window drag.
Implement `send_window_cursor` and `remove_window_cursor` for custom controls;
delegate unhandled input to the corresponding `UIWindow` hook.

The default drag area is the first 20 pixels across the current window width.
A window can supply another area through its constructor or `dragarea`, for
example Quest Helper's 25-pixel header. Header buttons retain priority. A custom
`is_in_range` must include `in_drag_area` while its panel is visible.

Use `set_default_position` for layout anchors that can be recalculated, such as
centering NPC dialogue after its content changes. Once a user moves the window,
subsequent layout updates preserve that position. Call `keep_on_screen` after
changing a panel's dimensions. Child elements with cached absolute coordinates
can update them in `position_changed`.

`UIDragElement<PositionSetting>` is the Settings adapter for persistent window
positions. It contains no input handling. Register each position setting in
`Configuration` so it participates in loading and saving; a negative default
coordinate indicates that the window should use its initial layout anchor.

## Tests

Run the floating-window regression from the repository root:

```bash
./scripts/run_tests.sh -- -R ui_window
```

This self-contained test needs no NX assets, GPU, or Cosmic server. It exercises
the production `UIWindow` dispatcher with a small test control and stubs for the
surrounding UI. Checks cover pointer capture, grab offsets, child-control routing,
release outside the window, saved-position callbacks, and screen clamping after
resizing. Compile-time assertions verify that floating windows inherit `UIWindow`
while docked HUD elements do not.

The case also runs in the default suite and native-test CI. When the local
toolchain is unavailable, use:

```bash
./scripts/docker_run_tests.sh -- -R ui_window
```

The default local Debug run writes results to
`build/tests/native-Debug/reports/junit.xml`. See the
[testing guide](testing.md) for prerequisites, other build variants, and failure
logs. For panel artwork and layout diagnostics using real NX data, see
[UI layout tests](ui-layout-diagnostics.md#tests).
