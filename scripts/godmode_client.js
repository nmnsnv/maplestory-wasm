(() => {
  const setGodmode = Module?._maple_set_godmode;
  const getGodmode = Module?._maple_get_godmode;

  if (typeof setGodmode !== 'function') {
    console.warn('[godmode] Client godmode hook is not available on this page. Rebuild the WASM client first.');
    return false;
  }

  const params = new URLSearchParams(window.location.search);
  const arg = globalThis.godmode ?? params.get('godmode') ?? true;
  const enabled = !(arg === false || arg === 0 || arg === '0' || String(arg).toLowerCase() === 'false' || String(arg).toLowerCase() === 'off');

  setGodmode(enabled ? 1 : 0);
  const active = typeof getGodmode === 'function' ? getGodmode() !== 0 : enabled;
  console.log(`[godmode] Client test godmode ${active ? 'enabled' : 'disabled'}.`);
  return active;
})();
