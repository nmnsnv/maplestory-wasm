(async () => {
  const loadBrowserScript = async (url, globalName, minVersion = 1) => {
    if (globalThis[globalName]?.version >= minVersion) {
      return globalThis[globalName];
    }

    const response = await fetch(url, { cache: 'no-store' });
    if (!response.ok) {
      throw new Error(`Could not load ${url}: HTTP ${response.status}`);
    }
    await globalThis.eval(await response.text());
    return globalThis[globalName];
  };

  const MapleUi = await loadBrowserScript('/scripts/ui_navigation.js', 'MapleUi', 3);
  const MapleAi = await loadBrowserScript('/scripts/maple_ai_common.js', 'MapleAi', 1);

  const options = {
    name: String(
      globalThis.charSelectRescueName ??
      globalThis.characterUiCharacterName ??
      globalThis.characterUiSelectName ??
      '',
    ).trim(),
    slot: globalThis.charSelectRescueSlot ?? globalThis.characterUiSlot ?? null,
    timeoutMs: Number(globalThis.charSelectRescueTimeoutMs ?? globalThis.characterUiEnterTimeoutMs ?? 20000),
    fallbackUi: globalThis.charSelectRescueFallbackUi !== false,
    reloadOnFailure: globalThis.charSelectRescueReloadOnFailure === true,
  };

  const report = {
    generatedAt: new Date().toISOString(),
    options,
    steps: [],
    playerBefore: MapleAi.getPlayerState(),
    playerAfter: null,
    charSelectBefore: MapleAi.getCharacterSelectState(),
    charSelectAfter: null,
    enteredGame: false,
    error: '',
  };

  const recordStep = (name, details = {}) => {
    const step = { name, at: new Date().toISOString(), ...details };
    report.steps.push(step);
    return step;
  };

  const normalizedSlot = () => {
    if (options.slot === null || options.slot === undefined || options.slot === '') {
      return null;
    }
    const slot = Number(options.slot);
    if (!Number.isInteger(slot) || slot < 0 || slot > 7) {
      throw new Error(`Character slot must be an integer from 0 to 7, got ${options.slot}`);
    }
    return slot;
  };

  const selectWithHooks = () => {
    if (options.name && MapleAi.hasHook('maple_select_character_by_name')) {
      const selected = MapleAi.selectCharacterByName(options.name);
      recordStep('selectByNameHook', { name: options.name, selected });
      if (selected) {
        return true;
      }
    }

    const slot = normalizedSlot();
    if (slot !== null && MapleAi.hasHook('maple_select_character_slot')) {
      const selected = MapleAi.selectCharacterSlot(slot);
      recordStep('selectSlotHook', { slot, selected });
      if (selected) {
        return true;
      }
    }

    return false;
  };

  const selectWithUi = async () => {
    const slot = normalizedSlot();
    if (slot === null) {
      recordStep('selectUiSkipped', { reason: 'no slot requested' });
      return false;
    }

    const client = await MapleUi.clickGame(MapleUi.points.charSelect.slot(slot), {
      afterMs: globalThis.charSelectRescueStepDelayMs ?? 500,
    });
    recordStep('selectSlotUi', {
      slot,
      gameX: client.gameX,
      gameY: client.gameY,
      clientX: client.x,
      clientY: client.y,
    });
    return true;
  };

  const startWithHook = () => {
    if (!MapleAi.hasHook('maple_start_selected_character')) {
      return false;
    }

    const dispatched = MapleAi.startSelectedCharacter();
    recordStep('startHook', { dispatched });
    return dispatched;
  };

  const startWithUi = async () => {
    const client = await MapleUi.doubleClickGame(MapleUi.points.charSelect.start, {
      betweenMs: globalThis.charSelectRescueDoubleClickDelayMs ?? 90,
      afterMs: globalThis.charSelectRescueStartDelayMs ?? 500,
    });
    recordStep('startDoubleClickUi', {
      gameX: client.gameX,
      gameY: client.gameY,
      clientX: client.x,
      clientY: client.y,
    });
    return true;
  };

  try {
    if (Number.isInteger(report.playerBefore.mapId) && report.playerBefore.mapId > 0) {
      report.enteredGame = true;
      report.playerAfter = report.playerBefore;
      recordStep('alreadyInGame', { mapId: report.playerBefore.mapId });
    } else {
      const selectedByHook = selectWithHooks();
      if (!selectedByHook && options.fallbackUi) {
        await selectWithUi();
      }

      const startedByHook = startWithHook();
      if (!startedByHook && options.fallbackUi) {
        await startWithUi();
      }
      if (!startedByHook && !options.fallbackUi) {
        throw new Error('No character-select start hook is available and UI fallback is disabled.');
      }

      report.playerAfter = await MapleAi.waitForAnyMap(options.timeoutMs);
      report.enteredGame = Number.isInteger(report.playerAfter.mapId) && report.playerAfter.mapId > 0;
      if (!report.enteredGame) {
        throw new Error(`Timed out entering game after ${options.timeoutMs}ms; last player snapshot ${JSON.stringify(report.playerAfter)}`);
      }
    }
  } catch (error) {
    report.error = error instanceof Error ? error.message : String(error);
    if (!report.playerAfter) {
      report.playerAfter = MapleAi.getPlayerState();
    }
  } finally {
    report.charSelectAfter = MapleAi.getCharacterSelectState();
    globalThis.charSelectRescueReport = report;
  }

  console.log(`[char-select-rescue] enteredGame=${report.enteredGame} error=${report.error || 'none'}`);

  if (!report.enteredGame && options.reloadOnFailure) {
    recordStep('reloadOnFailure');
    setTimeout(() => globalThis.location.reload(), 250);
  }

  return report;
})();
