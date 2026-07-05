(async () => {
  const loadMapleAi = async () => {
    if (globalThis.MapleAi?.version >= 2) return globalThis.MapleAi;
    const response = await fetch('/scripts/maple_ai_common.js', { cache: 'no-store' });
    if (!response.ok) throw new Error(`Could not load maple_ai_common.js: HTTP ${response.status}`);
    await globalThis.eval(await response.text());
    return globalThis.MapleAi;
  };

  const MapleAi = await loadMapleAi();
  const DEFAULT_PANELS = Object.freeze(['stats', 'inventory', 'equips', 'skill', 'quest', 'keyConfig', 'party', 'miniMap', 'menu', 'systemMenu']);

  const normalizePanels = (value) => {
    const source = value === undefined || value === null ? DEFAULT_PANELS : value;
    const panels = Array.isArray(source) ? source : String(source).split(',');
    return panels.map((panel) => String(panel).trim()).filter(Boolean);
  };

  const panelConfig = (name) => {
    const config = MapleAi.menuPanelActions?.[name];
    if (!config) {
      throw new Error(`Unknown UI panel '${name}'. Known panels: ${Object.keys(MapleAi.menuPanelActions ?? {}).sort().join(', ')}`);
    }
    return config;
  };

  const sleepAfterAction = (delayMs) => MapleAi.sleep(delayMs);

  const ensureClosed = async (config, delayMs) => {
    const active = MapleAi.getUiElementActive(config.elementType);
    if (active) {
      MapleAi.sendMenuAction(config.action);
      await sleepAfterAction(delayMs);
    }
    return MapleAi.getUiElementActive(config.elementType) === false;
  };

  async function runUiPanelProbe(overrides = {}) {
    if (!MapleAi.hasCcall()) {
      throw new Error('WASM Module.ccall not available. Load client page before running script.');
    }
    for (const hook of ['maple_send_menu_action', 'maple_get_ui_element_active']) {
      if (!MapleAi.hasHook(hook)) {
        throw new Error(`${hook} hook unavailable. Rebuild reload WASM client.`);
      }
    }

    const panels = normalizePanels(overrides.panels ?? globalThis.uiPanelProbePanels);
    const delayMs = Number(overrides.delayMs ?? globalThis.uiPanelProbeDelayMs ?? 250);
    if (!Number.isFinite(delayMs) || delayMs < 0) {
      throw new Error(`uiPanelProbeDelayMs must be non-negative, got ${overrides.delayMs ?? globalThis.uiPanelProbeDelayMs}`);
    }

    const results = [];
    for (const name of panels) {
      const config = panelConfig(name);
      const result = {
        name,
        action: config.action,
        elementType: config.elementType,
        beforeActive: null,
        dispatched: false,
        afterActive: null,
        closedAfterProbe: null,
        status: 'failed',
        error: '',
      };

      try {
        await ensureClosed(config, delayMs);
        result.beforeActive = MapleAi.getUiElementActive(config.elementType);
        result.dispatched = MapleAi.sendMenuAction(config.action);
        await sleepAfterAction(delayMs);
        result.afterActive = MapleAi.getUiElementActive(config.elementType);
        if (!result.dispatched) {
          throw new Error(`Menu action ${config.action} was not dispatched.`);
        }
        if (!result.afterActive) {
          throw new Error(`Menu action ${config.action} did not activate UI element ${config.elementType}.`);
        }
        result.status = 'passed';
      } catch (error) {
        result.error = error instanceof Error ? error.message : String(error);
      } finally {
        try {
          result.closedAfterProbe = await ensureClosed(config, delayMs);
        } catch (error) {
          result.closedAfterProbe = false;
          if (!result.error) {
            result.error = error instanceof Error ? error.message : String(error);
          }
        }
      }

      results.push(result);
      console.log(`[ui-panel-probe] ${result.status.toUpperCase()} ${name}${result.error ? `: ${result.error}` : ''}`);
    }

    const report = {
      generatedAt: new Date().toISOString(),
      planned: panels.length,
      tested: results.length,
      passed: results.filter((result) => result.status === 'passed').length,
      failed: results.filter((result) => result.status !== 'passed').length,
      results,
    };

    globalThis.uiPanelProbeReport = report;
    if (overrides.download ?? globalThis.uiPanelProbeDownload) {
      MapleAi.downloadJson(report, 'ui_panel_probe_report.json');
    }
    return report;
  }

  globalThis.runUiPanelProbe = runUiPanelProbe;

  const autorun = globalThis.uiPanelProbeAutorun ?? true;
  if (!autorun) {
    return { loaded: true, runUiPanelProbe };
  }

  return runUiPanelProbe();
})();
