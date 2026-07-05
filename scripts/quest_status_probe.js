(async () => {
  const loadMapleAi = async () => {
    if (globalThis.MapleAi?.version >= 2) return globalThis.MapleAi;
    const response = await fetch('/scripts/maple_ai_common.js', { cache: 'no-store' });
    if (!response.ok) throw new Error(`Could not load maple_ai_common.js: HTTP ${response.status}`);
    await globalThis.eval(await response.text());
    return globalThis.MapleAi;
  };

  const MapleAi = await loadMapleAi();
  const INT16_MIN = -32768;
  const INT16_MAX = 32767;

  const statusLabel = (status) => {
    switch (status) {
      case 0:
        return 'notStarted';
      case 1:
        return 'started';
      case 2:
        return 'completed';
      default:
        return 'unknown';
    }
  };

  const normalizeQuestId = (value, label) => {
    const text = String(value ?? '').trim();
    if (!text) {
      throw new Error(`${label} must be a non-empty quest id`);
    }
    const questId = Number(text);
    if (!Number.isInteger(questId) || questId < INT16_MIN || questId > INT16_MAX) {
      throw new Error(`${label} must be an int16 quest id, got ${value}`);
    }
    return questId;
  };

  const parseQuestIds = (value) => {
    if (value === undefined || value === null || value === '') {
      return null;
    }
    const rawValues = Array.isArray(value)
      ? value
      : String(value).split(',').map((entry) => entry.trim()).filter(Boolean);
    return rawValues.map((entry, index) => normalizeQuestId(entry, `questStatusIds[${index}]`));
  };

  const deriveQuestIds = async (manifestUrl) => {
    const manifest = await MapleAi.loadJson(manifestUrl);
    return Object.entries(manifest)
      .filter(([, quest]) => quest?.hasScript)
      .map(([questId]) => Number(questId))
      .filter((questId) => Number.isInteger(questId) && questId >= INT16_MIN && questId <= INT16_MAX)
      .sort((left, right) => left - right);
  };

  async function runQuestStatusProbe(overrides = {}) {
    if (!MapleAi.hasCcall()) {
      throw new Error('WASM Module.ccall not available. Load client page before running script.');
    }
    if (!MapleAi.hasHook('maple_get_quest_status')) {
      throw new Error('maple_get_quest_status hook unavailable. Rebuild reload WASM client.');
    }

    const explicitIds = parseQuestIds(overrides.questIds ?? globalThis.questStatusIds);
    const usingExplicitIds = explicitIds !== null;
    const manifestUrl = String(overrides.manifestUrl ?? globalThis.questStatusManifestUrl ?? '/scripts/quest_manifest.json');
    const defaultLimit = usingExplicitIds ? 0 : 100;
    const limit = Number(overrides.limit ?? globalThis.questStatusLimit ?? defaultLimit);
    if (!Number.isInteger(limit) || limit < 0) {
      throw new Error(`questStatusLimit must be a non-negative integer, got ${overrides.limit ?? globalThis.questStatusLimit}`);
    }

    let questIds = usingExplicitIds ? explicitIds : await deriveQuestIds(manifestUrl);
    if (limit > 0) {
      questIds = questIds.slice(0, limit);
    }

    const results = questIds.map((questId) => {
      try {
        const status = MapleAi.call('maple_get_quest_status', 'number', ['number'], [questId]);
        return { questId, status, label: statusLabel(status), error: '' };
      } catch (error) {
        return {
          questId,
          status: null,
          label: 'unknown',
          error: error instanceof Error ? error.message : String(error),
        };
      }
    });

    const report = {
      generatedAt: new Date().toISOString(),
      planned: questIds.length,
      tested: results.length,
      failed: results.filter((result) => result.error).length,
      results,
    };

    globalThis.questStatusProbeReport = report;
    console.log(`[quest-status-probe] Complete: ${report.tested - report.failed}/${report.tested} readable, ${report.failed} failed.`);

    const shouldDownload = Boolean(overrides.download ?? globalThis.questStatusDownload);
    if (shouldDownload) {
      MapleAi.downloadJson(report, 'quest_status_probe_report.json');
    }

    return report;
  }

  globalThis.runQuestStatusProbe = runQuestStatusProbe;

  const autorun = globalThis.questStatusProbeAutorun ?? true;
  if (!autorun) {
    return { loaded: true, runQuestStatusProbe };
  }

  return runQuestStatusProbe();
})();
