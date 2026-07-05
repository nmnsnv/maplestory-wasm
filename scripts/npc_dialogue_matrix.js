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
    'maple_warp_to_map',
    'maple_send_chat',
    'maple_get_player_mapid',
    'maple_teleport_to_npc',
    'maple_talk_to_nearest_npc',
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

  const normalizeMapId = (value, label) => {
    const mapId = Number(value);
    if (!Number.isInteger(mapId) || mapId < 0) {
      throw new Error(`${label} must be a non-negative integer map id, got ${value}`);
    }
    return mapId;
  };

  const normalizeTarget = (target, index) => {
    if (!target || typeof target !== 'object') {
      throw new Error(`npcDialogueTargets[${index}] must be an object`);
    }
    const name = String(target.name ?? '').trim();
    if (!name) {
      throw new Error(`npcDialogueTargets[${index}].name must be non-empty`);
    }

    const normalized = {
      name,
      mapId: normalizeMapId(target.mapId, `npcDialogueTargets[${index}].mapId`),
    };
    if (target.npcId !== undefined && target.npcId !== null && target.npcId !== '') {
      const npcId = Number(target.npcId);
      if (!Number.isInteger(npcId)) {
        throw new Error(`npcDialogueTargets[${index}].npcId must be an integer, got ${target.npcId}`);
      }
      normalized.npcId = npcId;
    }
    if (target.questId !== undefined && target.questId !== null) {
      normalized.questId = String(target.questId);
    }
    if (Array.isArray(target.questIds)) {
      normalized.questIds = target.questIds.map(String);
    } else if (normalized.questId !== undefined) {
      normalized.questIds = [normalized.questId];
    }
    if (target.questName !== undefined && target.questName !== null) {
      normalized.questName = String(target.questName);
    }
    return normalized;
  };

  const deriveTargets = async (manifestUrl) => {
    const manifest = await MapleAi.loadJson(manifestUrl);
    const targetsByKey = new Map();
    for (const [questId, quest] of Object.entries(manifest)) {
      if (!quest?.hasScript) {
        continue;
      }
      const name = String(quest.startNpcName ?? '').trim();
      const mapId = Number(Array.isArray(quest.startNpcMaps) ? quest.startNpcMaps[0] : NaN);
      if (!name || !Number.isInteger(mapId) || mapId < 0) {
        continue;
      }

      const key = `${name}\u0000${mapId}`;
      const existing = targetsByKey.get(key);
      if (existing) {
        existing.questIds.push(String(questId));
        if (quest.name !== undefined && quest.name !== null) {
          existing.questNames.push(String(quest.name));
        }
        continue;
      }

      targetsByKey.set(key, {
        name,
        mapId,
        npcId: Number.isInteger(quest.startNpc) ? quest.startNpc : undefined,
        questIds: [String(questId)],
        questNames: quest.name !== undefined && quest.name !== null ? [String(quest.name)] : [],
      });
    }
    return [...targetsByKey.values()].sort((left, right) => (
      left.name.localeCompare(right.name) || left.mapId - right.mapId
    ));
  };

  const explicitTargets = globalThis.npcDialogueTargets;
  const usingExplicitTargets = explicitTargets !== undefined && explicitTargets !== null;
  if (usingExplicitTargets && !Array.isArray(explicitTargets)) {
    throw new Error('npcDialogueTargets must be an array of { name, mapId, npcId?, questId?, questName? }');
  }

  const manifestUrl = String(globalThis.npcDialogueManifestUrl ?? '/scripts/quest_manifest.json');
  const defaultLimit = usingExplicitTargets ? 0 : 10;
  const limit = Number(globalThis.npcDialogueLimit ?? defaultLimit);
  if (!Number.isInteger(limit) || limit < 0) {
    throw new Error(`npcDialogueLimit must be a non-negative integer, got ${globalThis.npcDialogueLimit}`);
  }

  let targets = usingExplicitTargets
    ? explicitTargets.map((target, index) => normalizeTarget(target, index))
    : await deriveTargets(manifestUrl);
  if (limit > 0) {
    targets = targets.slice(0, limit);
  }

  const dialogueTimeoutMs = Number(globalThis.npcDialogueTimeoutMs ?? 5000);
  const nextTimeoutMs = Number(globalThis.npcDialogueNextTimeoutMs ?? 1500);
  const dialogueMaxSteps = Number(globalThis.npcDialogueMaxSteps ?? 50);
  const mapTimeoutMs = Number(globalThis.npcDialogueMapTimeoutMs ?? 10000);
  const cooldownMs = Number(globalThis.npcDialogueCooldownMs ?? 650);
  for (const [name, value] of Object.entries({ dialogueTimeoutMs, nextTimeoutMs, dialogueMaxSteps, mapTimeoutMs, cooldownMs })) {
    if (!Number.isFinite(value) || value < 0) {
      throw new Error(`${name} must be non-negative, got ${value}`);
    }
  }
  if (!Number.isInteger(dialogueMaxSteps) || dialogueMaxSteps < 1) {
    throw new Error(`dialogueMaxSteps must be a positive integer, got ${dialogueMaxSteps}`);
  }

  const report = {
    generatedAt: new Date().toISOString(),
    planned: targets.length,
    tested: 0,
    passed: 0,
    failed: 0,
    results: [],
  };

  for (const target of targets) {
    const result = {
      target,
      status: 'failed',
      dialoguePages: [],
      dialogueSteps: [],
      error: '',
    };

    try {
      await MapleAi.warpToMap(target.mapId, { timeoutMs: mapTimeoutMs });
      await MapleAi.sleep(cooldownMs);

      const teleported = MapleAi.teleportToNpc(target.name);
      if (!teleported) {
        throw new Error(`Could not find active NPC "${target.name}" on map ${target.mapId}`);
      }
      await MapleAi.sleep(cooldownMs);

      const talked = MapleAi.talkToNearestNpc();
      if (!talked) {
        throw new Error('No nearby scripted NPC responded talk request.');
      }

      const dialoguePages = await MapleAi.captureDialogue({
        initialTimeoutMs: dialogueTimeoutMs,
        nextTimeoutMs,
        maxSteps: dialogueMaxSteps,
      });
      if (dialoguePages.length === 0) {
        throw new Error('NPC dialogue opened but no pages were captured.');
      }

      result.dialoguePages = dialoguePages;
      result.dialogueSteps = dialoguePages.map((page) => page.text);
      result.status = 'passed';
    } catch (error) {
      result.error = error instanceof Error ? error.message : String(error);
    } finally {
      await MapleAi.dispose(cooldownMs);
    }

    report.tested += 1;
    if (result.status === 'passed') {
      report.passed += 1;
    } else {
      report.failed += 1;
    }
    report.results.push(result);
    console.log(`[npc-dialogue-matrix] ${result.status.toUpperCase()} ${target.name} @ ${target.mapId}: ${result.error || `${result.dialoguePages.length} page(s)`}`);
  }

  globalThis.npcDialogueMatrixReport = report;
  console.log(`[npc-dialogue-matrix] Done: ${report.passed}/${report.tested} passed, ${report.failed} failed.`);

  if (globalThis.npcDialogueDownload) {
    MapleAi.downloadJson(report, 'npc_dialogue_matrix_report.json');
  }

  return report;
})();
