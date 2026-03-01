# Task: Login, Region, Race, And Character Creation

Status: Not started

## Goal

Review MapleStory-Client login and creation flows, then port only the parts that make sense for this fork's server and character model.

## Upstream References

- `671ec3e` Finished WorldSelect screen + new screen
- `890b91b` Added Terms of Service
- `f0cf386` Renamed UICharCreation to UIRaceSelect
- `9a44255` Renamed UIClassCreation to UIAranCreation
- `1d950dc` Quick fix for new race creation method
- `0927cf8` Fixed remaining issues with new character creation method
- `b0cd6e1` Character creation bug fixes
- `2c59c58` Updated Terms of Service based on GMS v83
- `10e0d71` Added temporary code for EU server switch (UIRegion)
- `0ef89d0` Added auto login for splash screen removal
- `b6c0c1a` Remove start logo on debug

## Relevant Upstream Files

- `IO/UITypes/UILogo.cpp`
- `IO/UITypes/UILogo.h`
- `IO/UITypes/UITermsOfService.cpp`
- `IO/UITypes/UITermsOfService.h`
- `IO/UITypes/UIRegion.cpp`
- `IO/UITypes/UIRegion.h`
- `IO/UITypes/UIGender.cpp`
- `IO/UITypes/UIGender.h`
- `IO/UITypes/UIRaceSelect.cpp`
- `IO/UITypes/UIRaceSelect.h`
- `IO/UITypes/UICommonCreation.cpp`
- `IO/UITypes/UICommonCreation.h`
- `IO/UITypes/UIAranCreation.h`
- `IO/UITypes/UICygnusCreation.h`
- `IO/UITypes/UIExplorerCreation.h`

## Subtasks

- [ ] Decide which of the upstream login screens apply to this fork's server flow.
- [ ] Port `UITermsOfService` only if the server/login flow requires it.
- [ ] Evaluate `UIRegion` and race-selection flows for relevance to the v83 target.
- [ ] Port reusable creation infrastructure from `UICommonCreation` if it improves local character creation without introducing incompatible jobs.
- [ ] Keep any non-v83 jobs or creation branches behind an explicit "not supported" decision.
- [ ] Validate login/creation packets before wiring in new screens.

## Notes

- This epic should be treated as selective parity, not blind upstream copying.
