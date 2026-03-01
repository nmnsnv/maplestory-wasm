# Task: Field Effects And Travel

Status: Implemented, runtime verification pending

## Goal

Finish the non-window map feature gap: field effects, set-field integration, and teleport/travel utilities.

## Upstream References

- `9fbd09a` Added map effects for Opcode 138 - FIELD_EFFECT
- `aff5f55` Fixed general map issues
- `a3a9500` Added fix for outlink hitting different files - Updated required NX files
- `15a4b50` Fix portals not placing characters correctly (#27)
- `0daa69d` Quick Fixes

## Relevant Upstream Files

- `Gameplay/MapleMap/MapEffect.cpp`
- `Gameplay/MapleMap/MapEffect.h`
- `Character/TeleportRock.cpp`
- `Character/TeleportRock.h`
- `Net/Handlers/SetFieldHandlers.cpp`
- `Net/Handlers/SetFieldHandlers.h`

## Current Fork Status

- `MapEffect` is now ported for local v83 NX paths via `assets/Map.nx -> Effect.img`.
- `FIELD_EFFECT` is wired to a dedicated handler for the upstream-supported mode `3` payload.
- `Stage` now owns, updates, draws, and clears map effects during travel.
- Intermap portal requests now follow the upstream `-1 + portal name` behavior instead of sending the destination map id directly.
- Local player stats now keep the destination map id in sync during portal travel.
- Teleport rock saved-map parsing was already present locally and remains the current stopping point for this epic.
- The wasm build is clean with `./scripts/build_wasm.sh -g`.

## Subtasks

- [x] Port `MapEffect` and wire `FIELD_EFFECT` handling to the v83 opcode flow.
- [x] Compare our set-field handler behavior with upstream `SetFieldHandlers`.
- [ ] Verify portal placement, respawn, and intramap warp behavior against v83 maps.
- [x] Evaluate whether `TeleportRock` is protocol-complete for this fork or should be deferred.
- [ ] Re-test map asset resolution after every change with representative outlink-heavy maps.
- [x] Validate every protocol assumption against [MapleStory_v83_Protocol_Specification.pdf](/Users/noamnisanov/work/learn/maplestory-wasm/docs/MapleStory_v83_Protocol_Specification.pdf).

## Notes

- This epic should be done before broad gameplay QA, because map loading and travel bugs mask downstream UI bugs.
- Teleport rock remains parser/storage-only in this fork for now. UI and outbound packet work should be handled in a later task if needed.
- Runtime verification should include at least one field-effect map entry and portal travel checks on Amherst `1000000`, Lith Harbor `104000000`, and a representative Victoria town such as Henesys `100000000`.
