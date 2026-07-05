(async () => {
  const module = globalThis.Module;

  const sleep = (ms) => new Promise((resolve) => setTimeout(resolve, ms));
  const hasCcall = () => module && typeof module.ccall === 'function';
  const call = (name, returnType = 'number', argTypes = [], args = []) => {
    if (!hasCcall()) {
      throw new Error('WASM Module.ccall is not available. Load the client page before running this script.');
    }
    return module.ccall(name, returnType, argTypes, args);
  };
  const hasHook = (name) => Boolean(module && typeof module[`_${name}`] === 'function');
  const sendChat = (message) => hasHook('maple_send_chat') && call('maple_send_chat', 'number', ['string'], [message]) !== 0;
  const getMapId = () => hasHook('maple_get_player_mapid') ? call('maple_get_player_mapid', 'number', [], []) : 0;

  const params = new URLSearchParams(globalThis.location?.search ?? '');
  const rawMapId = globalThis.mapid ?? globalThis.mapId ?? params.get('mapid') ?? params.get('mapId');
  const mapId = Number(rawMapId);
  if (!Number.isInteger(mapId) || mapId < 0) {
    throw new Error('Usage: set globalThis.mapid or add ?mapid=<id> before running warp_to_map.js');
  }

  if (!hasHook('maple_warp_to_map')) {
    throw new Error('maple_warp_to_map hook is unavailable. Rebuild and reload the WASM client.');
  }

  console.log(`[warp] Warping to map ${mapId}.`);
  let dispatched = call('maple_warp_to_map', 'number', ['number'], [mapId]) !== 0;
  if (!dispatched) {
    console.warn(`[warp] Direct ChangeMapPacket dispatch failed; trying !warp ${mapId}.`);
    dispatched = sendChat(`!warp ${mapId}`);
  }
  if (!dispatched) {
    throw new Error(`Could not dispatch warp to ${mapId}.`);
  }

  const timeoutMs = Number(globalThis.warpTimeoutMs ?? 8000);
  const deadline = Date.now() + timeoutMs;
  while (Date.now() < deadline) {
    const currentMap = getMapId();
    if (currentMap === mapId) {
      console.log(`[warp] Arrived on map ${mapId}.`);
      return { ok: true, mapId };
    }
    await sleep(100);
  }

  const currentMap = getMapId();
  throw new Error(`Timed out waiting for map ${mapId}; current map is ${currentMap}.`);
})();
