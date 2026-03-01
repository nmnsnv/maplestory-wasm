# Task: System UI, Options, Event, Joypad, Quit, And Character Info

Status: Not started

## Goal

Port the remaining standalone system windows that do not naturally fit under map, chat, quest, or cash shop work.

## Upstream References

- `0c294c6` Added ability to change res in-game + options menu
- `5626bd4` Added UIGender, MapleComboBox and moved colors to Color.h
- `3d365b6` Added UIQuit
- `e28d8a9` Finished most of UIQuit + key handling fixes
- `118be5d` Fixed quit alert positioning
- `b3c3d4e` Added 4 new UIs, UI enhancements
- `e3e97c2` General UI positioning fixes

## Relevant Upstream Files

- `IO/UITypes/UIOptionMenu.cpp`
- `IO/UITypes/UIOptionMenu.h`
- `IO/UITypes/UIJoypad.cpp`
- `IO/UITypes/UIJoypad.h`
- `IO/UITypes/UIEvent.cpp`
- `IO/UITypes/UIEvent.h`
- `IO/UITypes/UIQuit.cpp`
- `IO/UITypes/UIQuit.h`
- `IO/UITypes/UICharInfo.cpp`
- `IO/UITypes/UICharInfo.h`

## Subtasks

- [ ] Port `UIOptionMenu`.
- [ ] Decide whether the option menu should expose wasm-specific settings or mirror upstream exactly.
- [ ] Port `UIJoypad` only if it maps cleanly to the fork's input layer.
- [ ] Port `UIEvent` if the server actually uses the corresponding flows.
- [ ] Port `UIQuit`.
- [ ] Reconcile `UICharInfo` placement if it also lands through the player-interaction epic.

## Notes

- `UICharInfo` is intentionally duplicated here as a reminder that it affects both player interaction and standalone UI parity.
