# Task: Quest Log

Status: Not started

## Goal

Port MapleStory-Client quest log behavior and its supporting interaction model to the v83 protocol and NX data layout.

## Upstream References

- `e47da55` Added quest window, code cleanup, ui position + slider fixes
- `6276ebd` Added new UIs and properly handled tab/esc keys
- `a14f1dc` Updates to UIQuestLog and UIChatbar
- `e3a9337` UIStatusbar/UIQuestLog changes, added new color
- `72c1cbc` UISkillbook + UIQuestLog clean up
- `cf0d240` Changes for Mini Map

## Relevant Upstream Files

- `IO/UITypes/UIQuestLog.cpp`
- `IO/UITypes/UIQuestLog.h`

## Subtasks

- [ ] Port `UIQuestLog`.
- [ ] Add any missing `UIElement::Type`, keybinding, tooltip, and slider support the quest log depends on.
- [ ] Verify quest list, detail panes, tab switching, and escape/close behavior.
- [ ] Check item, NPC, and map cross-links from the quest log against v83 data.
- [ ] Audit shared-file diffs in quest-related handlers and UI helpers after the base port compiles.
- [ ] Verify protocol/data expectations against [MapleStory_v83_Protocol_Specification.pdf](/Users/noamnisanov/work/learn/maplestory-wasm/docs/MapleStory_v83_Protocol_Specification.pdf).

## Notes

- Expect follow-up work in shared UI support code even though the upstream-only surface is small.
