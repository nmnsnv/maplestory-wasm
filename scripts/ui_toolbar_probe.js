(async () => {
  const loadMapleAi = async () => {
    if (globalThis.MapleAi?.version >= 2) return globalThis.MapleAi;
    const response = await fetch('/scripts/maple_ai_common.js', { cache: 'no-store' });
    if (!response.ok) throw new Error(`Could not load maple_ai_common.js: HTTP ${response.status}`);
    await globalThis.eval(await response.text());
    return globalThis.MapleAi;
  };

  const loadMapleUi = async () => {
    if (globalThis.MapleUi?.version >= 3) return globalThis.MapleUi;
    const response = await fetch('/scripts/ui_navigation.js', { cache: 'no-store' });
    if (!response.ok) throw new Error(`Could not load ui_navigation.js: HTTP ${response.status}`);
    await globalThis.eval(await response.text());
    return globalThis.MapleUi;
  };

  const MapleAi = await loadMapleAi();
  const MapleUi = await loadMapleUi();

  const BUTTONS = Object.freeze({
    menu: Object.freeze({ button: MapleAi.statusbarButtons.menu, elementType: MapleAi.uiElementTypes.MENU }),
    system: Object.freeze({ button: MapleAi.statusbarButtons.options, elementType: MapleAi.uiElementTypes.SYSTEMMENU }),
    options: Object.freeze({ button: MapleAi.statusbarButtons.options, elementType: MapleAi.uiElementTypes.SYSTEMMENU }),
    character: Object.freeze({ button: MapleAi.statusbarButtons.character, elementType: MapleAi.uiElementTypes.STATSINFO }),
    stats: Object.freeze({ button: MapleAi.statusbarButtons.stats, elementType: MapleAi.uiElementTypes.STATSINFO }),
    quest: Object.freeze({ button: MapleAi.statusbarButtons.quest, elementType: MapleAi.uiElementTypes.QUESTLOG }),
    inventory: Object.freeze({ button: MapleAi.statusbarButtons.inventory, elementType: MapleAi.uiElementTypes.ITEMINVENTORY }),
    equips: Object.freeze({ button: MapleAi.statusbarButtons.equips, elementType: MapleAi.uiElementTypes.EQUIPINVENTORY }),
    skill: Object.freeze({ button: MapleAi.statusbarButtons.skill, elementType: MapleAi.uiElementTypes.SKILLBOOK }),
  });
  const DEFAULT_BUTTONS = Object.freeze(['menu', 'system', 'character', 'stats', 'quest', 'inventory', 'equips', 'skill']);

  const normalizeButtons = (value) => {
    const source = value === undefined || value === null ? DEFAULT_BUTTONS : value;
    const buttons = Array.isArray(source) ? source : String(source).split(',');
    return buttons.map((button) => String(button).trim()).filter(Boolean);
  };

  const buttonConfig = (name) => {
    const config = BUTTONS[name];
    if (!config) {
      throw new Error(`Unknown toolbar button '${name}'. Known buttons: ${Object.keys(BUTTONS).sort().join(', ')}`);
    }
    return config;
  };

  const closePanel = async (elementType, delayMs) => {
    if (MapleAi.getUiElementActive(elementType)) {
      await MapleUi.pressKey('Escape', { afterMs: delayMs });
    }
    return MapleAi.getUiElementActive(elementType) === false;
  };

  async function runUiToolbarProbe(overrides = {}) {
    const buttons = normalizeButtons(overrides.buttons ?? globalThis.uiToolbarProbeButtons ?? globalThis.toolbarProbeButtons);
    const delayMs = Number(overrides.delayMs ?? globalThis.uiToolbarProbeDelayMs ?? globalThis.toolbarProbeDelayMs ?? 250);
    if (!Number.isFinite(delayMs) || delayMs < 0) {
      throw new Error(`uiToolbarProbeDelayMs must be non-negative, got ${overrides.delayMs ?? globalThis.uiToolbarProbeDelayMs}`);
    }

    const results = [];
    for (const name of buttons) {
      const config = buttonConfig(name);
      const result = {
        name,
        button: config.button,
        elementType: config.elementType,
        beforeActive: null,
        afterActive: null,
        closedAfterProbe: null,
        bounds: null,
        click: null,
        status: 'failed',
        error: '',
      };

      try {
        await closePanel(config.elementType, delayMs);
        result.beforeActive = MapleAi.getUiElementActive(config.elementType);
        const bounds = MapleAi.getStatusbarButtonBounds(config.button);
        result.bounds = bounds;
        if (!bounds) {
          throw new Error(`Statusbar button ${config.button} bounds are unavailable.`);
        }
        result.click = await MapleUi.clickGame(bounds.center, { afterMs: delayMs });
        result.afterActive = MapleAi.getUiElementActive(config.elementType);
        if (!result.afterActive) {
          throw new Error(`Toolbar button '${name}' did not activate UI element ${config.elementType}.`);
        }
        result.status = 'passed';
      } catch (error) {
        result.error = error instanceof Error ? error.message : String(error);
      } finally {
        try {
          result.closedAfterProbe = await closePanel(config.elementType, delayMs);
        } catch (error) {
          result.closedAfterProbe = false;
          if (!result.error) {
            result.error = error instanceof Error ? error.message : String(error);
          }
        }
      }

      results.push(result);
      console.log(`[ui-toolbar-probe] ${result.status.toUpperCase()} ${name}${result.error ? `: ${result.error}` : ''}`);
    }

    const report = {
      generatedAt: new Date().toISOString(),
      player: MapleAi.getPlayerState(),
      planned: buttons.length,
      tested: results.length,
      passed: results.filter((result) => result.status === 'passed').length,
      failed: results.filter((result) => result.status !== 'passed').length,
      results,
      unsupportedButtons: ['whisper', 'callGm', 'cashShop'].map((name) => ({
        name,
        button: MapleAi.statusbarButtons[name],
        bounds: MapleAi.getStatusbarButtonBounds(MapleAi.statusbarButtons[name]),
        reason: name === 'callGm'
          ? 'No client menu is wired for the Call GM statusbar button.'
          : 'The button dispatches a KeyAction but UIStateGame has no matching client panel.',
      })),
    };

    globalThis.uiToolbarProbeReport = report;
    globalThis.toolbarProbeReport = report;
    if (overrides.download ?? globalThis.uiToolbarProbeDownload ?? globalThis.toolbarProbeDownload) {
      MapleAi.downloadJson(report, 'ui_toolbar_probe_report.json');
    }
    return report;
  }

  globalThis.runUiToolbarProbe = runUiToolbarProbe;
  globalThis.runToolbarProbe = runUiToolbarProbe;

  const autorun = globalThis.uiToolbarProbeAutorun ?? globalThis.toolbarProbeAutorun ?? true;
  if (!autorun) {
    return { loaded: true, runUiToolbarProbe };
  }

  return runUiToolbarProbe();
})();
