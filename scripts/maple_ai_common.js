(() => {
  if (globalThis.MapleAi?.version >= 2) {
    return globalThis.MapleAi;
  }

  const textDecoder = new TextDecoder();

  const UI_ELEMENT_TYPES = Object.freeze({
    NONE: 0,
    LOGIN: 1,
    LOGINWAIT: 2,
    LOGINNOTICE: 3,
    WORLDSELECT: 4,
    CHARSELECT: 5,
    CHARCREATION: 6,
    SOFTKEYBOARD: 7,
    STATUSMESSENGER: 8,
    STATUSBAR: 9,
    BUFFLIST: 10,
    NOTICE: 11,
    NPCTALK: 12,
    SHOP: 13,
    STORAGE: 14,
    STATSINFO: 15,
    ITEMINVENTORY: 16,
    EQUIPINVENTORY: 17,
    SKILLBOOK: 18,
    KEYCONFIG: 19,
    PARTY: 20,
    MINIMAP: 21,
    WORLDMAP: 22,
    MENU: 23,
    SYSTEMMENU: 24,
    QUESTLOG: 25,
  });

  const STATUSBAR_BUTTONS = Object.freeze({
    whisper: 0,
    callGm: 1,
    cashShop: 2,
    menu: 3,
    options: 4,
    character: 5,
    stats: 6,
    quest: 7,
    inventory: 8,
    equips: 9,
    skill: 10,
  });

  const STATUSBAR_PANEL_BUTTONS = Object.freeze({
    character: Object.freeze({ button: STATUSBAR_BUTTONS.character, elementType: UI_ELEMENT_TYPES.STATSINFO }),
    stats: Object.freeze({ button: STATUSBAR_BUTTONS.stats, elementType: UI_ELEMENT_TYPES.STATSINFO }),
    quest: Object.freeze({ button: STATUSBAR_BUTTONS.quest, elementType: UI_ELEMENT_TYPES.QUESTLOG }),
    inventory: Object.freeze({ button: STATUSBAR_BUTTONS.inventory, elementType: UI_ELEMENT_TYPES.ITEMINVENTORY }),
    equips: Object.freeze({ button: STATUSBAR_BUTTONS.equips, elementType: UI_ELEMENT_TYPES.EQUIPINVENTORY }),
    skill: Object.freeze({ button: STATUSBAR_BUTTONS.skill, elementType: UI_ELEMENT_TYPES.SKILLBOOK }),
    menu: Object.freeze({ button: STATUSBAR_BUTTONS.menu, elementType: UI_ELEMENT_TYPES.MENU }),
    options: Object.freeze({ button: STATUSBAR_BUTTONS.options, elementType: UI_ELEMENT_TYPES.SYSTEMMENU }),
  });

  const KEY_ACTIONS = Object.freeze({
    equips: 0,
    inventory: 1,
    charStats: 2,
    skillBook: 3,
    worldMap: 5,
    miniMap: 7,
    questLog: 8,
    keyConfig: 9,
    mainMenu: 14,
    party: 19,
    systemMenu: 42,
  });

  const MENU_PANEL_ACTIONS = Object.freeze({
    stats: Object.freeze({ action: KEY_ACTIONS.charStats, elementType: UI_ELEMENT_TYPES.STATSINFO }),
    inventory: Object.freeze({ action: KEY_ACTIONS.inventory, elementType: UI_ELEMENT_TYPES.ITEMINVENTORY }),
    equips: Object.freeze({ action: KEY_ACTIONS.equips, elementType: UI_ELEMENT_TYPES.EQUIPINVENTORY }),
    skill: Object.freeze({ action: KEY_ACTIONS.skillBook, elementType: UI_ELEMENT_TYPES.SKILLBOOK }),
    keyConfig: Object.freeze({ action: KEY_ACTIONS.keyConfig, elementType: UI_ELEMENT_TYPES.KEYCONFIG }),
    party: Object.freeze({ action: KEY_ACTIONS.party, elementType: UI_ELEMENT_TYPES.PARTY }),
    miniMap: Object.freeze({ action: KEY_ACTIONS.miniMap, elementType: UI_ELEMENT_TYPES.MINIMAP }),
    worldMap: Object.freeze({ action: KEY_ACTIONS.worldMap, elementType: UI_ELEMENT_TYPES.WORLDMAP }),
    menu: Object.freeze({ action: KEY_ACTIONS.mainMenu, elementType: UI_ELEMENT_TYPES.MENU }),
    systemMenu: Object.freeze({ action: KEY_ACTIONS.systemMenu, elementType: UI_ELEMENT_TYPES.SYSTEMMENU }),
    quest: Object.freeze({ action: KEY_ACTIONS.questLog, elementType: UI_ELEMENT_TYPES.QUESTLOG }),
  });

  const resolveUiElementType = (value) => {
    if (Number.isInteger(value)) {
      return value;
    }
    const key = String(value ?? '').trim();
    if (!key) {
      throw new Error('UI element type is required.');
    }
    const normalized = key.replace(/[-\s]/g, '').toUpperCase();
    if (Object.prototype.hasOwnProperty.call(UI_ELEMENT_TYPES, normalized)) {
      return UI_ELEMENT_TYPES[normalized];
    }
    throw new Error(`Unknown UI element type: ${value}`);
  };

  const resolveStatusbarButtonId = (value) => {
    if (Number.isInteger(value)) {
      return value;
    }
    const key = String(value ?? '').trim();
    if (!key) {
      throw new Error('Statusbar button name is required.');
    }
    if (Object.prototype.hasOwnProperty.call(STATUSBAR_BUTTONS, key)) {
      return STATUSBAR_BUTTONS[key];
    }
    const lower = key.charAt(0).toLowerCase() + key.slice(1);
    if (Object.prototype.hasOwnProperty.call(STATUSBAR_BUTTONS, lower)) {
      return STATUSBAR_BUTTONS[lower];
    }
    throw new Error(`Unknown statusbar button: ${value}`);
  };

  const safeRead = (fn, fallback = null) => {
    try {
      return fn();
    } catch (error) {
      return fallback;
    }
  };

  const api = {
    version: 2,

    sleep(ms) {
      return new Promise((resolve) => setTimeout(resolve, Number(ms) || 0));
    },

    module() {
      return globalThis.Module;
    },

    hasCcall() {
      return typeof api.module()?.ccall === 'function';
    },

    hasHook(name) {
      return typeof api.module()?.[`_${name}`] === 'function';
    },

    call(name, returnType = 'number', argTypes = [], args = []) {
      const module = api.module();
      if (!api.hasCcall()) {
        throw new Error('WASM Module.ccall not available. Load client page before running script.');
      }
      if (!api.hasHook(name)) {
        throw new Error(`${name} hook unavailable. Rebuild reload WASM client.`);
      }
      return module.ccall(name, returnType, argTypes, args);
    },

    optionalCall(name, returnType = 'number', argTypes = [], args = [], fallback = null) {
      if (!api.hasCcall() || !api.hasHook(name)) {
        return fallback;
      }
      return api.module().ccall(name, returnType, argTypes, args);
    },

    sendChat(message) {
      return api.call('maple_send_chat', 'number', ['string'], [String(message)]) !== 0;
    },

    sendMenuAction(action) {
      const id = Number(action);
      if (!Number.isInteger(id) || id < 0) {
        throw new Error(`Menu action must be a non-negative integer, got ${action}`);
      }
      return api.call('maple_send_menu_action', 'number', ['number'], [id]) !== 0;
    },

    getQuestStatus(questId) {
      const id = Number(questId);
      if (!Number.isInteger(id)) {
        throw new Error(`Quest id must be an integer, got ${questId}`);
      }
      return api.optionalCall('maple_get_quest_status', 'number', ['number'], [id], null);
    },

    getUiElementActive(elementType) {
      const type = resolveUiElementType(elementType);
      const active = api.optionalCall('maple_get_ui_element_active', 'number', ['number'], [type], null);
      if (active === null || active < 0) {
        return null;
      }
      return active !== 0;
    },

    getUiButtonBounds(elementType, buttonId) {
      if (!api.hasCcall() || !api.hasHook('maple_get_ui_button_bounds')) {
        return null;
      }

      const module = api.module();
      if (typeof module?._malloc !== 'function' || typeof module?._free !== 'function' || !globalThis.HEAP32) {
        throw new Error('WASM heap helpers unavailable. Rebuild and reload the WASM client.');
      }

      const type = resolveUiElementType(elementType);
      const id = Number(buttonId);
      if (!Number.isInteger(id) || id < 0) {
        throw new Error(`Button id must be a non-negative integer, got ${buttonId}`);
      }

      const ptr = module._malloc(16);
      try {
        const ok = api.call(
          'maple_get_ui_button_bounds',
          'number',
          ['number', 'number', 'number', 'number', 'number', 'number'],
          [type, id, ptr, ptr + 4, ptr + 8, ptr + 12],
        );
        if (!ok) {
          return null;
        }

        const heap32 = globalThis.HEAP32;
        const left = heap32[ptr >> 2];
        const top = heap32[(ptr + 4) >> 2];
        const right = heap32[(ptr + 8) >> 2];
        const bottom = heap32[(ptr + 12) >> 2];
        return {
          left,
          top,
          right,
          bottom,
          width: right - left,
          height: bottom - top,
          center: {
            x: left + ((right - left) / 2),
            y: top + ((bottom - top) / 2),
          },
        };
      } finally {
        module._free(ptr);
      }
    },

    getStatusbarButtonBounds(button) {
      return api.getUiButtonBounds(UI_ELEMENT_TYPES.STATUSBAR, resolveStatusbarButtonId(button));
    },

    getRuntimeDiagnostics() {
      const dialogueMode = safeRead(() => api.getDialogueMode(), 0);
      return {
        generatedAt: new Date().toISOString(),
        url: String(globalThis.location?.href ?? ''),
        moduleReady: api.hasCcall(),
        hooks: {
          questStatus: api.hasHook('maple_get_quest_status'),
          menuAction: api.hasHook('maple_send_menu_action'),
          uiElementActive: api.hasHook('maple_get_ui_element_active'),
          uiButtonBounds: api.hasHook('maple_get_ui_button_bounds'),
          npcInfo: api.hasHook('maple_get_npc_info'),
          playerMap: api.hasHook('maple_get_player_mapid'),
        },
        player: safeRead(() => api.getPlayerState(), null),
        characterSelect: safeRead(() => api.getCharacterSelectState(), null),
        dialogue: {
          mode: dialogueMode,
          page: dialogueMode !== 0 ? safeRead(() => api.readDialoguePage(0), null) : null,
        },
        npcs: safeRead(() => api.listNpcs(), []),
        ui: {
          statusbarActive: safeRead(() => api.getUiElementActive(UI_ELEMENT_TYPES.STATUSBAR), null),
          panels: Object.fromEntries(Object.entries(STATUSBAR_PANEL_BUTTONS).map(([name, config]) => [
            name,
            {
              active: safeRead(() => api.getUiElementActive(config.elementType), null),
              buttonBounds: safeRead(() => api.getStatusbarButtonBounds(config.button), null),
            },
          ])),
        },
      };
    },

    uiElementTypes: UI_ELEMENT_TYPES,
    keyActions: KEY_ACTIONS,
    menuPanelActions: MENU_PANEL_ACTIONS,
    statusbarButtons: STATUSBAR_BUTTONS,
    statusbarPanelButtons: STATUSBAR_PANEL_BUTTONS,

    getPlayerState() {
      return {
        name: api.optionalCall('maple_get_player_name', 'string', [], [], null),
        mapId: api.optionalCall('maple_get_player_mapid', 'number', [], [], null),
        x: api.optionalCall('maple_get_player_position_x', 'number', [], [], null),
        y: api.optionalCall('maple_get_player_position_y', 'number', [], [], null),
        hp: api.optionalCall('maple_get_player_hp', 'number', [], [], null),
        mp: api.optionalCall('maple_get_player_mp', 'number', [], [], null),
        level: api.optionalCall('maple_get_player_level', 'number', [], [], null),
        job: api.optionalCall('maple_get_player_job', 'number', [], [], null),
        exp: api.optionalCall('maple_get_player_exp', 'number', [], [], null),
      };
    },

    getCharacterSelectState() {
      const count = api.optionalCall('maple_get_charselect_count', 'number', [], [], -1);
      const selectedIndex = api.optionalCall('maple_get_charselect_selected_index', 'number', [], [], -1);
      const selectedId = api.optionalCall('maple_get_charselect_selected_id', 'number', [], [], -1);
      const selectedName = api.optionalCall('maple_get_charselect_selected_name', 'string', [], [], null);
      const characters = [];

      if (Number.isInteger(count) && count > 0 && api.hasHook('maple_get_charselect_character_name')) {
        for (let index = 0; index < count; index += 1) {
          characters.push({
            index,
            name: api.optionalCall('maple_get_charselect_character_name', 'string', ['number'], [index], null),
          });
        }
      }

      return {
        available: Number.isInteger(count) && count >= 0,
        count,
        selectedIndex,
        selectedId,
        selectedName,
        characters,
      };
    },

    selectCharacterSlot(slot) {
      const index = Number(slot);
      if (!Number.isInteger(index) || index < 0) {
        throw new Error(`Character slot must be a non-negative integer, got ${slot}`);
      }
      return api.call('maple_select_character_slot', 'number', ['number'], [index]) !== 0;
    },

    selectCharacterByName(name) {
      const characterName = String(name ?? '').trim();
      if (!characterName) {
        throw new Error('Character name is required.');
      }
      return api.call('maple_select_character_by_name', 'number', ['string'], [characterName]) !== 0;
    },

    startSelectedCharacter() {
      return api.call('maple_start_selected_character') !== 0;
    },

    async waitForAnyMap(timeoutMs = 15000) {
      const deadline = Date.now() + Number(timeoutMs);
      while (Date.now() < deadline) {
        const state = api.getPlayerState();
        if (Number.isInteger(state.mapId) && state.mapId > 0) {
          return state;
        }
        await api.sleep(250);
      }
      return api.getPlayerState();
    },

    async enterSelectedCharacter(options = {}) {
      if (!api.startSelectedCharacter()) {
        throw new Error('Could not dispatch selected character start. Is the client on character select?');
      }
      return api.waitForAnyMap(options.timeoutMs ?? 20000);
    },

    readNpcInfo(index) {
      const module = api.module();
      if (!api.hasCcall()) {
        throw new Error('WASM Module.ccall not available. Load client page before running script.');
      }
      if (!api.hasHook('maple_get_npc_info')) {
        throw new Error('maple_get_npc_info hook unavailable. Rebuild reload WASM client.');
      }
      if (typeof module?._malloc !== 'function' || typeof module?._free !== 'function') {
        throw new Error('WASM heap allocation helpers unavailable. Rebuild reload WASM client.');
      }

      const heap32 = globalThis.HEAP32;
      const heapU8 = globalThis.HEAPU8;
      if (!heap32 || !heapU8) {
        throw new Error('WASM heap views unavailable. Rebuild reload WASM client.');
      }

      const oidPtr = module._malloc(4);
      const xPtr = module._malloc(4);
      const yPtr = module._malloc(4);
      const nameSize = 256;
      const namePtr = module._malloc(nameSize);

      try {
        const ok = api.call(
          'maple_get_npc_info',
          'number',
          ['number', 'number', 'number', 'number', 'number', 'number'],
          [index, oidPtr, xPtr, yPtr, namePtr, nameSize],
        );
        if (!ok) {
          return null;
        }

        let end = namePtr;
        const limit = namePtr + nameSize;
        while (end < limit && heapU8[end] !== 0) {
          end += 1;
        }

        return {
          index,
          oid: heap32[oidPtr >> 2],
          name: textDecoder.decode(heapU8.subarray(namePtr, end)),
          x: heap32[xPtr >> 2],
          y: heap32[yPtr >> 2],
        };
      } finally {
        module._free(oidPtr);
        module._free(xPtr);
        module._free(yPtr);
        module._free(namePtr);
      }
    },

    listNpcs() {
      const count = api.call('maple_get_npc_count');
      const npcs = [];
      for (let index = 0; index < count; index += 1) {
        const npc = api.readNpcInfo(index);
        if (npc) {
          npcs.push(npc);
        }
      }
      return npcs;
    },

    async waitForMap(mapId, timeoutMs = 10000) {
      const targetMapId = Number(mapId);
      const deadline = Date.now() + Number(timeoutMs);
      while (Date.now() < deadline) {
        if (api.call('maple_get_player_mapid') === targetMapId) {
          return true;
        }
        await api.sleep(100);
      }
      return api.call('maple_get_player_mapid') === targetMapId;
    },

    async warpToMap(mapId, options = {}) {
      const targetMapId = Number(mapId);
      if (!Number.isInteger(targetMapId) || targetMapId < 0) {
        throw new Error(`Invalid map id: ${mapId}`);
      }

      let dispatched = api.call('maple_warp_to_map', 'number', ['number'], [targetMapId]) !== 0;
      if (!dispatched) {
        dispatched = api.sendChat(`!warp ${targetMapId}`);
      }
      if (!dispatched) {
        throw new Error(`Could not dispatch warp to map ${targetMapId}`);
      }

      const timeoutMs = options.timeoutMs ?? 10000;
      if (!await api.waitForMap(targetMapId, timeoutMs)) {
        const currentMap = api.optionalCall('maple_get_player_mapid', 'number', [], [], null);
        throw new Error(`Timed out waiting for map ${targetMapId}; current map is ${currentMap}`);
      }
      return true;
    },

    teleportToNpc(name) {
      return api.call('maple_teleport_to_npc', 'number', ['string'], [String(name)]) !== 0;
    },

    talkToNearestNpc() {
      return api.call('maple_talk_to_nearest_npc') !== 0;
    },

    getDialogueMode() {
      return api.optionalCall('maple_get_dialogue_mode', 'number', [], [], 0) ?? 0;
    },

    readSelections() {
      const count = api.call('maple_get_dialogue_selection_count');
      const selections = [];
      for (let index = 0; index < count; index += 1) {
        selections.push(api.call('maple_get_dialogue_selection', 'string', ['number'], [index]) ?? '');
      }
      return selections;
    },

    readDialoguePage(step = 0) {
      return {
        step,
        mode: api.call('maple_get_dialogue_mode'),
        npcId: api.call('maple_get_dialogue_npcid'),
        type: api.call('maple_get_dialogue_type'),
        text: api.call('maple_get_dialogue_text', 'string', [], []) ?? '',
        selections: api.readSelections(),
      };
    },

    dialogueActionForMode(mode) {
      if (mode === 4) return 6;
      if (mode === 2 || mode === 3) return 4;
      if (mode === 5) return 0;
      return 1;
    },

    async waitForDialogue(timeoutMs = 5000) {
      const deadline = Date.now() + Number(timeoutMs);
      while (Date.now() < deadline) {
        const mode = api.getDialogueMode();
        if (mode !== 0) {
          return mode;
        }
        await api.sleep(100);
      }
      return api.getDialogueMode();
    },

    async captureDialogue(options = {}) {
      const initialTimeoutMs = options.initialTimeoutMs ?? options.dialogueTimeoutMs ?? 5000;
      const nextTimeoutMs = options.nextTimeoutMs ?? options.nextDialogueTimeoutMs ?? 1500;
      const maxSteps = options.maxSteps ?? options.maxDialogueSteps ?? 50;
      const delayMs = options.delayMs ?? 150;
      const pages = [];

      let mode = await api.waitForDialogue(initialTimeoutMs);
      if (mode === 0) {
        throw new Error(`NPC did not respond with dialogue after ${initialTimeoutMs}ms`);
      }

      for (let step = 0; step < maxSteps; step += 1) {
        mode = api.getDialogueMode();
        if (mode === 0) {
          break;
        }

        const page = api.readDialoguePage(step);
        pages.push(page);

        const advanced = api.call('maple_advance_dialogue', 'number', ['number'], [api.dialogueActionForMode(mode)]) !== 0;
        if (!advanced) {
          break;
        }

        await api.sleep(delayMs);
        const nextMode = await api.waitForDialogue(nextTimeoutMs);
        if (nextMode === 0) {
          break;
        }
      }

      if (pages.length >= maxSteps) {
        throw new Error(`Dialogue exceeded ${maxSteps} steps`);
      }

      return pages;
    },

    async dispose(cooldownMs = 650) {
      let dispatched = false;
      try {
        dispatched = api.optionalCall('maple_send_chat', 'number', ['string'], ['!dispose'], 0) !== 0;
      } catch (error) {
        console.warn('[maple-ai] Failed to send !dispose:', error);
      }
      await api.sleep(cooldownMs);
      return dispatched;
    },

    async loadJson(url) {
      const response = await fetch(url, { cache: 'no-store' });
      if (!response.ok) {
        throw new Error(`Could not load ${url}: HTTP ${response.status}`);
      }
      return response.json();
    },

    async loadBrowserScript(url) {
      const response = await fetch(url, { cache: 'no-store' });
      if (!response.ok) {
        throw new Error(`Could not load ${url}: HTTP ${response.status}`);
      }
      return globalThis.eval(await response.text());
    },

    downloadJson(value, filename) {
      const blob = new Blob([JSON.stringify(value, null, 2), '\n'], { type: 'application/json' });
      const url = URL.createObjectURL(blob);
      const link = document.createElement('a');
      link.href = url;
      link.download = filename;
      link.click();
      URL.revokeObjectURL(url);
    },
  };

  globalThis.MapleAi = Object.freeze(api);
  return globalThis.MapleAi;
})();
