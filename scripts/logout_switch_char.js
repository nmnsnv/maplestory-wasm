// Logout and switch characters without a page reload.
//
// Usage in browser console:
//   globalThis.switchCharacterName = 'Aiq704b';
//   await (await fetch('/scripts/logout_switch_char.js', { cache: 'no-store' })).text();
//
// Or with explicit name:
//   await globalThis.eval(await (await fetch('/scripts/logout_switch_char.js', { cache: 'no-store' })).text());
//   // then: await globalThis.switchCharacter('Aiq704b');
//
// Requires WASM hooks: maple_relogin, maple_set_auto_login_character
// (added alongside maple_logout in Journey.cpp).

(() => {
  if (globalThis.MapleAi?.version >= 2) {
    return globalThis.MapleAi;
  }
  // The IIFE below is a fallback loader so this script is self-contained.
})();

(async () => {
  const loadScript = async (url, globalName, minVersion = 1) => {
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

  const MapleAi = await loadScript('/scripts/maple_ai_common.js', 'MapleAi', 2);

  const options = {
    name: String(globalThis.switchCharacterName ?? '').trim(),
    timeoutMs: Number(globalThis.switchCharacterTimeoutMs ?? 30000),
    pollIntervalMs: Number(globalThis.switchCharacterPollIntervalMs ?? 1000),
  };

  const report = {
    generatedAt: new Date().toISOString(),
    options,
    steps: [],
    playerBefore: MapleAi.getPlayerState(),
    playerAfter: null,
    switched: false,
    error: '',
  };

  const recordStep = (name, details = {}) => {
    const step = { name, at: new Date().toISOString(), ...details };
    report.steps.push(step);
    return step;
  };

  globalThis.switchCharacterReport = report;

  const sleep = (ms) => new Promise((r) => setTimeout(r, ms));

  const waitForMap = async (timeoutMs, pollMs) => {
    const deadline = Date.now() + timeoutMs;
    while (Date.now() < deadline) {
      const state = MapleAi.getPlayerState();
      if (Number.isInteger(state.mapId) && state.mapId > 0) {
        return state;
      }
      await sleep(pollMs);
    }
    return MapleAi.getPlayerState();
  };

  globalThis.switchCharacter = async (characterName) => {
    const name = String(characterName ?? options.name ?? '').trim();
    if (!name) {
      throw new Error('Character name is required. Set globalThis.switchCharacterName or pass it to switchCharacter().');
    }

    report.options.name = name;
    report.playerBefore = MapleAi.getPlayerState();
    report.steps = [];
    report.switched = false;
    report.error = '';

    try {
      // Step 1: Update the AutoLoginCharacter setting so the next auto-login
      // selects the desired character on the char select screen.
      if (!MapleAi.hasHook('maple_set_auto_login_character')) {
        throw new Error('maple_set_auto_login_character hook unavailable. Rebuild and reload the WASM client.');
      }
      const setOk = MapleAi.call('maple_set_auto_login_character', 'number', ['string'], [name]);
      recordStep('setAutoLoginCharacter', { name, ok: setOk === 1 });
      if (setOk !== 1) {
        throw new Error(`Failed to set auto-login character to "${name}".`);
      }

      // Step 2: Re-login without suppressing auto-login. This disconnects
      // from the channel server, reconnects to the login server, and
      // changes UI state to LOGIN. Auto-login then fires automatically:
      //   UILogin fills credentials → UIWorldSelect picks world/channel
      //   → UICharSelect auto-selects the configured character → sends
      //   SelectCharPicPacket with the auto-PIC.
      if (!MapleAi.hasHook('maple_relogin')) {
        throw new Error('maple_relogin hook unavailable. Rebuild and reload the WASM client.');
      }
      MapleAi.call('maple_relogin');
      recordStep('relogin', { dispatched: true });

      // Step 3: Wait for the client to re-enter the game. The auto-login
      // flow goes through LOGIN → WORLDSELECT → CHARSELECT → game map.
      // We poll until mapId becomes non-zero (meaning the character has
      // loaded into the game world) or the timeout expires.
      const playerAfter = await waitForMap(options.timeoutMs, options.pollIntervalMs);
      report.playerAfter = playerAfter;
      recordStep('waitForMap', {
        mapId: playerAfter.mapId,
        name: playerAfter.name,
        timeout: playerAfter.mapId === 0 || playerAfter.mapId === null,
      });

      if (Number.isInteger(playerAfter.mapId) && playerAfter.mapId > 0 && playerAfter.name === name) {
        report.switched = true;
        console.log(`[switch-char] Successfully switched to "${name}" on map ${playerAfter.mapId}.`);
      } else if (Number.isInteger(playerAfter.mapId) && playerAfter.mapId > 0) {
        report.error = `Entered game as "${playerAfter.name}" (map ${playerAfter.mapId}), expected "${name}".`;
        console.warn(`[switch-char] ${report.error}`);
      } else {
        report.error = `Timed out waiting for "${name}" to enter the game. Current mapId: ${playerAfter.mapId}.`;
        console.warn(`[switch-char] ${report.error}`);
      }

      globalThis.switchCharacterReport = report;
      return report;
    } catch (e) {
      report.error = String(e.message || e);
      console.error(`[switch-char] ${report.error}`);
      globalThis.switchCharacterReport = report;
      return report;
    }
  };

  // Also expose a simple logout that re-logins with the same character.
  globalThis.reloginSameCharacter = async () => {
    const current = MapleAi.getPlayerState();
    const name = current.name || options.name;
    if (!name) {
      throw new Error('Cannot determine current character name. Set globalThis.switchCharacterName first.');
    }
    return globalThis.switchCharacter(name);
  };

  // Auto-run if a character name was provided via global config.
  if (options.name) {
    globalThis.switchCharacter(options.name);
  } else {
    console.log('[switch-char] Ready. Call globalThis.switchCharacter("CharacterName") to switch.');
  }

  return report;
})();
