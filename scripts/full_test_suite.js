(async () => {
  const module = globalThis.Module;

  const hasCcall = () => module && typeof module.ccall === 'function';
  const hasHook = (name) => Boolean(module && typeof module[`_${name}`] === 'function');
  const call = (name, returnType = 'number', argTypes = [], args = []) => {
    if (!hasCcall()) {
      throw new Error('WASM Module.ccall is not available. Load the client page before running this script.');
    }
    if (!hasHook(name)) {
      throw new Error(`${name} hook is unavailable. Rebuild and reload the WASM client.`);
    }
    return module.ccall(name, returnType, argTypes, args);
  };

  const ensureManifest = async (manifestUrl) => {
    const response = await fetch(manifestUrl, { cache: 'no-store' });
    if (!response.ok) {
      throw new Error(`Missing ${manifestUrl}. Run: python3 scripts/extract_quest_data.py`);
    }
    return response.json();
  };

  const loadQuestRunner = async () => {
    if (typeof globalThis.runQuestTests === 'function') {
      return;
    }

    const response = await fetch('/scripts/quest_test_runner.js', { cache: 'no-store' });
    if (!response.ok) {
      throw new Error(`Could not load quest_test_runner.js: HTTP ${response.status}`);
    }

    const previousAutorun = globalThis.questTestAutorun;
    globalThis.questTestAutorun = false;
    try {
      // The runner is a browser automation script, so evaluating it in-page is the intended loading path.
      await globalThis.eval(await response.text());
    } finally {
      globalThis.questTestAutorun = previousAutorun;
    }

    if (typeof globalThis.runQuestTests !== 'function') {
      throw new Error('quest_test_runner.js loaded but did not expose runQuestTests().');
    }
  };

  const options = {
    manifestUrl: '/scripts/quest_manifest.json',
    ...(globalThis.fullQuestTestOptions ?? {}),
  };

  await ensureManifest(options.manifestUrl);

  const playerName = hasHook('maple_get_player_name')
    ? call('maple_get_player_name', 'string', [], [])
    : 'hello';
  if (hasHook('maple_send_chat')) {
    call('maple_send_chat', 'number', ['string'], [`!setgmlevel ${playerName || 'hello'} 6`]);
  }

  await loadQuestRunner();
  const report = await globalThis.runQuestTests(options);
  globalThis.fullQuestTestReport = report;
  console.log(`[full-suite] Complete: ${report.passed}/${report.tested} passed, ${report.failed} failed.`);
  return report;
})();
