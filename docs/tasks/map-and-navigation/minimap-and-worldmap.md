# Task: Minimap And World Map

Status: In progress

## Goal

Reach parity with MapleStory-Client minimap and world map behavior while keeping `v83` NX path compatibility.

## Upstream References

- `854f23e` UIWorldMap implementation
- `a1d8e91` Added UIMiniMap + window focus event + changes
- `adfc8be` Add MiniMap canvas, scaling window
- `e7c6357` Make minimap window fully scalable
- `2930696` NPC List on Minimap (#22)
- `caf455f` MiniMap tooltips on hover (#23)
- `cf0d240` Changes for Mini Map
- `a5bbff0` Fixed mini map positioning + start up type
- `1d59f9c` Revert Stage changes, modify MapChars and MapNpcs access
- `5560b7e` Missed a few files for UIMiniMap

## Relevant Upstream Files

- `IO/UITypes/UIMiniMap.cpp`
- `IO/UITypes/UIMiniMap.h`
- `IO/UITypes/UIWorldMap.cpp`
- `IO/UITypes/UIWorldMap.h`
- `IO/Components/MapTooltip.cpp`
- `IO/Components/MapTooltip.h`
- `IO/Components/TextTooltip.cpp`
- `IO/Components/TextTooltip.h`

## Current Fork Status

- Base minimap and world map windows are ported.
- Tooltip routing and v83 NX path translation are in place.
- The build is clean with `./scripts/build_wasm.sh -g`.

## Remaining Subtasks

- [x] Port the minimap and world map windows.
- [x] Translate upstream asset lookups to `assets/UI.nx`, `assets/Map.nx`, and `assets/String.nx`.
- [x] Add map tooltip and text tooltip plumbing.
- [ ] Runtime-test minimap behavior on town, field, dungeon, mirror, and maps without `miniMap`.
- [ ] Runtime-test world map navigation, parent map traversal, hover state, and close/escape behavior.
- [ ] Verify minimap NPC list scrolling, selection, and tooltip behavior on maps with many NPCs.
- [ ] Verify map marker alignment against v83 canvases and center offsets.
- [ ] Audit saved settings for startup type, position, and toggled state.
- [ ] Compare remaining shared-file behavior against upstream for `UIStateGame`, `Stage`, `Slider`, and `Textfield`.

## Notes

- All asset lookups must continue to be discovered through `scripts/nxdump/nxdump`.
- Do not reintroduce MapleStory-Client WZ path assumptions such as non-v83 map roots.
