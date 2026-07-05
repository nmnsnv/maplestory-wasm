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
    'maple_get_npc_count',
    'maple_get_npc_info',
  ];
  if (!MapleAi.hasCcall()) {
    throw new Error('WASM Module.ccall not available. Load client page before running script.');
  }
  for (const hook of requiredHooks) {
    if (!MapleAi.hasHook(hook)) {
      throw new Error(`${hook} hook unavailable. Rebuild reload WASM client.`);
    }
  }


  const parseMapId = (value, label) => {
    const mapId = Number(value);
    if (!Number.isInteger(mapId) || mapId < 0) {
      throw new Error(`${label} must be a non-negative integer map id, got ${value}`);
    }
    return mapId;
  };

  const explicitMapIds = (value) => {
    if (value === undefined || value === null || value === '') {
      return null;
    }
    const rawValues = Array.isArray(value)
      ? value
      : String(value).split(',').map((entry) => entry.trim()).filter(Boolean);
    if (rawValues.length === 0) {
      return [];
    }
    return rawValues.map((entry, index) => parseMapId(entry, `mapProbeMapIds[${index}]`));
  };

  const deriveMapIds = async (manifestUrl) => {
    const manifest = await MapleAi.loadJson(manifestUrl);
    const unique = new Set();
    for (const quest of Object.values(manifest)) {
      if (!quest?.hasScript) {
        continue;
      }
      for (const key of ['startNpcMaps', 'endNpcMaps']) {
        const maps = Array.isArray(quest[key]) ? quest[key] : [];
        for (const rawMapId of maps) {
          const mapId = Number(rawMapId);
          if (Number.isInteger(mapId) && mapId >= 0) {
            unique.add(mapId);
          }
        }
      }
    }
    return [...unique].sort((left, right) => left - right);
  };

  const manifestUrl = String(globalThis.mapProbeManifestUrl ?? '/scripts/quest_manifest.json');
  const providedMapIds = explicitMapIds(globalThis.mapProbeMapIds);
  const usingExplicitMapIds = providedMapIds !== null;
  const defaultLimit = usingExplicitMapIds ? 0 : 25;
  const limit = Number(globalThis.mapProbeLimit ?? defaultLimit);
  if (!Number.isInteger(limit) || limit < 0) {
    throw new Error(`mapProbeLimit must be a non-negative integer, got ${globalThis.mapProbeLimit}`);
  }

  let mapIds = usingExplicitMapIds ? providedMapIds : await deriveMapIds(manifestUrl);
  if (limit > 0) {
    mapIds = mapIds.slice(0, limit);
  }

  const timeoutMs = Number(globalThis.mapProbeTimeoutMs ?? 10000);
  const cooldownMs = Number(globalThis.mapProbeCooldownMs ?? 650);
  if (!Number.isFinite(timeoutMs) || timeoutMs < 0) {
    throw new Error(`mapProbeTimeoutMs must be non-negative, got ${globalThis.mapProbeTimeoutMs}`);
  }
  if (!Number.isFinite(cooldownMs) || cooldownMs < 0) {
    throw new Error(`mapProbeCooldownMs must be non-negative, got ${globalThis.mapProbeCooldownMs}`);
  }

  const report = {
    generatedAt: new Date().toISOString(),
    planned: mapIds.length,
    tested: 0,
    passed: 0,
    failed: 0,
    results: [],
  };

  for (const mapId of mapIds) {
    const result = {
      mapId,
      status: 'failed',
      player: null,
      npcs: [],
      error: '',
    };

    try {
      await MapleAi.warpToMap(mapId, { timeoutMs });
      await MapleAi.sleep(cooldownMs);
      result.player = MapleAi.getPlayerState();
      result.npcs = MapleAi.listNpcs();
      result.status = 'passed';
    } catch (error) {
      result.error = error instanceof Error ? error.message : String(error);
    }

    report.tested += 1;
    if (result.status === 'passed') {
      report.passed += 1;
    } else {
      report.failed += 1;
    }
    report.results.push(result);
    console.log(`[map-probe] ${result.status.toUpperCase()} ${mapId}: ${result.error || `${result.npcs.length} NPC(s)`}`);
  }

  globalThis.mapProbeReport = report;
  console.log(`[map-probe] Done: ${report.passed}/${report.tested} passed, ${report.failed} failed.`);

  if (globalThis.mapProbeDownload) {
    MapleAi.downloadJson(report, 'map_probe_report.json');
  }

  return report;
})();
