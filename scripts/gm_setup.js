(() => {
  const module = globalThis.Module;

  const hasCcall = () => module && typeof module.ccall === 'function';
  const call = (name, returnType = 'number', argTypes = [], args = []) => {
    if (!hasCcall()) {
      throw new Error('WASM Module.ccall is not available. Load the client page before running this script.');
    }
    return module.ccall(name, returnType, argTypes, args);
  };

  const hasHook = (name) => Boolean(module && typeof module[`_${name}`] === 'function');
  const sendChat = (message) => {
    if (!hasHook('maple_send_chat')) {
      throw new Error('maple_send_chat hook is unavailable. Rebuild and reload the WASM client.');
    }
    return call('maple_send_chat', 'number', ['string'], [message]) !== 0;
  };

  const params = new URLSearchParams(globalThis.location?.search ?? '');
  const currentName = hasHook('maple_get_player_name')
    ? call('maple_get_player_name', 'string', [], [])
    : '';
  const characterName = String(
    globalThis.characterName ??
    globalThis.gmCharacterName ??
    params.get('character') ??
    currentName ??
    'hello'
  ).trim() || 'hello';
  const gmLevel = Number(globalThis.gmLevel ?? params.get('gmLevel') ?? 6);

  console.log(`[gm-setup] Requesting GM level ${gmLevel} for ${characterName}.`);
  console.log('[gm-setup] If this character is not already authorized to run GM commands, run: mariadb cosmic < scripts/grant_gm.sql');

  const ok = sendChat(`!setgmlevel ${characterName} ${gmLevel}`);
  if (ok) {
    console.log(`[gm-setup] Sent !setgmlevel ${characterName} ${gmLevel}.`);
  }
  return ok;
})();
