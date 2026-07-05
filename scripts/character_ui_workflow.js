(async () => {
  const loadMapleUi = async () => {
    if (globalThis.MapleUi?.version >= 1) return globalThis.MapleUi;
    const response = await fetch('/scripts/ui_navigation.js', { cache: 'no-store' });
    if (!response.ok) throw new Error(`Could not load ui_navigation.js: HTTP ${response.status}`);
    await globalThis.eval(await response.text());
    return globalThis.MapleUi;
  };

  const loadMapleAi = async () => {
    if (globalThis.MapleAi?.version >= 1) return globalThis.MapleAi;
    const response = await fetch('/scripts/maple_ai_common.js', { cache: 'no-store' });
    if (!response.ok) throw new Error(`Could not load maple_ai_common.js: HTTP ${response.status}`);
    await globalThis.eval(await response.text());
    return globalThis.MapleAi;
  };

  const MapleUi = await loadMapleUi();
  const MapleAi = await loadMapleAi();

  const normalizeAction = (value) => String(value ?? 'selectAndStart').trim();

  const parseSlot = (value) => {
    const slot = Number(value);
    if (!Number.isInteger(slot) || slot < 0 || slot > 7) {
      throw new Error(`characterUiSlot must be an integer from 0 to 7, got ${value}`);
    }
    return slot;
  };

  const report = {
    generatedAt: new Date().toISOString(),
    action: normalizeAction(globalThis.characterUiAction),
    steps: [],
    playerBefore: MapleUi.playerSnapshot(),
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

  const click = async (name, point, options = {}) => {
    const client = await MapleUi.clickGame(point, options);
    recordStep(name, { gameX: client.gameX, gameY: client.gameY, clientX: client.x, clientY: client.y });
  };

  const selectCharacter = async (slot) => {
    const requestedName = String(globalThis.characterUiCharacterName ?? globalThis.characterUiSelectName ?? '').trim();
    if (requestedName && MapleAi.hasHook('maple_select_character_by_name')) {
      const selected = MapleAi.selectCharacterByName(requestedName);
      recordStep('selectCharacterByNameHook', { name: requestedName, selected });
      if (selected) {
        return;
      }
    }

    const normalizedSlot = parseSlot(slot);
    if (MapleAi.hasHook('maple_select_character_slot')) {
      const selected = MapleAi.selectCharacterSlot(normalizedSlot);
      recordStep(`selectCharacterSlot${normalizedSlot}Hook`, { selected });
      if (selected) {
        return;
      }
    }

    await click(`selectCharacterSlot${normalizedSlot}`, MapleUi.points.charSelect.slot(normalizedSlot), {
      afterMs: globalThis.characterUiStepDelayMs ?? 500,
    });
  };

  const startSelectedCharacter = async () => {
    if (MapleAi.hasHook('maple_start_selected_character')) {
      const dispatched = MapleAi.startSelectedCharacter();
      recordStep('startSelectedCharacterHook', { dispatched });
      if (!dispatched) {
        throw new Error('maple_start_selected_character returned false; are we on the character select screen?');
      }
    } else {
      const client = await MapleUi.doubleClickGame(MapleUi.points.charSelect.start, {
        afterMs: globalThis.characterUiStartDelayMs ?? 500,
        betweenMs: globalThis.characterUiDoubleClickDelayMs ?? 90,
      });
      recordStep('startSelectedCharacterDoubleClick', {
        gameX: client.gameX,
        gameY: client.gameY,
        clientX: client.x,
        clientY: client.y,
      });
    }

    const timeoutMs = globalThis.characterUiEnterTimeoutMs ?? 20000;
    const player = MapleAi.hasHook('maple_get_player_mapid')
      ? await MapleAi.waitForAnyMap(timeoutMs)
      : await MapleUi.waitForPlayerMap(timeoutMs);
    report.playerAfter = player;
    report.charSelectAfter = MapleAi.getCharacterSelectState();
    report.enteredGame = Number.isInteger(player.mapId) && player.mapId > 0;
    if (!report.enteredGame) {
      throw new Error(`Timed out entering game after ${timeoutMs}ms; last player snapshot ${JSON.stringify(player)}`);
    }
  };

  const createCharacter = async (name) => {
    const characterName = String(name ?? '').trim();
    if (characterName.length < 4 || characterName.length > 12) {
      throw new Error('characterUiName must be 4 to 12 characters.');
    }

    await click('openCreateCharacter', MapleUi.points.charSelect.create, {
      afterMs: globalThis.characterUiCreateScreenDelayMs ?? 800,
    });
    await click('focusNameField', MapleUi.points.charCreation.nameField, {
      afterMs: globalThis.characterUiStepDelayMs ?? 200,
    });
    await MapleUi.typeText(characterName, { keyDelayMs: globalThis.characterUiKeyDelayMs ?? 35 });
    recordStep('typeCharacterName', { name: characterName });

    await click('submitNameCheck', MapleUi.points.charCreation.okName, {
      afterMs: globalThis.characterUiNameCheckDelayMs ?? 2500,
    });
    await click('submitCharacterCreate', MapleUi.points.charCreation.okCreate, {
      afterMs: globalThis.characterUiCreationDelayMs ?? 2500,
    });
  };

  try {
    switch (report.action) {
      case 'select':
        await selectCharacter(globalThis.characterUiSlot ?? 0);
        report.playerAfter = MapleUi.playerSnapshot();
        break;
      case 'start':
        await startSelectedCharacter();
        break;
      case 'selectAndStart':
        await selectCharacter(globalThis.characterUiSlot ?? 0);
        await startSelectedCharacter();
        break;
      case 'create':
        await createCharacter(globalThis.characterUiName);
        report.playerAfter = MapleUi.playerSnapshot();
        break;
      case 'createAndStart':
        await createCharacter(globalThis.characterUiName);
        if (globalThis.characterUiSlot !== undefined && globalThis.characterUiSlot !== null) {
          await selectCharacter(globalThis.characterUiSlot);
        }
        await startSelectedCharacter();
        break;
      default:
        throw new Error(`Unsupported characterUiAction: ${report.action}`);
    }
  } catch (error) {
    report.error = error instanceof Error ? error.message : String(error);
    report.charSelectAfter = MapleAi.getCharacterSelectState();
    if (!report.playerAfter) {
      report.playerAfter = MapleUi.playerSnapshot();
    }
  }

  globalThis.characterUiWorkflowReport = report;
  console.log(`[character-ui-workflow] ${report.action} completed: enteredGame=${report.enteredGame} error=${report.error || 'none'}`);
  return report;
})();
