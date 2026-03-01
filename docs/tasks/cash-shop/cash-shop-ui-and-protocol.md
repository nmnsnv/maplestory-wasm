# Task: Cash Shop UI And Protocol

Status: Not started

## Goal

Decide whether cash shop parity is in scope for this fork, and if it is, port the full stack instead of only the top-level UI files.

## Upstream References

- `4da966f` Added Cash Shop integration UI needs work
- `570a142` UI updates for UICashShop

## Relevant Upstream Files

- `IO/UIStateCashShop.cpp`
- `IO/UIStateCashShop.h`
- `IO/UITypes/UICashShop.cpp`
- `IO/UITypes/UICashShop.h`
- `Net/Handlers/CashShopHandlers.cpp`
- `Net/Handlers/CashShopHandlers.h`
- `Net/Handlers/Helpers/CashShopParser.cpp`
- `Net/Handlers/Helpers/CashShopParser.h`

## Subtasks

- [ ] Confirm whether the target v83 server exposes cash shop flows that this client should support.
- [ ] If yes, port `CashShopHandlers` and `CashShopParser` first.
- [ ] Port `UICashShop` and `UIStateCashShop`.
- [ ] Map every asset path through the local NX layout instead of upstream WZ helpers.
- [ ] Define a safe fallback if the server/protocol does not support cash shop entry.
- [ ] Validate all packet assumptions against [MapleStory_v83_Protocol_Specification.pdf](/Users/noamnisanov/work/learn/maplestory-wasm/docs/MapleStory_v83_Protocol_Specification.pdf).

## Notes

- This is the most likely epic to contain large version drift relative to the fork's v83 target.
