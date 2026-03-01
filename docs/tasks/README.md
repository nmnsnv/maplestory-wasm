# Upstream Task Map

This backlog tracks the remaining work needed to reach feature parity with MapleStory-Client in this fork.

- Verified against `ryantpayton/MapleStory-Client` upstream `origin/master` at `cbb0fe2` on March 1, 2026.
- This fork is `v83` + `NX`, so every task that touches assets must translate MapleStory-Client paths through `/assets/*.nx` instead of copying newer WZ assumptions directly.
- Use `scripts/nxdump/nxdump` for asset discovery.
- Use `./scripts/build_wasm.sh -g` for verification builds.
- Protocol-sensitive work should be checked against [MapleStory_v83_Protocol_Specification.pdf](/Users/noamnisanov/work/learn/maplestory-wasm/docs/MapleStory_v83_Protocol_Specification.pdf).

## Epics

- `inventory`
  - `complete-missing-feature-inventory.md`
- `map-and-navigation`
  - `minimap-and-worldmap.md`
  - `field-effects-and-travel.md`
- `quests-and-progression`
  - `quest-log.md`
- `chat-and-social`
  - `chat-channel-and-userlist.md`
  - `player-interaction.md`
- `cash-shop`
  - `cash-shop-ui-and-protocol.md`
- `login-and-character-creation`
  - `login-shell-and-notices.md`
  - `login-region-race-and-creation.md`
- `system-ui`
  - `statusbar-and-game-shell.md`
  - `options-event-joypad-quit-charinfo.md`
- `infrastructure`
  - `input-and-ui-state-plumbing.md`
  - `shared-components-and-platform-divergence.md`

## Current Read

- `inventory/complete-missing-feature-inventory.md`: authoritative complete inventory of remaining upstream parity gaps.
- `map-and-navigation/minimap-and-worldmap.md`: partially done in this fork, needs runtime hardening and remaining upstream map features.
- `map-and-navigation/field-effects-and-travel.md`: not started.
- `quests-and-progression/quest-log.md`: not started.
- `chat-and-social/chat-channel-and-userlist.md`: not started.
- `chat-and-social/player-interaction.md`: not started.
- `cash-shop/cash-shop-ui-and-protocol.md`: not started.
- `login-and-character-creation/login-shell-and-notices.md`: not started.
- `login-and-character-creation/login-region-race-and-creation.md`: not started.
- `system-ui/statusbar-and-game-shell.md`: not started.
- `system-ui/options-event-joypad-quit-charinfo.md`: not started.
- `infrastructure/input-and-ui-state-plumbing.md`: not started.
- `infrastructure/shared-components-and-platform-divergence.md`: partially done, still needs cleanup decisions and support ports.
