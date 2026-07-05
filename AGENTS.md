# AGENTS.md

## Scope

This file tells agents how to build and run this repository without guessing.
Prefer the local workflow first. Use Docker when the local workflow is not possible or the required local toolchain is unavailable.

**IMPORTANT**
This repo is the WASM client. The Cosmic server lives in a separate repository at `/home/formatme/Cosmic/` and is also reachable from this repo via the `cosmic/` symlink. Both the client and the server are editable; when a feature needs server-side changes (NPC scripts, quest data, channel behavior), edit the Cosmic server directly rather than working around it client-side.

## Hard Rules

- Never modify anything under `assets/`.
- Treat `assets/` as read-only input data at all times.
- Do not delete, move, rename, regenerate, or reformat files in `assets/`.
- Do not change build commands just to work around a missing local dependency. Use the documented Docker fallback instead.
- The `cosmic/` symlink points at `/home/formatme/Cosmic/` (the server repo). Treat it as a real checkout: edit server source, NPC scripts (`cosmic/scripts/...`), and config there. Do not delete or retarget the symlink.

## Repo Facts

- WASM client source: `src/client`
- Shared NX library: `src/nlnx`
- Client build output: `build/JourneyClient.js`, `build/JourneyClient.wasm`, and optionally `build/JourneyClient.wasm.map`
- Local web entrypoints: `web/server.py`, `web/ws_proxy.py`, `web/assets_server.py`
- Docker web stack: root `docker-compose.yml`
- The client is designed to run with Cosmic server
- Cosmic server repo: `/home/formatme/Cosmic/` (symlinked as `cosmic/` from this repo)
- Server NPC/quest scripts: `cosmic/scripts/npc/`, `cosmic/scripts/quest/`
- Server handbook (quest/item/map reference data): `cosmic/handbook/`
- Server config: `cosmic/config.yaml`

## Client Build

Preferred build command:

```bash
./scripts/build_wasm.sh
```

Useful variants:

```bash
./scripts/build_wasm.sh --debug # --debug is the same as -g
./scripts/build_wasm.sh --jobs 4
```

If the local Emscripten or CMake toolchain is unavailable, or the local build is not possible, use the Docker fallback:

```bash
./scripts/docker_build_wasm.sh
```

The Docker fallback accepts the same flags:

```bash
./scripts/docker_build_wasm.sh --debug
./scripts/docker_build_wasm.sh --jobs 4
```

## Cosmic Server

The server is a separate Java (Maven) project at `/home/formatme/Cosmic/` (`cosmic/` from this repo). It is editable: change NPC scripts, quest scripts, config, or Java source there and rebuild.

**Requirements**

- Java 21 (required). The Cosmic v83 GraalVM JS engine relies on `sun.misc.Unsafe.ensureClassInitialized`, which was removed in JDK 24+. Java 26 crashes; Java 21 works. Use `/usr/lib/jvm/java-21-openjdk/bin/java`.
- MySQL at `localhost:3306`, database `cosmic`, user `root`, password `password123` (per `cosmic/config.yaml`).

**Build**

```bash
cd cosmic && ./mvnw -q package
```

**Run** (from the `cosmic/` directory so `wz-path=wz` resolves)

```bash
/usr/lib/jvm/java-21-openjdk/bin/java -Xmx2048m -Dwz-path=wz -jar target/Cosmic.jar
```

The server listens on `8484` (login) and `7575`–`7577` (channels). NPC/quest scripts under `cosmic/scripts/` are loaded at startup, so restart the server after editing them.

## Local Deployment

Use local deployment when the local toolchain and local services are available.

1. Build the client with `./scripts/build_wasm.sh`.
2. If that is not possible, build the client with `./scripts/docker_build_wasm.sh`.
3. Install the local Python dependency if needed:

```bash
pip install -r web/requirements.txt
```

4. Start the web services from the repository root in separate terminals:

```bash
python3 web/server.py
python3 web/ws_proxy.py --ws-port 8080
python3 web/assets_server.py --port 8765 --directory .
```

5. Open `http://localhost:8000`.

Assume the websocket proxy is forwarding to a running Cosmic server unless the user explicitly says otherwise.

## Docker Deployment

Use Docker when local deployment is not practical.

Preferred Docker web-stack command:

```bash
./scripts/run_all.sh
```

This script creates the shared Docker network and starts the root web containers.

For the web side only:

```bash
./scripts/docker_web_up.sh -d
```

To stop everything:

```bash
./scripts/stop_all.sh
```

## In-Browser GM Commands & Automation

When the WASM client is loaded in a browser tab and `MapleAi` is injected (via `scripts/maple_ai_common.js`), all Cosmic GM commands are available as chat messages sent through `MapleAi.sendChat('!command args')`. The server's `CommandsExecutor` intercepts any message starting with `!` and runs the GM command — no special API or server access beyond the normal chat packet is needed.

**Prerequisite**: inject `MapleAi` first:

```js
await eval(await (await fetch('/scripts/maple_ai_common.js')).text());
```

### Available GM Commands (from browser)

| Command | Syntax | Effect |
|---|---|---|
| `!item` | `!item <itemid> <qty>` | Spawn item into inventory |
| `!level` | `!level <n>` | Set player level |
| `!heal` | `!heal` | Restore HP/MP to full |
| `!hpmp` | `!hpmp <value>` | Set HP/MP to specific value |
| `!warp` | `!warp <mapid>` | Warp to any map |
| `!spawn` | `!spawn <mobid> <qty>` | Spawn mobs on player position |
| `!killall` | `!killall` | Kill all mobs on current map |
| `!startquest` | `!startquest <questid>` | Force-start a quest (status → 1) |
| `!completequest` | `!completequest <questid>` | Force-complete a quest (status → 2) |
| `!resetquest` | `!resetquest <questid>` | Reset a quest to not-started (status → 0) |
| `!dispose` | `!dispose` | Close any open NPC dialogue / unstick UI |

### Automation Scripts

All scripts live in `scripts/` and are served at `/scripts/...` by the web server.

| Script | Purpose |
|---|---|
| `maple_ai_common.js` | Core harness. Exposes `globalThis.MapleAi` (v2) with WASM hooks: `sendChat`, `warpToMap`, `teleportToNpc`, `talkToNearestNpc`, `captureDialogue`, `getQuestStatus`, `getPlayerState`, `listNpcs`, `dispose`, etc. Load this first. |
| `quest_requirement_setup.js` | Auto-satisfies quest requirements before testing. Reads `quest_requirements.json` and uses GM commands to give items, spawn+kill mobs, start/complete prerequisite quests, set level, and heal. Call `setupQuestRequirements(questId, 'start'|'complete')` or `setupAllQuestRequirements(questId)`. |
| `quest_requirements.json` | Machine-readable quest requirements extracted from WZ `Check.img.xml` (quests 1000–1050). Contains NPC, level, job, item, mob, and quest-prerequisite data for both start and complete phases. |
| `quest_test_runner.js` | Runs scripted quest start dialogues in bulk. Filters to quests with `hasScript: true` in the manifest (scripted quests only — normal quests driven by NPC scripts are **not** covered). Set `globalThis.questTestOptions` or pass overrides to `runQuestTests()`. |
| `full_test_suite.js` | One-shot full-suite runner. Auto-sets GM level 6 via `!setgmlevel`, loads `quest_test_runner.js`, and runs every scripted quest. Results land in `globalThis.fullQuestTestReport`. Configure via `globalThis.fullQuestTestOptions`. |
| `extract_quest_data.py` | Python generator (not a browser script). Rebuilds `scripts/quest_manifest.json` from Cosmic WZ XML (`Quest.wz/Check.img.xml`, `Quest.img.xml`) and the quest-script index. Run after adding/removing quest scripts: `python3 scripts/extract_quest_data.py`. |
| `quest_manifest.json` | Generated quest manifest (~94000 entries). Each entry has `name`, `startNpc`/`endNpc`, `startNpcMaps`/`endNpcMaps`, `hasScript`, `scriptPath`, and `startRequirements`. `quest_test_runner.js` and `full_test_suite.js` only test entries where `hasScript: true`. |
| `npc_scanner.js` | Lists NPCs on the current map via `maple_get_npc_info` (index, oid, name, x, y). Useful for verifying an NPC actually spawned before talking to it. Result in `globalThis.mapleNpcScan` and `console.table`. |
| `npc_dialogue_matrix.js` | Tests NPC dialogues across multiple maps. Warps to each map, teleports to the NPC, talks, and captures dialogue. Set `globalThis.npcDialogueTargets` with `{name, mapId, npcId?}` entries. |
| `ui_navigation.js` | Canvas coordinate helpers for clicking UI elements. |
| `logout_switch_char.js` | Character switching without page reload. |

### Typical Test Flow

```js
// 1. Load MapleAi
await eval(await (await fetch('/scripts/maple_ai_common.js')).text());

// 2. Load quest requirement setup
await eval(await (await fetch('/scripts/quest_requirement_setup.js')).text());

// 3. Satisfy quest requirements before testing
await globalThis.setupQuestRequirements(1018, 'complete');  // gives item, spawns+kills mob

// 4. Start and complete the quest
MapleAi.sendChat('!startquest 1018');
await MapleAi.sleep(800);
MapleAi.sendChat('!completequest 1018');
await MapleAi.sleep(800);

// 5. Verify quest status
MapleAi.getQuestStatus(1018);  // → 2 (completed)

// 6. Talk to the NPC to verify post-completion dialogue
await MapleAi.warpToMap(40000, { timeoutMs: 15000 });
MapleAi.teleportToNpc('Peter');
await MapleAi.sleep(500);
MapleAi.talkToNearestNpc();
await MapleAi.sleep(800);
MapleAi.call('maple_get_dialogue_text', 'string', [], []);  // → dialogue text
await MapleAi.dispose();
```

### Quest Status Values

| Value | Meaning |
|---|---|
| 0 | Not started |
| 1 | Started (in progress) |
| 2 | Completed |

### Dialogue Action Mapping

Used by `maple_advance_dialogue` to click through NPC dialogues:

| Action | Button |
|---|---|
| 0 | OK |
| 1 | NEXT |
| 2 | PREV |
| 3 | END |
| 4 | YES |
| 5 | NO |
| 6 | Select-first option + OK (for selection dialogs) |

`MapleAi.dialogueActionForMode(mode)` picks the right action: mode 4 (selection) → action 6, mode 2/3 (yes/no, accept/decline) → action 4, mode 5 (unknown) → action 0, else → action 1 (NEXT).

## Quest & NPC Testing

This section documents how quests and NPCs are structured on the Cosmic server, how to create missing NPC/quest scripts, and how to test quest flows end-to-end from the WASM client using the automation scripts in `scripts/`.

### Quest Lifecycle

Every quest has a status stored per-character:

| Status | Meaning |
|---|---|
| 0 | Not started |
| 1 | Started (in progress) |
| 2 | Completed |

There are two quest types on the Cosmic server:

- **Normal quests**: The NPC script (`cosmic/scripts/npc/<npcid>.js`) drives the dialogue and calls `cm.startQuest()` / `cm.completeQuest()`. The server's `QuestActionHandler` processes quest actions 0 (lost) and 2 (complete). Most Maple Island quests are normal quests.
- **Scripted quests**: The WZ data declares a `startscript` and/or `endscript` (e.g. quest 1021 has `q1021s` / `q1021e`). The client sends quest action 4 (scripted start) or 5 (scripted end), and the server's `QuestScriptManager` runs `cosmic/scripts/quest/<questid>.js`, which has `start(mode, type, selection)` and `end(mode, type, selection)` functions using `qm.*` methods.

### NPC Script Format

NPC scripts live at `cosmic/scripts/npc/<npcid>.js` and are loaded at server startup. They use the GraalVM JS engine and follow this shape:

```js
// start() runs when the player opens dialogue with the NPC.
function start() {
    cm.sendNext("Hello, new Mapler!");
}

// action() runs on each dialogue advance. mode is the dialogue mode
// (1=text, 2=yes/no, 3=accept/decline, 4=selection), type is the button,
// selection is the chosen option index for selection dialogs.
function action(mode, type, selection) {
    if (mode === 0 && type === 0) { cm.dispose(); return; }
    if (cm.isQuestCompleted(1018)) {
        cm.sendOk("You already finished Todd's lesson.");
        cm.dispose();
    } else if (cm.isQuestStarted(1018)) {
        cm.sendOk("Bring the shellpiece to Peter.");
        cm.dispose();
    } else {
        cm.sendOk("Talk to Todd first.");
        cm.dispose();
    }
}
```

Common `cm.*` methods: `sendNext`, `sendPrev`, `sendNextPrev`, `sendOk`, `sendYesNo`, `sendSimple`, `sendAcceptDecline`, `warp`, `startQuest`, `completeQuest`, `gainExp`, `gainItem`, `haveItem`, `isQuestStarted`, `isQuestCompleted`, `dispose`. Selection dialogs use `cm.sendSimple("text\r\n#L0#Option A#l\r\n#L1#Option B#l")` and read `selection` in `action()`.

### Quest Script Format

Scripted quests live at `cosmic/scripts/quest/<questid>.js` and use `qm.*` (same methods as `cm` plus `forceStartQuest` / `forceCompleteQuest`):

```js
function start(mode, type, selection) {
    if (mode === 0 && type === 0) { qm.dispose(); return; }
    qm.sendNext("Roger's Apple story page 1...");
    // additional pages via qm.sendNextPrev in subsequent action() calls
}

function end(mode, type, selection) {
    if (qm.haveItem(2010007)) {
        qm.gainItem(2010007, -1);
        qm.gainExp(10);
        qm.forceCompleteQuest();
    }
    qm.dispose();
}
```

### Creating Missing NPC Scripts

Some NPCs in the WZ data have no server-side script, so talking to them produces no dialogue. To make them testable:

1. Find the NPC ID from the WZ placement data (see the Maple Island reference below).
2. Create `cosmic/scripts/npc/<npcid>.js` with `start()` and `action(mode, type, selection)` functions.
3. Branch dialogue on quest status using `cm.isQuestStarted()` / `cm.isQuestCompleted()` so the NPC responds differently before, during, and after its associated quest.
4. Restart the Cosmic server — scripts load only at startup.

Scripts created this way during the Maple Island test pass: `2000.js` (Roger), `2002.js` (Peter), `2005.js` (Sam), `2006.js` (Tienk).

### End-to-End Quest Test Workflow

The automation stack lets you test a quest flow without manual gameplay. The general recipe:

```js
// 1. Load the automation harness (exposes globalThis.MapleAi).
await eval(await (await fetch('/scripts/maple_ai_common.js')).text());

// 2. Load the quest requirement satisfier.
await eval(await (await fetch('/scripts/quest_requirement_setup.js')).text());

// 3. Satisfy the quest's start requirements (items, level, prereq quests, HP).
await globalThis.setupQuestRequirements(1018, 'start');

// 4. Travel to the start NPC and talk to it.
await MapleAi.warpToMap(40000, { timeoutMs: 15000 });
MapleAi.teleportToNpc('Todd');
await MapleAi.sleep(500);
MapleAi.talkToNearestNpc();
const pages = await MapleAi.captureDialogue();
await MapleAi.dispose();

// 5. Satisfy the complete requirements (mob kills, items, etc.).
await globalThis.setupQuestRequirements(1018, 'complete');

// 6. Travel to the end NPC and complete via dialogue, or force-complete
//    with !completequest when the mob-kill requirement can't be simulated.
MapleAi.sendChat('!completequest 1018');
await MapleAi.sleep(800);
MapleAi.getQuestStatus(1018);  // → 2

// 7. Talk to the end NPC again to verify post-completion dialogue.
await MapleAi.warpToMap(40000, { timeoutMs: 15000 });
MapleAi.teleportToNpc('Peter');
await MapleAi.sleep(500);
MapleAi.talkToNearestNpc();
await MapleAi.captureDialogue();
await MapleAi.dispose();
```

For **scripted quests** (those with `startscript`/`endscript`), step 4 uses `maple_send_quest_action` instead of talking to the NPC:

```js
// Scripted start: action 4. Scripted end: action 5.
MapleAi.call('maple_send_quest_action', 'number',
  ['number', 'number', 'number'], [4, 1021, 2000]);
```

### Quest Requirement Automation

`scripts/quest_requirement_setup.js` reads `scripts/quest_requirements.json` (extracted from WZ `Check.img.xml`, quests 1000–1050) and uses GM commands to auto-satisfy prerequisites before you test:

- **Quest prerequisites**: `!startquest` / `!completequest` to reach the required state.
- **Level requirements**: `!level` to set the player level within the min/max range.
- **Item requirements**: `!item <id> <qty>` to give required items.
- **Mob kill requirements**: `!spawn <mobid> <count>` then `!killall` (credits kills to the player). **Caveat**: `!killall` may not register kills for quest progress on all server builds; when in doubt, fall back to `!completequest` to force completion.
- **HP requirements**: `!heal` / `!hpmp` to restore or set HP/MP.

Call `setupQuestRequirements(questId, 'start' | 'complete')` for a single phase, or `setupAllQuestRequirements(questId)` for both. Each call returns `{ questId, phase, log, questStatus }`.

### Batch Quest Testing

`scripts/quest_test_runner.js` runs scripted quest start dialogues in bulk. It reads `scripts/quest_manifest.json` (which marks quests with `hasScript: true`), warps to each quest's start NPC, dispatches quest action 4, captures the dialogue pages, and reports pass/fail. Configure via `globalThis.questTestOptions` or pass overrides to `runQuestTests()`:

```js
await eval(await (await fetch('/scripts/quest_test_runner.js')).text());
// Runs automatically on load. To re-run with specific quests:
await globalThis.runQuestTests({ questIds: ['1021', '1033'], downloadReport: true });
```

The report lands in `globalThis.questTestReport` and (if `downloadReport`) downloads as `quest_test_report.json`.

### Batch NPC Dialogue Testing

`scripts/npc_dialogue_matrix.js` walks a list of NPC targets, warps to each map, teleports to the NPC, talks, and captures dialogue. Provide targets explicitly or derive them from `quest_manifest.json`:

```js
globalThis.npcDialogueTargets = [
  { name: 'Roger', mapId: 20000, npcId: 2000 },
  { name: 'Peter', mapId: 40000, npcId: 2002 },
];
await eval(await (await fetch('/scripts/npc_dialogue_matrix.js')).text());
```

**Known limitation**: NPCs with selection dialogs (e.g. Robin, 2003) loop in automated capture because `dialogueActionForMode(4)` always selects the first option and restarts the script. Test selection NPCs manually or extend the matrix to advance only once on selection dialogs.

### Full Test Suite Pipeline

The end-to-end pipeline for regenerating quest data and running the full scripted-quest suite:

1. **Generate the manifest** (after adding/removing quest scripts or pulling WZ changes):
   ```bash
   python3 scripts/extract_quest_data.py
   ```
   This reads `cosmic/wz/Quest.wz/Check.img.xml` + `Quest.img.xml` and the `cosmic/scripts/quest/` index, then writes `scripts/quest_manifest.json`. Each entry's `hasScript` flag is `true` only when a matching `cosmic/scripts/quest/<id>.js` exists.

2. **Run the full suite** in the browser (one shot):
   ```js
   await eval(await (await fetch('/scripts/full_test_suite.js')).text());
   ```
   `full_test_suite.js` auto-sets GM level 6 (`!setgmlevel <player> 6`), loads `quest_test_runner.js`, and runs every `hasScript: true` quest. The report lands in `globalThis.fullQuestTestReport`; the runner also writes `globalThis.questTestReport` and (if `downloadReport`) downloads `quest_test_report.json`.

3. **Read the report**: `globalThis.questTestReport` has `{ totalQuests, planned, tested, passed, failed, failures, results }`. Failures include the quest ID, NPC, map, error, and any captured dialogue.

**Coverage caveat**: `quest_test_runner.js` filters to `hasScript: true` (line 259), so only scripted quests are tested. Normal quests (driven by NPC scripts via `cm.startQuest`/`cm.completeQuest`) are **not** covered by the suite — test those individually with the End-to-End Quest Test Workflow above. The last full run tested 250 of ~94000 manifest entries (all scripted quests present at that time).

### Updating NPC / Quest Behavior

When a quest or NPC needs new or changed behavior, the edit-rebuild-retest loop is:

1. **Edit the script** under `cosmic/scripts/npc/<npcid>.js` or `cosmic/scripts/quest/<questid>.js`. NPC scripts load at server startup; quest scripts load on first invocation but are safest to restart after editing.
2. **Rebuild the server** (only needed if you changed Java source; script-only changes skip this):
   ```bash
   cd cosmic && ./mvnw -q package
   ```
3. **Restart the Cosmic server** with Java 21 (scripts load at startup):
   ```bash
   # kill the old process, then from cosmic/:
   /usr/lib/jvm/java-21-openjdk/bin/java -Xmx2048m -Dwz-path=wz -jar target/Cosmic.jar
   ```
4. **Regenerate the manifest** if you added/removed a quest script:
   ```bash
   python3 scripts/extract_quest_data.py
   ```
5. **Re-run the test suite** (`full_test_suite.js`) or the single-quest workflow to verify the change.

### Maple Island NPC & Quest Reference

NPCs placed on Maple Island maps (from WZ data):

| Map ID | NPC (ID) | Script | Associated Quests |
|---|---|---|---|
| 10000 | Heena (2101), Sera (2100), MapleAdmin (2007) | 2101.js, 2100.js, 2007.js | 1000/1001 (Sera↔Heena mirror) |
| 20000 | Roger (2000) | 2000.js (created) | 1021 (Roger's Apple, scripted) |
| 20001 | Sen (2001) | 2001.js | 1003/1004 (Nina↔Sen food) |
| 30000 | Nina (2102) | 2102.js | 1003/1004 |
| 30001 | Sen (2001) | 2001.js | 1003/1004 |
| 40000 | Peter (2002), Todd (2004) | 2002.js (created), 2004.js | 1018 (Todd's How-to-Hunt) |
| 50000 | Sam (2005), Robin (2003) | 2005.js (created), 2003.js | 1019 (Sam's Suggestion), 1029 (Sam's Advice), 1036 (Robin) |
| 50001 | Sen (2001) | 2001.js | 1003/1004 |
| 60000 | Biggs (20002), Shanks (22000), MapleAdmin (9010000), Paul (9000000) | 22000.js, 9000000.js | 1007 (Bigg's Collection), 1031 (Shanks sail) |
| 104000000 | Tienk (2006) | 2006.js (created) | 1047 (Designated Monster Effect) — Victoria Island, not Maple Island |

### Testing Notes

- **`!completequest` bypasses `canComplete()`**: It calls `forceCompleteQuest()`, skipping item/mob/level checks. Use it when mob-kill requirements can't be satisfied through automated gameplay. For verifying the *scripted end* dialogue (quest action 5), the player must actually satisfy `canComplete()` (e.g. hold item 2010007 for quest 1021) — `!completequest` won't trigger the end script.
- **Server restart required after script changes**: NPC and quest scripts load at startup. After creating or editing any file under `cosmic/scripts/`, restart the Cosmic server.
- **GM level**: The test character needs `gm >= 5` in the database to use `!warp`, `!item`, `!startquest`, etc. Set it via `UPDATE characters SET gm = 5 WHERE name = 'Aiq704b';` in MySQL.
- **Dialogue action mapping**: `maple_advance_dialogue(action)` clicks through dialogues. Action 1 = NEXT, 4 = YES, 6 = select-first + OK. `MapleAi.dialogueActionForMode(mode)` picks automatically.

## Agent Behavior

- Prefer `./scripts/build_wasm.sh` for client builds.
- Fall back to `./scripts/docker_build_wasm.sh` only when the local build is not possible.
- Prefer local deployment first when the environment supports it.
- Fall back to Docker deployment when local deployment is blocked by missing dependencies or services.
- Leave `assets/` untouched.
- When a task needs server-side behavior (NPC dialogs, quest logic, drops, warps), edit the Cosmic server under `cosmic/` and rebuild/restart it. NPC/quest scripts load at startup, so a restart is required after changes.
- Use Java 21 (`/usr/lib/jvm/java-21-openjdk/bin/java`) for the Cosmic server; newer JDKs crash the GraalVM JS engine.
- When testing quests or NPCs, follow the workflow in **Quest & NPC Testing** above: load `MapleAi`, satisfy requirements with `setupQuestRequirements`, then exercise the dialogue/quest action and verify status with `getQuestStatus`. Create missing NPC scripts under `cosmic/scripts/npc/` and restart the server before retesting.

## Coding Styles:

* Write generic code whenever possible.
* Never write unsafe code.
* Add meaningful comments
  * Don't just repeat the code in the comment, explain the reasoning behind it.
* Don't have any credit or whatever in the source files (like license, reminder of journey client) in any comment

## Task Completion
* Make sure the project builds.
* Build should have no warnings.

## Documentation
* Check `docs/ms-network-protocol.md` for the network protocol.
