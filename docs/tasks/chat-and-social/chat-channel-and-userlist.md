# Task: Chat, Channel, And User List

Status: Not started

## Goal

Bring chat-side windows and overlays closer to MapleStory-Client behavior without breaking the existing v83 chat protocol.

## Upstream References

- `675b9af` Mainly recoded UIChatbar
- `80e042f` UIChatbar adjustments to match current GMS
- `e67e3be` Updated UIChatbar to latest from GMS
- `a32247a` Fixed typing + PIC display + channel screen
- `13729ea` Quick fix for UIUserList
- `b3c3d4e` Added 4 new UIs, UI enhancements
- `ae53d6e` Handled player.message() from server

## Relevant Upstream Files

- `IO/UITypes/UIChat.cpp`
- `IO/UITypes/UIChat.h`
- `IO/UITypes/UIChannel.cpp`
- `IO/UITypes/UIChannel.h`
- `IO/UITypes/UIUserList.cpp`
- `IO/UITypes/UIUserList.h`

## Subtasks

- [ ] Audit the current chat bar against upstream before adding more windows.
- [ ] Port `UIChat` and integrate it with the local chat/input stack.
- [ ] Port `UIChannel` and validate its open/close/key behavior.
- [ ] Port `UIUserList`.
- [ ] Reconcile channel/chat behavior with the existing v83 packet handlers and UIStatusbar/UIStatusMessenger.
- [ ] Verify whisper, system message, server message, and channel-specific flows against the v83 protocol.

## Notes

- Some of this epic may require shared-file backports rather than just adding upstream-only files.
