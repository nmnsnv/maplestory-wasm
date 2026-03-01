# Task: Player Interaction

Status: Not started

## Goal

Port trade and other player-interaction flows that MapleStory-Client handles outside the current fork.

## Upstream References

- `675b9af` Mainly recoded UIChatbar
- `d927a4a` Added 3 new handlers, improved some old code
- `2534752` Added UICharInfo

## Relevant Upstream Files

- `Net/Handlers/PlayerInteractionHandlers.cpp`
- `Net/Handlers/PlayerInteractionHandlers.h`
- `Net/Packets/PlayerInteractionPackets.h`
- `IO/UITypes/UICharInfo.cpp`
- `IO/UITypes/UICharInfo.h`

## Subtasks

- [ ] Diff local interaction packet coverage against upstream `PlayerInteractionPackets`.
- [ ] Port `PlayerInteractionHandlers`.
- [ ] Port `UICharInfo`.
- [ ] Identify which interaction windows exist upstream but still need a local UI type.
- [ ] Validate every interaction packet against [MapleStory_v83_Protocol_Specification.pdf](/Users/noamnisanov/work/learn/maplestory-wasm/docs/MapleStory_v83_Protocol_Specification.pdf) before enabling it.

## Notes

- This epic is likely to expose v83 protocol mismatches quickly, so handler-first validation is safer than UI-first porting.
