# Task: Field Effects And Travel

Status: Not started

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

## Subtasks

- [ ] Port `MapEffect` and wire `FIELD_EFFECT` handling to the v83 opcode flow.
- [ ] Compare our set-field handler behavior with upstream `SetFieldHandlers`.
- [ ] Verify portal placement, respawn, and intramap warp behavior against v83 maps.
- [ ] Evaluate whether `TeleportRock` is protocol-complete for this fork or should be deferred.
- [ ] Re-test map asset resolution after every change with representative outlink-heavy maps.
- [ ] Validate every protocol assumption against [MapleStory_v83_Protocol_Specification.pdf](/Users/noamnisanov/work/learn/maplestory-wasm/docs/MapleStory_v83_Protocol_Specification.pdf).

## Notes

- This epic should be done before broad gameplay QA, because map loading and travel bugs mask downstream UI bugs.
