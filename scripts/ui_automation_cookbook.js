(async () => {
  const loadMapleUi = async () => {
    if (globalThis.MapleUi?.version >= 3) return globalThis.MapleUi;
    const response = await fetch('/scripts/ui_navigation.js', { cache: 'no-store' });
    if (!response.ok) throw new Error(`Could not load ui_navigation.js: HTTP ${response.status}`);
    await globalThis.eval(await response.text());
    return globalThis.MapleUi;
  };

  const MapleUi = await loadMapleUi();

  const recipes = {
    async openStats(options = {}) {
      return MapleUi.openPanel('stats', options);
    },

    async openInventory(options = {}) {
      return MapleUi.openPanel('inventory', options);
    },

    async openEquipment(options = {}) {
      return MapleUi.openPanel('equips', options);
    },

    async openSkills(options = {}) {
      return MapleUi.openPanel('skills', options);
    },

    async openQuestLog(options = {}) {
      return MapleUi.openPanel('questLog', options);
    },

    async openWorldMap(options = {}) {
      return MapleUi.openPanel('worldMap', options);
    },

    async openKeyConfig(options = {}) {
      return MapleUi.openPanel('keyConfig', options);
    },

    async openMainMenu(options = {}) {
      return MapleUi.openPanel('mainMenu', options);
    },

    async openSystemMenu(options = {}) {
      return MapleUi.openPanel('systemMenu', options);
    },

    async focusChat(options = {}) {
      await MapleUi.focusChat(options);
      return { focused: 'chat' };
    },

    async sendChat(options = {}) {
      const message = options.message ?? globalThis.uiAutomationMessage;
      if (message === undefined || message === null) {
        throw new Error('sendChat recipe requires options.message or globalThis.uiAutomationMessage.');
      }
      await MapleUi.sendChatViaUi(String(message), options);
      return { sent: String(message) };
    },

    async clickPoint(options = {}) {
      const point = options.point ?? globalThis.uiAutomationPoint;
      if (point === undefined || point === null) {
        throw new Error('clickPoint recipe requires options.point or globalThis.uiAutomationPoint.');
      }
      return MapleUi.clickGame(point, options);
    },

    async doubleClickPoint(options = {}) {
      const point = options.point ?? globalThis.uiAutomationPoint;
      if (point === undefined || point === null) {
        throw new Error('doubleClickPoint recipe requires options.point or globalThis.uiAutomationPoint.');
      }
      return MapleUi.doubleClickGame(point, options);
    },

    async characterSelectRescue(options = {}) {
      Object.assign(globalThis, {
        charSelectRescueName: options.name ?? globalThis.charSelectRescueName,
        charSelectRescueSlot: options.slot ?? globalThis.charSelectRescueSlot,
        charSelectRescueTimeoutMs: options.timeoutMs ?? globalThis.charSelectRescueTimeoutMs,
        charSelectRescueFallbackUi: options.fallbackUi ?? globalThis.charSelectRescueFallbackUi,
        charSelectRescueReloadOnFailure: options.reloadOnFailure ?? globalThis.charSelectRescueReloadOnFailure,
      });
      const response = await fetch('/scripts/char_select_rescue.js', { cache: 'no-store' });
      if (!response.ok) throw new Error(`Could not load char_select_rescue.js: HTTP ${response.status}`);
      return globalThis.eval(await response.text());
    },

    async clickClient(options = {}) {
      const x = options.x ?? globalThis.uiAutomationClientX;
      const y = options.y ?? globalThis.uiAutomationClientY;
      if (!Number.isFinite(Number(x)) || !Number.isFinite(Number(y))) {
        throw new Error('clickClient recipe requires finite x/y client coordinates.');
      }
      return MapleUi.clickClient(Number(x), Number(y), options);
    },

    async clickFraction(options = {}) {
      const x = options.x ?? globalThis.uiAutomationFractionX;
      const y = options.y ?? globalThis.uiAutomationFractionY;
      if (!Number.isFinite(Number(x)) || !Number.isFinite(Number(y))) {
        throw new Error('clickFraction recipe requires finite x/y fractions.');
      }
      return MapleUi.clickFraction(Number(x), Number(y), options);
    },

    async registerPoint(options = {}) {
      const name = options.name ?? globalThis.uiAutomationPointName;
      const point = options.point ?? globalThis.uiAutomationPoint;
      if (!name) {
        throw new Error('registerPoint recipe requires options.name or globalThis.uiAutomationPointName.');
      }
      return MapleUi.registerPoint(String(name), point);
    },

    async sequence(options = {}) {
      const steps = options.steps ?? globalThis.uiAutomationSequence;
      if (!Array.isArray(steps)) {
        throw new Error('sequence recipe requires options.steps or globalThis.uiAutomationSequence array.');
      }
      return MapleUi.runSequence(steps, options);
    },

    metrics() {
      return MapleUi.metrics();
    },

    points() {
      return MapleUi.listPoints();
    },

    player() {
      return MapleUi.playerSnapshot();
    },

    async uiPanelProbe(options = {}) {
      globalThis.uiPanelProbeAutorun = false;
      globalThis.uiPanelProbePanels = options.panels ?? globalThis.uiPanelProbePanels;
      globalThis.uiPanelProbeDelayMs = options.delayMs ?? globalThis.uiPanelProbeDelayMs;
      const response = await fetch('/scripts/ui_panel_probe.js', { cache: 'no-store' });
      if (!response.ok) throw new Error(`Could not load ui_panel_probe.js: HTTP ${response.status}`);
      await globalThis.eval(await response.text());
      return globalThis.runUiPanelProbe(options);
    },

    async toolbarProbe(options = {}) {
      globalThis.uiToolbarProbeAutorun = false;
      globalThis.uiToolbarProbeButtons = options.buttons ?? options.panels ?? globalThis.uiToolbarProbeButtons;
      globalThis.uiToolbarProbeDelayMs = options.delayMs ?? globalThis.uiToolbarProbeDelayMs;
      const response = await fetch('/scripts/ui_toolbar_probe.js', { cache: 'no-store' });
      if (!response.ok) throw new Error(`Could not load ui_toolbar_probe.js: HTTP ${response.status}`);
      await globalThis.eval(await response.text());
      return globalThis.runUiToolbarProbe(options);
    },
  };

  const runRecipe = async (name, options = {}) => {
    const recipe = recipes[String(name)];
    if (typeof recipe !== 'function') {
      throw new Error(`Unknown UI automation recipe '${name}'. Known recipes: ${Object.keys(recipes).sort().join(', ')}`);
    }
    const result = await recipe(options);
    const report = {
      generatedAt: new Date().toISOString(),
      recipe: String(name),
      options,
      result,
      player: MapleUi.playerSnapshot(),
      error: '',
    };
    globalThis.uiAutomationReport = report;
    console.log(`[ui-automation] ${name} completed.`);
    return report;
  };

  const api = Object.freeze({
    version: 1,
    recipes: Object.freeze(recipes),
    runRecipe,
  });

  globalThis.MapleUiRecipes = api;

  if (globalThis.uiAutomationRecipe !== undefined && globalThis.uiAutomationRecipe !== null) {
    try {
      return await runRecipe(globalThis.uiAutomationRecipe, globalThis.uiAutomationOptions ?? {});
    } catch (error) {
      const report = {
        generatedAt: new Date().toISOString(),
        recipe: String(globalThis.uiAutomationRecipe),
        options: globalThis.uiAutomationOptions ?? {},
        result: null,
        player: MapleUi.playerSnapshot(),
        error: error instanceof Error ? error.message : String(error),
      };
      globalThis.uiAutomationReport = report;
      console.error(`[ui-automation] ${report.recipe} failed: ${report.error}`);
      return report;
    }
  }

  return api;
})();
