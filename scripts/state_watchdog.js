(async () => {
  const loadMapleAi = async () => {
    if (globalThis.MapleAi?.version >= 2) return globalThis.MapleAi;
    const response = await fetch('/scripts/maple_ai_common.js', { cache: 'no-store' });
    if (!response.ok) throw new Error(`Could not load maple_ai_common.js: HTTP ${response.status}`);
    await globalThis.eval(await response.text());
    return globalThis.MapleAi;
  };

  const MapleAi = await loadMapleAi();

  const numberOption = (value, fallback, name, { integer = false, min = 0 } = {}) => {
    const raw = value ?? fallback;
    const normalized = Number(raw);
    if (!Number.isFinite(normalized) || normalized < min || (integer && !Number.isInteger(normalized))) {
      throw new Error(`${name} must be ${integer ? 'an integer' : 'a finite number'} >= ${min}, got ${raw}`);
    }
    return normalized;
  };

  const copyValue = (value) => {
    if (value === undefined || value === null) {
      return null;
    }
    try {
      return JSON.parse(JSON.stringify(value));
    } catch (error) {
      return { error: error instanceof Error ? error.message : String(error) };
    }
  };

  const npcSignature = (npcs) => (Array.isArray(npcs) ? npcs.map((npc) => ({
    oid: npc.oid,
    name: npc.name,
    x: npc.x,
    y: npc.y,
  })) : []);

  const dialogueSignature = (dialogue) => (dialogue ? {
    mode: dialogue.mode,
    npcId: dialogue.npcId,
    text: dialogue.text,
    selections: dialogue.selections,
  } : null);

  const sampleSignature = (sample) => JSON.stringify({
    player: sample.player ? {
      mapId: sample.player.mapId,
      x: sample.player.x,
      y: sample.player.y,
      hp: sample.player.hp,
      mp: sample.player.mp,
    } : null,
    npcs: npcSignature(sample.npcs),
    dialogue: dialogueSignature(sample.dialogue),
  });

  const captureSample = (atMs, options) => {
    const errors = [];
    const section = (label, fallback, fn) => {
      try {
        return fn();
      } catch (error) {
        errors.push(`${label}: ${error instanceof Error ? error.message : String(error)}`);
        return fallback;
      }
    };

    const player = section('player', null, () => MapleAi.getPlayerState());
    const npcs = options.includeNpcs
      ? section('npcs', [], () => MapleAi.listNpcs())
      : [];
    const dialogue = options.includeDialogue
      ? section('dialogue', null, () => {
        const mode = MapleAi.getDialogueMode();
        return mode !== 0 ? MapleAi.readDialoguePage(0) : null;
      })
      : null;

    return {
      atMs,
      player: copyValue(player),
      npcs: copyValue(npcs),
      dialogue: copyValue(dialogue),
      errors,
    };
  };

  async function runStateWatchdog(overrides = {}) {
    const durationMs = numberOption(overrides.durationMs ?? globalThis.stateWatchDurationMs, 10000, 'stateWatchDurationMs');
    const intervalMs = numberOption(overrides.intervalMs ?? globalThis.stateWatchIntervalMs, 250, 'stateWatchIntervalMs', { min: 1 });
    const includeNpcs = overrides.includeNpcs ?? globalThis.stateWatchIncludeNpcs ?? true;
    const includeDialogue = overrides.includeDialogue ?? globalThis.stateWatchIncludeDialogue ?? true;
    const onlyChanges = overrides.onlyChanges ?? globalThis.stateWatchOnlyChanges ?? true;
    const startedAt = Date.now();
    const samples = [];
    let lastSignature = '';

    while (Date.now() - startedAt <= durationMs) {
      const sample = captureSample(Date.now() - startedAt, { includeNpcs, includeDialogue });
      const signature = sampleSignature(sample);
      if (!onlyChanges || samples.length === 0 || signature !== lastSignature) {
        samples.push(sample);
        lastSignature = signature;
      }
      if (Date.now() - startedAt >= durationMs) {
        break;
      }
      await MapleAi.sleep(Math.min(intervalMs, Math.max(0, durationMs - (Date.now() - startedAt))));
    }

    const report = {
      generatedAt: new Date().toISOString(),
      durationMs,
      intervalMs,
      sampleCount: samples.length,
      samples,
    };

    globalThis.stateWatchdogReport = report;
    globalThis.mapleStateWatchdogReport = report;
    console.log(`[state-watchdog] Captured ${report.sampleCount} sample(s).`);

    const shouldDownload = overrides.download ?? globalThis.stateWatchDownload;
    if (shouldDownload) {
      MapleAi.downloadJson(report, 'state_watchdog_report.json');
    }

    return report;
  }

  globalThis.runStateWatchdog = runStateWatchdog;

  const autorun = globalThis.stateWatchdogAutorun ?? true;
  if (!autorun) {
    return { loaded: true, runStateWatchdog };
  }

  return runStateWatchdog();
})();
