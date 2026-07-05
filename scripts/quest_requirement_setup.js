// quest_requirement_setup.js
// Automatically satisfies quest requirements using GM commands before testing.
// Reads quest requirements from /scripts/quest_requirements.json and uses
// GM chat commands to give items, set levels, start/complete prerequisite
// quests, heal HP/MP, and spawn+kill mobs.
//
// Usage in browser:
//   globalThis.questRequirementSetup = { questId: 1018, phase: 'complete' };
//   await eval(await (await fetch('/scripts/quest_requirement_setup.js')).text());
//
// Or call directly:
//   await globalThis.setupQuestRequirements(1018, 'complete');
//   await globalThis.setupQuestRequirements(1018, 'start');
//   await globalThis.setupAllQuestRequirements(1018);  // sets up both start+complete

(async () => {
  const MapleAi = globalThis.MapleAi ?? await (async () => {
    const resp = await fetch('/scripts/maple_ai_common.js', { cache: 'no-store' });
    globalThis.eval(await resp.text());
    return globalThis.MapleAi;
  })();

  if (!MapleAi?.hasCcall()) {
    throw new Error('WASM Module.ccall not available. Load client page before running script.');
  }

  const sleep = (ms) => new Promise((r) => setTimeout(r, ms));
  const chat = (msg) => MapleAi.sendChat(msg);
  const questStatus = (id) => MapleAi.getQuestStatus(id);

  // GM command wrappers with cooldown
  async function gmItem(itemId, quantity = 1) {
    chat(`!item ${itemId} ${quantity}`);
    await sleep(800);
  }

  async function gmLevel(level) {
    chat(`!level ${level}`);
    await sleep(1200);
  }

  async function gmHeal() {
    chat('!heal');
    await sleep(600);
  }

  async function gmHpMp(value) {
    chat(`!hpmp ${value}`);
    await sleep(600);
  }

  async function gmStartQuest(questId) {
    if (questStatus(questId) === 0) {
      chat(`!startquest ${questId}`);
      await sleep(800);
    }
  }

  async function gmCompleteQuest(questId) {
    if (questStatus(questId) === 1) {
      chat(`!completequest ${questId}`);
      await sleep(800);
    }
  }

  async function gmResetQuest(questId) {
    if (questStatus(questId) !== 0) {
      chat(`!resetquest ${questId}`);
      await sleep(800);
    }
  }

  async function gmSpawnMob(mobId, quantity = 1) {
    chat(`!spawn ${mobId} ${quantity}`);
    await sleep(1000);
  }

  async function gmKillAll() {
    chat('!killall');
    await sleep(1000);
  }

  // Spawn a mob, kill it, and pick up its drop.
  // This is the hardest requirement to satisfy — mob kills register on the
  // server when the player deals the killing blow. For automated testing we
  // spawn the mob, use !killall (which credits the kill to the player), and
  // then wait for the drop to be picked up.
  async function satisfyMobKill(mobId, count) {
    // Spawn the required number of mobs
    await gmSpawnMob(mobId, count);
    // Kill them — !killall credits kills to the player
    await gmKillAll();
    // Wait for drops to settle
    await sleep(500);
  }

  // Give all items required by a requirement state
  async function satisfyItems(items) {
    if (!items || items.length === 0) return;
    for (const item of items) {
      // Only give items with positive count (negative = consume/remove)
      if (item.count > 0) {
        await gmItem(item.id, item.count);
      }
    }
  }

  // Satisfy quest prerequisites
  async function satisfyQuestPrereqs(prereqs) {
    if (!prereqs || prereqs.length === 0) return;
    for (const pq of prereqs) {
      const currentStatus = questStatus(pq.id);
      if (pq.state === 2 && currentStatus !== 2) {
        // Need prerequisite quest completed
        if (currentStatus === 0) {
          await gmStartQuest(pq.id);
        }
        await gmCompleteQuest(pq.id);
      } else if (pq.state === 1 && currentStatus === 0) {
        // Need prerequisite quest started
        await gmStartQuest(pq.id);
      } else if (pq.state === 0 && currentStatus !== 0) {
        // Need prerequisite quest not started (reset it)
        await gmResetQuest(pq.id);
      }
    }
  }

  // Satisfy level requirements
  async function satisfyLevel(req) {
    const currentLevel = MapleAi.call('maple_get_player_level');
    if (req.levelMin && currentLevel < req.levelMin) {
      await gmLevel(req.levelMin);
    }
    // levelMax is a hard cap — if player is too high, we can't lower easily
    // without losing progress. Log a warning but don't force it.
    if (req.levelMax && currentLevel > req.levelMax) {
      console.warn(`[quest-req-setup] Player level ${currentLevel} exceeds max ${req.levelMax} — quest may not be available.`);
    }
  }

  // Satisfy HP requirement (some quests like 1021 need HP at a specific level)
  async function satisfyHp(req) {
    // No explicit HP requirement in WZ check data, but quest scripts may check
    // For quest 1021, the end script checks HP >= 50
    // We heal to full as a safe default
    await gmHeal();
  }

  // Main function: set up all requirements for a quest phase
  async function setupQuestRequirements(questId, phase = 'start') {
    const id = String(questId);
    const resp = await fetch('/scripts/quest_requirements.json', { cache: 'no-store' });
    const allReqs = await resp.json();
    const quest = allReqs[id];
    if (!quest) {
      throw new Error(`Quest ${questId} not found in quest_requirements.json`);
    }

    const req = quest[phase];
    if (!req) {
      throw new Error(`Quest ${questId} has no ${phase} requirements`);
    }

    const log = [];
    const logStep = (msg) => { log.push(msg); console.log(`[quest-req-setup] ${msg}`); };

    logStep(`Setting up ${phase} requirements for quest ${questId}`);

    // 1. Satisfy quest prerequisites first
    if (req.quests && req.quests.length > 0) {
      logStep(`  Prerequisite quests: ${req.quests.map(q => `${q.id}(state=${q.state})`).join(', ')}`);
      await satisfyQuestPrereqs(req.quests);
    }

    // 2. Satisfy level requirements
    if (req.levelMin || req.levelMax) {
      logStep(`  Level requirement: min=${req.levelMin || 'none'}, max=${req.levelMax || 'none'}`);
      await satisfyLevel(req);
    }

    // 3. Give required items
    if (req.items && req.items.length > 0) {
      logStep(`  Items: ${req.items.map(i => `${i.id}x${i.count}`).join(', ')}`);
      await satisfyItems(req.items);
    }

    // 4. Satisfy mob kill requirements (for complete phase)
    if (req.mobs && req.mobs.length > 0) {
      logStep(`  Mob kills: ${req.mobs.map(m => `${m.id}x${m.count}`).join(', ')}`);
      await satisfyMobKill(req.mobs[0].id, req.mobs[0].count);
    }

    // 5. Heal to full HP/MP
    await satisfyHp(req);

    // 6. Report current quest status
    const status = questStatus(questId);
    logStep(`  Quest ${questId} status after setup: ${status} (0=notStarted, 1=started, 2=completed)`);

    return { questId: Number(questId), phase, log, questStatus: status };
  }

  // Set up both start and complete requirements for a full quest test
  async function setupAllQuestRequirements(questId) {
    const startResult = await setupQuestRequirements(questId, 'start');
    const completeResult = await setupQuestRequirements(questId, 'complete');
    return { start: startResult, complete: completeResult };
  }

  // Export
  globalThis.setupQuestRequirements = setupQuestRequirements;
  globalThis.setupAllQuestRequirements = setupAllQuestRequirements;

  // Autorun if configured
  const autorun = globalThis.questRequirementSetup;
  if (autorun) {
    return setupQuestRequirements(autorun.questId, autorun.phase || 'start');
  }

  return { loaded: true, setupQuestRequirements, setupAllQuestRequirements };
})();
