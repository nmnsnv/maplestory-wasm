(() => {
  const module = globalThis.Module;

  const hasCcall = () => module && typeof module.ccall === 'function';
  const hasHook = (name) => Boolean(module && typeof module[`_${name}`] === 'function');
  const call = (name, returnType = 'number', argTypes = [], args = []) => {
    if (!hasCcall()) {
      throw new Error('WASM Module.ccall is not available. Load the client page before running this script.');
    }
    if (!hasHook(name)) {
      return null;
    }
    return module.ccall(name, returnType, argTypes, args);
  };

  const state = {
    name: call('maple_get_player_name', 'string', [], []),
    mapId: call('maple_get_player_mapid'),
    x: call('maple_get_player_position_x'),
    y: call('maple_get_player_position_y'),
    hp: call('maple_get_player_hp'),
    mp: call('maple_get_player_mp'),
    level: call('maple_get_player_level'),
    job: call('maple_get_player_job'),
    exp: call('maple_get_player_exp', 'number', [], []),
  };

  globalThis.maplePlayerState = state;
  console.table([state]);
  return state;
})();
