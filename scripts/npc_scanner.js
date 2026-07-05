(() => {
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

  const readI32 = (ptr) => module.HEAP32[ptr >> 2];
  const readCString = (ptr, maxBytes) => {
    let end = ptr;
    const limit = ptr + maxBytes;
    while (end < limit && module.HEAPU8[end] !== 0) {
      end += 1;
    }
    return new TextDecoder().decode(module.HEAPU8.subarray(ptr, end));
  };

  const readNpcInfo = (index) => {
    const oidPtr = module._malloc(4);
    const xPtr = module._malloc(4);
    const yPtr = module._malloc(4);
    const nameSize = 256;
    const namePtr = module._malloc(nameSize);
    try {
      const ok = call(
        'maple_get_npc_info',
        'number',
        ['number', 'number', 'number', 'number', 'number', 'number'],
        [index, oidPtr, xPtr, yPtr, namePtr, nameSize]
      );
      if (!ok) {
        return null;
      }
      return {
        index,
        oid: readI32(oidPtr),
        name: readCString(namePtr, nameSize),
        x: readI32(xPtr),
        y: readI32(yPtr),
      };
    } finally {
      module._free(oidPtr);
      module._free(xPtr);
      module._free(yPtr);
      module._free(namePtr);
    }
  };

  const count = call('maple_get_npc_count');
  const npcs = [];
  for (let index = 0; index < count; index += 1) {
    const npc = readNpcInfo(index);
    if (npc) {
      npcs.push(npc);
    }
  }

  globalThis.mapleNpcScan = npcs;
  console.table(npcs);
  return npcs;
})();
