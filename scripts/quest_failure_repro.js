(async () => {
  const loadMapleAi = async () => {
    if (globalThis.MapleAi?.version >= 2) return globalThis.MapleAi;
    const response = await fetch('/scripts/maple_ai_common.js', { cache: 'no-store' });
    if (!response.ok) throw new Error(`Could not load maple_ai_common.js: HTTP ${response.status}`);
    await globalThis.eval(await response.text());
    return globalThis.MapleAi;
  };

  const MapleAi = await loadMapleAi();
  const requiredHooks = [
    'maple_send_chat',
    'maple_send_quest_action',
    'maple_get_player_mapid',
    'maple_warp_to_map',
    'maple_teleport_to_npc',
    'maple_get_dialogue_mode',
    'maple_get_dialogue_npcid',
    'maple_get_dialogue_type',
    'maple_get_dialogue_text',
    'maple_get_dialogue_selection_count',
    'maple_get_dialogue_selection',
    'maple_advance_dialogue',
  ];
  if (!MapleAi.hasCcall()) {
    throw new Error('WASM Module.ccall not available. Load client page before running script.');
  }
  for (const hook of requiredHooks) {
    if (!MapleAi.hasHook(hook)) {
      throw new Error(`${hook} hook unavailable. Rebuild reload WASM client.`);
    }
  }

  const parseQuestIds = (value) => {
    if (value === undefined || value === null || value === '') {
      return null;
    }
    const rawValues = Array.isArray(value)
      ? value
      : String(value).split(',').map((entry) => entry.trim()).filter(Boolean);
    return rawValues.map((rawValue, index) => {
      const questId = String(rawValue).trim();
      if (!questId) {
        throw new Error(`questFailureIds[${index}] must be non-empty`);
      }
      return questId;
    });
  };

  const deriveQuestIds = async (reportUrl, limit) => {
    const sourceReport = await MapleAi.loadJson(reportUrl);
    const failures = Array.isArray(sourceReport.failures) ? sourceReport.failures : [];
    let ids = failures
      .map((failure) => failure?.questId)
      .filter((questId) => questId !== undefined && questId !== null)
      .map(String);
    if (limit > 0) {
      ids = ids.slice(0, limit);
    }
    return ids;
  };

  const loadQuestRunner = async () => {
    if (typeof globalThis.runQuestTests === 'function') {
      return;
    }

    const previousAutorun = globalThis.questTestAutorun;
    globalThis.questTestAutorun = false;
    try {
      await MapleAi.loadBrowserScript('/scripts/quest_test_runner.js');
    } finally {
      globalThis.questTestAutorun = previousAutorun;
    }

    if (typeof globalThis.runQuestTests !== 'function') {
      throw new Error('quest_test_runner.js loaded but did not expose runQuestTests().');
    }
  };

  const explicitIds = parseQuestIds(globalThis.questFailureIds);
  const usingExplicitIds = explicitIds !== null;
  const failureLimit = Number(globalThis.questFailureLimit ?? 5);
  if (!Number.isInteger(failureLimit) || failureLimit < 0) {
    throw new Error(`questFailureLimit must be a non-negative integer, got ${globalThis.questFailureLimit}`);
  }

  const reportUrl = String(globalThis.questFailureReportUrl ?? '/scripts/quest_test_report.json');
  const manifestUrl = String(globalThis.questFailureManifestUrl ?? '/scripts/quest_manifest.json');
  const questIds = usingExplicitIds ? explicitIds : await deriveQuestIds(reportUrl, failureLimit);
  if (questIds.length === 0) {
    const skipped = {
      status: 'skipped',
      reason: 'No failed quest IDs found.',
      questIds: [],
    };
    globalThis.questFailureReproReport = skipped;
    console.log('[quest-failure-repro] Skipped: No failed quest IDs found.');
    return skipped;
  }

  await loadQuestRunner();

  const dialogueTimeoutMs = Number(globalThis.questFailureDialogueTimeoutMs ?? 5000);
  const nextDialogueTimeoutMs = Number(globalThis.questFailureNextDialogueTimeoutMs ?? 2000);
  const maxDialogueSteps = Number(globalThis.questFailureMaxDialogueSteps ?? 75);
  for (const [name, value] of Object.entries({ dialogueTimeoutMs, nextDialogueTimeoutMs, maxDialogueSteps })) {
    if (!Number.isFinite(value) || value < 0) {
      throw new Error(`${name} must be non-negative, got ${value}`);
    }
  }
  if (!Number.isInteger(maxDialogueSteps) || maxDialogueSteps < 1) {
    throw new Error(`maxDialogueSteps must be a positive integer, got ${maxDialogueSteps}`);
  }

  const questReport = await globalThis.runQuestTests({
    manifestUrl,
    questIds,
    dialogueTimeoutMs,
    nextDialogueTimeoutMs,
    maxDialogueSteps,
    downloadReport: false,
  });

  const report = {
    generatedAt: new Date().toISOString(),
    questIds,
    planned: questIds.length,
    tested: questReport.tested ?? 0,
    passed: questReport.passed ?? 0,
    failed: questReport.failed ?? 0,
    failures: Array.isArray(questReport.failures) ? questReport.failures : [],
    results: Array.isArray(questReport.results) ? questReport.results : [],
  };

  globalThis.questFailureReproReport = report;
  console.log(`[quest-failure-repro] Complete: ${report.passed}/${report.tested} passed, ${report.failed} failed.`);

  if (globalThis.questFailureDownload) {
    MapleAi.downloadJson(report, 'quest_failure_repro_report.json');
  }

  return report;
})();
