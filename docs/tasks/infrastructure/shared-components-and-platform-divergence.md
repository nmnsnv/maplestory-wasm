# Task: Shared Components And Platform Divergence

Status: Partially started

## Goal

Port the small support pieces that unblock the remaining UI epics, and document intentional divergences where MapleStory-Client code should not be copied directly into this wasm/NX fork.

## Upstream References

- `5626bd4` Added UIGender, MapleComboBox and moved colors to Color.h
- `044d844` Refactor UISkillbook to utilize existing Icon functionality (#24)
- `713c974` Quick fixes - Fixed UI drag elements - Removed remove_cursor for Buttons - Fix for MapBackgrounds
- `3bc456c` Code refactor stemmed from HeavenClientNX
- `e3e97c2` General UI positioning fixes

## Relevant Upstream Files

- `IO/Components/MapleComboBox.cpp`
- `IO/Components/MapleComboBox.h`
- `IO/Components/StatefulIcon.cpp`
- `IO/Components/StatefulIcon.h`
- `IO/Components/NameTag.cpp`
- `IO/Components/NameTag.h`
- `Net/Handlers/Helpers/CharacterParser.cpp`
- `Net/Handlers/Helpers/CharacterParser.h`
- `Net/Handlers/TestingHandlers.cpp`
- `Net/Handlers/TestingHandlers.h`
- `Util/HardwareInfo.h`
- `Util/ScreenResolution.h`
- `Util/WzFiles.cpp`
- `Util/WzFiles.h`

## Subtasks

- [ ] Audit whether `MapleComboBox` is required by `UIOptionMenu`, `UICashShop`, or creation flows before porting it.
- [ ] Port `StatefulIcon` if a later epic depends on it.
- [ ] Compare the upstream `NameTag` component to the local name-tag implementation and merge only the missing behavior.
- [ ] Review `CharacterParser` for reusable parsing logic in player/cash-shop/interaction flows.
- [ ] Decide whether `TestingHandlers` belongs in this fork or should stay upstream-only.
- [ ] Record intentional divergences for `WzFiles`, `HardwareInfo`, and `ScreenResolution`.

## Intentional Divergence Candidates

- `Util/WzFiles.*`: likely not applicable because this fork already uses NX files directly.
- `Util/HardwareInfo.h`: probably not useful in wasm.
- `Util/ScreenResolution.h`: may need a wasm-friendly equivalent rather than a direct port.

## Notes

- This file should stay updated whenever an epic discovers a new shared dependency.
