# Task: Input And UI State Plumbing

Status: Not started

## Goal

Backport the shared UI/input capabilities that multiple missing features depend on.

## Upstream References

- `713c974` Fixed UI drag elements - Removed remove_cursor for Buttons - Fix for MapBackgrounds
- `e3e97c2` General UI positioning fixes
- `4f7786a` Renamed slider types
- `3bc456c` Code refactor stemmed from HeavenClientNX

## Relevant Upstream Files

- `IO/UI.cpp`
- `IO/UI.h`
- `IO/UIState.h`
- `IO/UIStateGame.cpp`
- `IO/UIStateGame.h`
- `IO/UIStateLogin.cpp`
- `IO/UIStateLogin.h`
- `IO/UIStateCashShop.cpp`
- `IO/UIStateCashShop.h`
- `IO/UIElement.cpp`
- `IO/UIElement.h`
- `IO/Window.cpp`
- `IO/Components/Slider.cpp`
- `IO/Components/Slider.h`

## Subtasks

- [ ] Add right-click plumbing from `Window` through `UI` and `UIState`.
- [ ] Add mouse-wheel routing from `Window` through `UI` and `UIState`.
- [ ] Add close/focus handling where upstream behavior matters.
- [ ] Expand per-element keyboard handling to include `escape`.
- [ ] Add missing `UIElement` capabilities: `get_type`, `send_key`, `send_scroll`, `rightclick`, `update_screen`.
- [ ] Decide whether to adopt upstream cursor-removal semantics wholesale or selectively.
- [ ] Add `UIStateCashShop` only when the cash-shop epic is ready to use it.
- [ ] Add upstream slider scroll behavior where required by quest log, shop, and minimap NPC lists.

## Notes

- Several higher-level epics should not start until this plumbing is explicitly assessed.
