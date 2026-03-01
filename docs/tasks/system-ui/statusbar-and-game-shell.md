# Task: Status Bar And Game Shell

Status: Not started

## Goal

Port the upstream game-shell behavior that lives around the status bar, menus, quickslot, and top-level game UI routing.

## Upstream References

- `675b9af` recoded UIChatbar
- `80e042f` status/chatbar/UI adjustments
- `e3e97c2` General UI positioning fixes
- `0c294c6` Added ability to change res in-game + options menu
- `13729ea` Quick fix for UIUserList

## Relevant Upstream Files

- `IO/UITypes/UIStatusBar.cpp`
- `IO/UITypes/UIStatusBar.h`
- `IO/UITypes/UIChatBar.cpp`
- `IO/UITypes/UIChatBar.h`
- `IO/UIStateGame.cpp`
- `IO/UIStateGame.h`

## Subtasks

- [ ] Compare local `UIStatusBar` against upstream submenu behavior.
- [ ] Port quickslot fold/extend behavior.
- [ ] Port status-bar menus for menu, setting, community, character, and event.
- [ ] Wire status-bar buttons to quest log, user list, channel, option menu, joypad, quit, and char info windows.
- [ ] Decide whether the local embedded chat bar should remain embedded or be refactored to match upstream shell behavior.
- [ ] Reconcile responsive layout logic with wasm screen sizing.
- [ ] Audit default game-state window composition on map entry.

## Notes

- This is one of the biggest "same file name, very different behavior" gaps.
