# Task: Complete Missing Feature Inventory

Status: Active inventory

## Scope

This file is the authoritative complete list of remaining upstream parity gaps relative to MapleStory-Client `origin/master` at `cbb0fe2`.

It covers:

- upstream-only source files
- missing UI states and UI element types
- shared-file feature gaps where the file exists locally but upstream behavior is still missing
- parser, handler, and protocol gaps
- platform-specific divergence candidates

## Upstream References

- `cbb0fe2` upstream baseline used for this inventory
- `3bc456c` Code refactor stemmed from HeavenClientNX
- `675b9af` recoded UIChatBar
- `a6ec68b` large UI/login/WZ change set
- `e3e97c2` general UI positioning fixes
- `0daa69d` quick fixes

## 1. Upstream-Only Gameplay Features

- `Character/TeleportRock.cpp`
- `Character/TeleportRock.h`
- `Gameplay/MapleMap/MapEffect.cpp`
- `Gameplay/MapleMap/MapEffect.h`

## 2. Upstream-Only UI Components

- `IO/Components/MapleComboBox.cpp`
- `IO/Components/MapleComboBox.h`
- `IO/Components/NameTag.cpp`
- `IO/Components/NameTag.h`
- `IO/Components/StatefulIcon.cpp`
- `IO/Components/StatefulIcon.h`

## 3. Upstream-Only UI States

- `IO/UIStateCashShop.cpp`
- `IO/UIStateCashShop.h`

## 4. Upstream-Only UI Types

- `IO/UITypes/UIAranCreation.h`
- `IO/UITypes/UICashShop.cpp`
- `IO/UITypes/UICashShop.h`
- `IO/UITypes/UIChannel.cpp`
- `IO/UITypes/UIChannel.h`
- `IO/UITypes/UICharInfo.cpp`
- `IO/UITypes/UICharInfo.h`
- `IO/UITypes/UIChat.cpp`
- `IO/UITypes/UIChat.h`
- `IO/UITypes/UICommonCreation.cpp`
- `IO/UITypes/UICommonCreation.h`
- `IO/UITypes/UICygnusCreation.h`
- `IO/UITypes/UIEvent.cpp`
- `IO/UITypes/UIEvent.h`
- `IO/UITypes/UIExplorerCreation.h`
- `IO/UITypes/UIGender.cpp`
- `IO/UITypes/UIGender.h`
- `IO/UITypes/UIJoypad.cpp`
- `IO/UITypes/UIJoypad.h`
- `IO/UITypes/UILogo.cpp`
- `IO/UITypes/UILogo.h`
- `IO/UITypes/UIOptionMenu.cpp`
- `IO/UITypes/UIOptionMenu.h`
- `IO/UITypes/UIQuestLog.cpp`
- `IO/UITypes/UIQuestLog.h`
- `IO/UITypes/UIQuit.cpp`
- `IO/UITypes/UIQuit.h`
- `IO/UITypes/UIRaceSelect.cpp`
- `IO/UITypes/UIRaceSelect.h`
- `IO/UITypes/UIRegion.cpp`
- `IO/UITypes/UIRegion.h`
- `IO/UITypes/UITermsOfService.cpp`
- `IO/UITypes/UITermsOfService.h`
- `IO/UITypes/UIUserList.cpp`
- `IO/UITypes/UIUserList.h`

## 5. Upstream-Only Network And Parser Files

- `Net/Handlers/CashShopHandlers.cpp`
- `Net/Handlers/CashShopHandlers.h`
- `Net/Handlers/Helpers/CashShopParser.cpp`
- `Net/Handlers/Helpers/CashShopParser.h`
- `Net/Handlers/Helpers/CharacterParser.cpp`
- `Net/Handlers/Helpers/CharacterParser.h`
- `Net/Handlers/PlayerInteractionHandlers.cpp`
- `Net/Handlers/PlayerInteractionHandlers.h`
- `Net/Handlers/SetFieldHandlers.cpp`
- `Net/Handlers/SetFieldHandlers.h`
- `Net/Handlers/TestingHandlers.cpp`
- `Net/Handlers/TestingHandlers.h`
- `Net/Packets/PlayerInteractionPackets.h`

## 6. Upstream-Only Utility Files

- `Util/HardwareInfo.h`
- `Util/ScreenResolution.h`
- `Util/WzFiles.cpp`
- `Util/WzFiles.h`

## 7. Missing UI Element Types In The Local Enum

These types exist upstream but are still missing from the local `UIElement::Type` surface.

- `START`
- `TOS`
- `GENDER`
- `REGION`
- `RACESELECT`
- `CLASSCREATION`
- `LOGINNOTICE_CONFIRM`
- `CHATBAR`
- `QUESTLOG`
- `USERLIST`
- `CHANNEL`
- `CHAT`
- `CHATRANK`
- `JOYPAD`
- `EVENT`
- `OPTIONMENU`
- `QUIT`
- `CHARINFO`
- `CASHSHOP`

## 8. Missing UI State Transitions And Shell Features

These are not represented by upstream-only files alone; they require shared-file parity work.

- cash shop state transition in `UI`
- login shell flow beyond plain `UILogin`
- right-click routing
- mouse-wheel routing
- focus-change routing
- close/escape routing
- UI-level focused element keyboard dispatch with `escape`
- per-element `get_type()` dispatch used by upstream cursor management
- screen resize propagation via `update_screen()`

## 9. Missing Login And Startup Flow Features

- startup logo screen
- terms of service screen
- gender screen
- region selection screen
- race selection screen
- common class-creation flow
- Aran/Cygnus/Explorer creation branches
- login notice confirm variants
- splash-screen removal / auto-login conveniences

## 10. Missing Game-Shell And Status-Bar Features

The local `UIStatusBar` exists, but upstream behavior is still missing.

- upstream `StatusBar3.img`-style layout and responsive positioning
- quickslot fold/extend behavior
- status-bar submenu groups:
  - menu
  - setting
  - community
  - character
  - event
- buttons for:
  - cash shop
  - options
  - channel
  - joypad
  - quit
  - quest
  - user list/community tabs
  - character info
  - event schedule
- direct status-bar key routing
- default game-shell behavior that opens status bar, chat bar, and minimap together

## 11. Missing Chat And Social Windows

- channel window
- chat window
- rank/chat-rank window
- user list window
- standalone community menu flows
- full upstream chat bar behavior
- player interaction handlers and packets
- character info popup window

## 12. Missing Map, Navigation, And Progression Windows

- field effects
- teleport rock support
- quest log
- remaining minimap runtime parity
- remaining world map runtime parity

## 13. Missing System Windows

- option menu
- joypad window
- event window
- quit window

## 14. Missing Cash Shop Stack

- cash shop UI state
- cash shop main window
- cash shop handlers
- cash shop parser
- cash shop entry/exit transitions
- any v83 protocol validation for those flows

## 15. Shared-File Feature Gaps In Existing Local Windows

These files exist locally but still lack upstream behavior.

- `IO/UI.cpp`
  - no cash shop state
  - no right-click / scroll / focus / close routing
  - narrower keyboard dispatch model than upstream
- `IO/UIState.h`
  - no right-click / scroll / close / escape-aware key API
- `IO/UIStateGame.cpp`
  - no upstream-style focused-element keyboard delegation
  - no upstream scroll propagation
  - no upstream right-click propagation
  - no upstream default user-list/chat/status/minimap shell
  - no cash shop menu action path
- `IO/UIStateLogin.cpp`
  - no multi-screen login shell flow
- `IO/UIElement.h`
  - missing `update_screen`, `rightclick`, `send_scroll`, `send_key`, and `get_type`
- `IO/Window.cpp`
  - missing upstream right-click, focus, scroll, and close callback behavior
- `IO/Components/Slider.*`
  - missing upstream scroll-wheel API and extra slider behavior
- `IO/UITypes/UIStatusBar.*`
  - major upstream shell/menu divergence
- `IO/UITypes/UIShop.*`
  - missing upstream right-click sell, scroll-wheel paging, and key handling
- `IO/UITypes/UIItemInventory.*`
  - missing upstream cash-shop button and newer key handling
- `IO/UITypes/UIEquipInventory.*`
  - missing upstream keyboard behavior
- `IO/UITypes/UISkillBook.*`
  - missing upstream keyboard behavior and later icon handling refinements
- `IO/UITypes/UIKeyConfig.*`
  - missing upstream cash-shop mapping position and later behavior additions
- `IO/UITypes/UILogin.*`
  - narrower than upstream login flow
- `IO/UITypes/UILoginNotice.*`
  - missing confirm variants and key handling
- `IO/UITypes/UIWorldSelect.*`
  - missing upstream keyboard behavior and login-shell integration
- `IO/UITypes/UINotice.*`
  - missing upstream keyboard behavior on notice dialogs
- `IO/UITypes/UINpcTalk.*`
  - missing upstream key handling refinements
- `IO/UITypes/UIBuffList.*`
  - missing upstream dynamic screen updates
- `IO/UITypes/UIStatusMessenger.*`
  - missing upstream dynamic screen updates
- `IO/UITypes/UIMiniMap.*`
  - runtime parity still pending
- `IO/UITypes/UIWorldMap.*`
  - runtime parity still pending

## 16. Platform-Divergence Candidates That Need Explicit Decisions

- `Util/WzFiles.*`
  - likely intentionally not portable because this fork uses NX directly
- `Util/HardwareInfo.h`
  - likely not useful in wasm
- `Util/ScreenResolution.h`
  - probably needs a wasm-native equivalent instead of a direct port

## 17. Task Mapping

- map and navigation:
  - `map-and-navigation/minimap-and-worldmap.md`
  - `map-and-navigation/field-effects-and-travel.md`
- quests and progression:
  - `quests-and-progression/quest-log.md`
- chat and social:
  - `chat-and-social/chat-channel-and-userlist.md`
  - `chat-and-social/player-interaction.md`
- cash shop:
  - `cash-shop/cash-shop-ui-and-protocol.md`
- login and creation:
  - `login-and-character-creation/login-shell-and-notices.md`
  - `login-and-character-creation/login-region-race-and-creation.md`
- system UI:
  - `system-ui/statusbar-and-game-shell.md`
  - `system-ui/options-event-joypad-quit-charinfo.md`
- infrastructure:
  - `infrastructure/input-and-ui-state-plumbing.md`
  - `infrastructure/shared-components-and-platform-divergence.md`
