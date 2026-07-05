(async () => {
  const module = globalThis.Module;

  const DEFAULTS = {
    manifestUrl: '/scripts/quest_manifest.json',
    dialogueTimeoutMs: 5000,
    nextDialogueTimeoutMs: 1500,
    mapTimeoutMs: 10000,
    npcCooldownMs: 650,
    maxDialogueSteps: 50,
    limit: null,
    questIds: null,
    downloadReport: false,
  };

  const sleep = (ms) => new Promise((resolve) => setTimeout(resolve, ms));
  const hasCcall = () => module && typeof module.ccall === 'function';
  const hasHook = (name) => Boolean(module && typeof module[`_${name}`] === 'function');
  const call = (name, returnType = 'number', argTypes = [], args = []) => {
    if (!hasCcall()) {
      throw new Error('WASM Module.ccall is not available. Load the client page before running this script.');
    }
    if (!hasHook(name)) {
      throw new Error(`${name} hook is unavailable. Rebuild and reload the WASM client.`);
    }
    return module.ccall(name, returnType, argTypes, args);
  };

  const sendChat = (message) => call('maple_send_chat', 'number', ['string'], [message]) !== 0;
  const sendQuestAction = (action, questId, npcId) => call(
    'maple_send_quest_action',
    'number',
    ['number', 'number', 'number'],
    [action, Number(questId), Number(npcId)],
  ) !== 0;
  const getMapId = () => call('maple_get_player_mapid');
  const getQuestStatus = (questId) => {
    if (!hasCcall() || !hasHook('maple_get_quest_status')) {
      return null;
    }
    return call('maple_get_quest_status', 'number', ['number'], [Number(questId)]);
  };
  const dialogueMode = () => call('maple_get_dialogue_mode');

  const normalizeQuestIds = (value) => {
    if (!value) return null;
    if (Array.isArray(value)) return new Set(value.map(String));
    return new Set(String(value).split(',').map((id) => id.trim()).filter(Boolean));
  };

  const loadManifest = async (manifestUrl) => {
    const response = await fetch(manifestUrl, { cache: 'no-store' });
    if (!response.ok) {
      throw new Error(`Could not load quest manifest from ${manifestUrl}: HTTP ${response.status}`);
    }
    return response.json();
  };

  const waitForMap = async (mapId, timeoutMs) => {
    const deadline = Date.now() + timeoutMs;
    while (Date.now() < deadline) {
      if (getMapId() === mapId) {
        return true;
      }
      await sleep(100);
    }
    return getMapId() === mapId;
  };

  const waitForDialogue = async (timeoutMs) => {
    const deadline = Date.now() + timeoutMs;
    while (Date.now() < deadline) {
      const mode = dialogueMode();
      if (mode !== 0) {
        return mode;
      }
      await sleep(100);
    }
    return 0;
  };

  const readSelections = () => {
    const count = call('maple_get_dialogue_selection_count');
    const selections = [];
    for (let index = 0; index < count; index += 1) {
      selections.push(call('maple_get_dialogue_selection', 'string', ['number'], [index]) ?? '');
    }
    return selections;
  };

  const dialogueActionForMode = (mode) => {
    if (mode === 4) return 6;          // deterministic first selection
    if (mode === 2 || mode === 3) return 4; // yes / accept
    if (mode === 5) return 0;          // unknown prompt, try OK
    return 1;                          // text prompts advance with NEXT semantics
  };

  const captureDialogue = async (options) => {
    const steps = [];
    let mode = await waitForDialogue(options.dialogueTimeoutMs);
    if (mode === 0) {
      throw new Error(`NPC did not respond with dialogue after ${options.dialogueTimeoutMs}ms`);
    }

    for (let step = 0; step < options.maxDialogueSteps; step += 1) {
      mode = dialogueMode();
      if (mode === 0) {
        break;
      }

      const page = {
        step,
        mode,
        npcId: call('maple_get_dialogue_npcid'),
        type: call('maple_get_dialogue_type'),
        text: call('maple_get_dialogue_text', 'string', [], []) ?? '',
        selections: readSelections(),
      };
      steps.push(page);

      const advanced = call('maple_advance_dialogue', 'number', ['number'], [dialogueActionForMode(mode)]) !== 0;
      if (!advanced) {
        break;
      }

      await sleep(150);
      const nextMode = await waitForDialogue(options.nextDialogueTimeoutMs);
      if (nextMode === 0) {
        break;
      }
    }

    if (steps.length >= options.maxDialogueSteps) {
      throw new Error(`Dialogue exceeded ${options.maxDialogueSteps} steps`);
    }

    return steps;
  };

  const warpToMap = async (mapId, options) => {
    let dispatched = call('maple_warp_to_map', 'number', ['number'], [mapId]) !== 0;
    if (!dispatched) {
      dispatched = sendChat(`!warp ${mapId}`);
    }
    if (!dispatched) {
      throw new Error(`Could not dispatch warp to map ${mapId}`);
    }
    if (!await waitForMap(mapId, options.mapTimeoutMs)) {
      throw new Error(`Timed out waiting for map ${mapId}; current map is ${getMapId()}`);
    }
  };

  const runCommand = async (message, options) => {
    if (!sendChat(message)) {
      throw new Error(`Could not dispatch command: ${message}`);
    }
    await sleep(options.npcCooldownMs);
  };

  const prepareStartRequirements = async (questId, quest, options) => {
    const requirements = quest.startRequirements ?? {};
    const jobs = Array.isArray(requirements.jobs) ? requirements.jobs : [];
    if (jobs.length > 0) {
      await runCommand(`!job ${jobs[0]}`, options);
    }

    if (Number.isInteger(requirements.levelMin) && requirements.levelMin > 0) {
      await runCommand(`!level ${requirements.levelMin}`, options);
    }

    const requiredQuests = Array.isArray(requirements.quests) ? requirements.quests : [];
    for (const requirement of requiredQuests) {
      const requiredId = Number(requirement.id);
      const state = Number(requirement.state ?? 0);
      if (!Number.isInteger(requiredId)) {
        continue;
      }
      if (state === 2) {
        await runCommand(`!startquest ${requiredId}`, options);
        await runCommand(`!completequest ${requiredId}`, options);
      } else if (state === 1) {
        await runCommand(`!startquest ${requiredId}`, options);
      } else {
        await runCommand(`!resetquest ${requiredId}`, options);
      }
    }

    await runCommand(`!resetquest ${questId}`, options);
  };

  const testQuest = async (questId, quest, options) => {
    const result = {
      questId,
      questName: quest.name,
      startNpc: quest.startNpcName || String(quest.startNpc ?? ''),
      startNpcId: quest.startNpc,
      mapId: quest.startNpcMaps?.[0] ?? null,
      status: 'failed',
      questStatusBefore: null,
      questStatusAfterOpen: null,
      questStatusAfterDispose: null,
      dialogueSteps: [],
      error: '',
    };

    try {
      await prepareStartRequirements(questId, quest, options);
      result.questStatusBefore = getQuestStatus(questId);

      if (!Array.isArray(quest.startNpcMaps) || quest.startNpcMaps.length === 0) {
        throw new Error('Quest start NPC has no indexed map');
      }
      if (!quest.startNpcName) {
        throw new Error('Quest start NPC name is missing');
      }
      if (!Number.isInteger(quest.startNpc)) {
        throw new Error('Quest start NPC id is missing');
      }

      await warpToMap(result.mapId, options);
      await sleep(options.npcCooldownMs);

      const teleported = call('maple_teleport_to_npc', 'number', ['string'], [quest.startNpcName]) !== 0;
      if (!teleported) {
        throw new Error(`Could not find active NPC "${quest.startNpcName}" on map ${result.mapId}`);
      }
      // Let the normal player-update path send the local test position before
      // opening the quest; Cosmic validates scripted quest actions against NPC range.
      await sleep(options.npcCooldownMs);

      const opened = sendQuestAction(4, questId, quest.startNpc);
      if (!opened) {
        throw new Error(`Could not dispatch scripted start for quest ${questId}`);
      }
      result.questStatusAfterOpen = getQuestStatus(questId);

      const dialogue = await captureDialogue(options);
      result.dialogueSteps = dialogue.map((page) => page.text);
      result.dialoguePages = dialogue;
      result.status = 'passed';
      return result;
    } catch (error) {
      result.error = error instanceof Error ? error.message : String(error);
      return result;
    } finally {
      try {
        sendChat('!dispose');
      } catch (error) {
        console.warn('[quest-runner] Failed to send !dispose:', error);
      }
      await sleep(options.npcCooldownMs);
      result.questStatusAfterDispose = getQuestStatus(questId);
    }
  };

  const prepareQuestList = (manifest, options) => {
    const requestedIds = normalizeQuestIds(options.questIds);
    let quests = Object.entries(manifest)
      .filter(([, quest]) => quest && quest.hasScript)
      .filter(([questId]) => !requestedIds || requestedIds.has(String(questId)))
      .sort((left, right) => {
        const leftName = String(left[1].name ?? '');
        const rightName = String(right[1].name ?? '');
        return leftName.localeCompare(rightName) || String(left[0]).localeCompare(String(right[0]));
      });

    if (Number.isInteger(options.limit) && options.limit > 0) {
      quests = quests.slice(0, options.limit);
    }
    return quests;
  };

  const downloadReport = (report) => {
    const blob = new Blob([JSON.stringify(report, null, 2), '\n'], { type: 'application/json' });
    const url = URL.createObjectURL(blob);
    const link = document.createElement('a');
    link.href = url;
    link.download = 'quest_test_report.json';
    link.click();
    URL.revokeObjectURL(url);
  };

  async function runQuestTests(overrides = {}) {
    const options = { ...DEFAULTS, ...(globalThis.questTestOptions ?? {}), ...overrides };
    if (options.limit !== null) {
      options.limit = Number(options.limit);
    }

    const manifest = await loadManifest(options.manifestUrl);
    const quests = prepareQuestList(manifest, options);
    const report = {
      generatedAt: new Date().toISOString(),
      totalQuests: Object.values(manifest).filter((quest) => quest && quest.hasScript).length,
      planned: quests.length,
      tested: 0,
      passed: 0,
      failed: 0,
      failures: [],
      results: [],
    };
    globalThis.questTestProgress = {
      planned: report.planned,
      tested: 0,
      passed: 0,
      failed: 0,
      currentQuestId: null,
      currentQuestName: '',
      done: false,
    };

    console.log(`[quest-runner] Testing ${quests.length} scripted quest(s).`);
    for (const [index, [questId, quest]] of quests.entries()) {
      globalThis.questTestProgress.currentQuestId = questId;
      globalThis.questTestProgress.currentQuestName = quest.name ?? '';
      console.log(`[quest-runner] ${index + 1}/${quests.length}: ${questId} ${quest.name}`);
      const result = await testQuest(questId, quest, options);
      report.tested += 1;
      report.results.push(result);
      if (result.status === 'passed') {
        report.passed += 1;
      } else {
        report.failed += 1;
        report.failures.push({
          questId: result.questId,
          questName: result.questName,
          startNpc: result.startNpc,
          mapId: result.mapId,
          error: result.error,
          questStatusBefore: result.questStatusBefore,
          questStatusAfterOpen: result.questStatusAfterOpen,
          questStatusAfterDispose: result.questStatusAfterDispose,
          dialogueCaptured: result.dialogueSteps,
        });
      }
      globalThis.questTestProgress.tested = report.tested;
      globalThis.questTestProgress.passed = report.passed;
      globalThis.questTestProgress.failed = report.failed;
      console.log(`[quest-runner] ${result.status.toUpperCase()} ${questId}: ${result.error || `${result.dialogueSteps.length} dialogue page(s)`}`);
    }

    globalThis.questTestProgress.currentQuestId = null;
    globalThis.questTestProgress.currentQuestName = '';
    globalThis.questTestProgress.done = true;
    globalThis.questTestReport = report;
    globalThis.questTestReportJson = `${JSON.stringify(report, null, 2)}\n`;
    if (options.downloadReport) {
      downloadReport(report);
    }

    console.log(`[quest-runner] Done: ${report.passed}/${report.tested} passed, ${report.failed} failed.`);
    return report;
  }

  globalThis.runQuestTests = runQuestTests;

  const autorun = globalThis.questTestAutorun ?? true;
  if (!autorun) {
    return { loaded: true, runQuestTests };
  }

  return runQuestTests();
})();
