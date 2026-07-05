(() => {
  const teleport = Module?._maple_teleport_to_npc;
  const talk = Module?._maple_talk_to_nearest_npc;

  if (typeof teleport !== 'function') {
    console.warn('[npc-teleport] Client teleport hook is not available on this page. Rebuild and reload the WASM client first.');
    return false;
  }

  if (typeof talk !== 'function') {
    console.warn('[npc-teleport] Client NPC-talk hook is not available on this page. Rebuild and reload the WASM client first.');
    return false;
  }

  const params = new URLSearchParams(window.location.search);
  const requested = globalThis.npcName ?? globalThis.npc ?? params.get('npc') ?? 'Peter';
  const name = String(requested);

  const callTeleport = (npcName) => {
    if (typeof Module.ccall === 'function') {
      return Module.ccall('maple_teleport_to_npc', 'number', ['string'], [npcName]) !== 0;
    }

    const length = lengthBytesUTF8(npcName) + 1;
    const ptr = Module._malloc(length);
    try {
      stringToUTF8(npcName, ptr, length);
      return teleport(ptr) !== 0;
    } finally {
      Module._free(ptr);
    }
  };

  if (!globalThis.__mapleNpcTeleportSpaceHook) {
    globalThis.__mapleNpcTeleportSpaceHook = true;
    window.addEventListener('keydown', (event) => {
      if (event.code !== 'Space' || event.repeat) {
        return;
      }

      const target = event.target;
      const tagName = target?.tagName?.toLowerCase();
      if (tagName === 'input' || tagName === 'textarea' || target?.isContentEditable) {
        return;
      }

      const handled = talk() !== 0;
      if (handled) {
        event.preventDefault();
        event.stopPropagation();
      }
    }, true);
  }

  const ok = callTeleport(name);
  if (!ok) {
    console.warn(`[npc-teleport] Could not find active NPC "${name}" on the current map.`);
    return false;
  }

  console.log(`[npc-teleport] Teleported to "${name}". Press Space to talk to the nearest NPC.`);
  return true;
})();
