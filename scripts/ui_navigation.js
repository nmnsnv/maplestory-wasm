(() => {
  if (globalThis.MapleUi?.version >= 3) {
    return globalThis.MapleUi;
  }

  const GAME_HEIGHT = 600;
  const customPoints = new Map();

  const points = Object.freeze({
    screen: Object.freeze({
      center() {
        const element = globalThis.Module?.canvas ?? document.getElementById('canvas') ?? document.querySelector('canvas');
        const width = element?.width ?? 800;
        const height = element?.height ?? GAME_HEIGHT;
        return { x: width / 2, y: height / 2 };
      },
      bottomCenter() {
        const element = globalThis.Module?.canvas ?? document.getElementById('canvas') ?? document.querySelector('canvas');
        const width = element?.width ?? 800;
        return { x: width / 2, y: GAME_HEIGHT - 20 };
      },
    }),
    login: Object.freeze({
      account: Object.freeze({ x: 390, y: 261 }),
      password: Object.freeze({ x: 390, y: 287 }),
      submit: Object.freeze({ x: 520, y: 261 }),
    }),
    worldSelect: Object.freeze({
      enterWorld: Object.freeze({ x: 200, y: 170 }),
      world0: Object.freeze({ x: 650, y: 45 }),
      channel(index) {
        const channel = Number(index);
        if (!Number.isInteger(channel) || channel < 0 || channel > 19) {
          throw new Error(`Channel must be an integer from 0 to 19, got ${index}`);
        }
        return { x: 160 + (80 * (channel % 5)), y: 180 + (28 * Math.floor(channel / 5)) };
      },
    }),
    charSelect: Object.freeze({
      start: Object.freeze({ x: 662, y: 410 }),
      startText: Object.freeze({ x: 661, y: 397 }),
      create: Object.freeze({ x: 253, y: 512 }),
      delete: Object.freeze({ x: 373, y: 512 }),
      pageLeft: Object.freeze({ x: 125, y: 515 }),
      pageRight: Object.freeze({ x: 515, y: 515 }),
      slot(index) {
        const slot = Number(index);
        if (!Number.isInteger(slot) || slot < 0 || slot > 7) {
          throw new Error(`Character slot must be an integer from 0 to 7, got ${index}`);
        }
        return {
          x: 130 + (120 * (slot % 4)),
          y: 210 + (200 * (slot > 3 ? 1 : 0)),
        };
      },
    }),
    charCreation: Object.freeze({
      nameField: Object.freeze({ x: 560, y: 231 }),
      okName: Object.freeze({ x: 517, y: 312 }),
      cancelName: Object.freeze({ x: 590, y: 312 }),
      okCreate: Object.freeze({ x: 521, y: 465 }),
      cancelCreate: Object.freeze({ x: 595, y: 465 }),
      faceLeft: Object.freeze({ x: 531, y: 226 }),
      faceRight: Object.freeze({ x: 655, y: 226 }),
      hairLeft: Object.freeze({ x: 531, y: 245 }),
      hairRight: Object.freeze({ x: 655, y: 245 }),
      hairColorLeft: Object.freeze({ x: 531, y: 264 }),
      hairColorRight: Object.freeze({ x: 655, y: 264 }),
      skinLeft: Object.freeze({ x: 531, y: 283 }),
      skinRight: Object.freeze({ x: 655, y: 283 }),
      topLeft: Object.freeze({ x: 531, y: 302 }),
      topRight: Object.freeze({ x: 655, y: 302 }),
      bottomLeft: Object.freeze({ x: 531, y: 321 }),
      bottomRight: Object.freeze({ x: 655, y: 321 }),
      shoesLeft: Object.freeze({ x: 531, y: 340 }),
      shoesRight: Object.freeze({ x: 655, y: 340 }),
      weaponLeft: Object.freeze({ x: 531, y: 359 }),
      weaponRight: Object.freeze({ x: 655, y: 359 }),
      genderLeft: Object.freeze({ x: 531, y: 378 }),
      genderRight: Object.freeze({ x: 655, y: 378 }),
    }),
  });

  const panelHotkeys = Object.freeze({
    stats: 'c',
    character: 'c',
    inventory: 'i',
    items: 'i',
    equips: 'e',
    equipment: 'e',
    skill: 'k',
    skills: 'k',
    quest: 'q',
    quests: 'q',
    questLog: 'q',
    worldMap: 'w',
    keyConfig: '\\',
    keyboard: '\\',
    party: 'p',
    miniMap: 'm',
    mainMenu: '[',
    menu: '[',
    systemMenu: 'Escape',
    chat: 'Enter',
  });

  const sleep = (ms) => new Promise((resolve) => setTimeout(resolve, Number(ms) || 0));

  const canvas = () => globalThis.Module?.canvas ?? document.getElementById('canvas') ?? document.querySelector('canvas');

  const requireCanvas = () => {
    const element = canvas();
    if (!element) {
      throw new Error('MapleStory canvas not found. Load the client page before running UI automation.');
    }
    return element;
  };

  const metrics = () => {
    const element = requireCanvas();
    const rect = element.getBoundingClientRect();
    return {
      canvasWidth: element.width,
      canvasHeight: element.height,
      cssWidth: rect.width,
      cssHeight: rect.height,
      left: rect.left,
      top: rect.top,
      scaleX: rect.width / element.width,
      scaleY: rect.height / element.height,
      gameHeight: GAME_HEIGHT,
    };
  };

  const resolveNamedPoint = (name) => {
    if (customPoints.has(name)) {
      return customPoints.get(name);
    }

    const path = String(name).split('.');
    let current = points;
    for (const segment of path) {
      current = current?.[segment];
    }
    if (typeof current === 'function') {
      current = current();
    }
    if (!current || typeof current.x !== 'number' || typeof current.y !== 'number') {
      throw new Error(`Unknown Maple UI point: ${name}`);
    }
    return current;
  };

  const normalizePoint = (pointOrName, maybeY) => {
    if (typeof pointOrName === 'string') {
      return resolveNamedPoint(pointOrName);
    }

    if (Array.isArray(pointOrName) && pointOrName.length >= 2) {
      return { x: Number(pointOrName[0]), y: Number(pointOrName[1]) };
    }

    if (typeof pointOrName === 'object' && pointOrName !== null) {
      return { x: Number(pointOrName.x), y: Number(pointOrName.y) };
    }

    return { x: Number(pointOrName), y: Number(maybeY) };
  };

  const assertPoint = (point) => {
    if (!Number.isFinite(point.x) || !Number.isFinite(point.y)) {
      throw new Error(`Invalid Maple UI point: ${JSON.stringify(point)}`);
    }
    return point;
  };

  const clientPoint = (pointOrName, maybeY) => {
    const point = assertPoint(normalizePoint(pointOrName, maybeY));
    const info = metrics();
    return {
      x: info.left + (point.x * info.scaleX),
      y: info.top + (point.y * info.scaleY),
      gameX: point.x,
      gameY: point.y,
    };
  };

  const gamePointFromClient = (clientX, clientY) => {
    const info = metrics();
    return {
      x: (Number(clientX) - info.left) / info.scaleX,
      y: (Number(clientY) - info.top) / info.scaleY,
      clientX: Number(clientX),
      clientY: Number(clientY),
    };
  };

  const registerPoint = (name, pointOrName, maybeY) => {
    const point = assertPoint(normalizePoint(pointOrName, maybeY));
    customPoints.set(String(name), Object.freeze({ x: point.x, y: point.y }));
    return customPoints.get(String(name));
  };

  const listPoints = () => ({
    builtIn: [
      'screen.center',
      'screen.bottomCenter',
      'login.account',
      'login.password',
      'login.submit',
      'worldSelect.enterWorld',
      'worldSelect.world0',
      'worldSelect.channel(index)',
      'charSelect.start',
      'charSelect.startText',
      'charSelect.create',
      'charSelect.delete',
      'charSelect.pageLeft',
      'charSelect.pageRight',
      'charSelect.slot(index)',
      'charCreation.nameField',
      'charCreation.okName',
      'charCreation.cancelName',
      'charCreation.okCreate',
      'charCreation.cancelCreate',
      'charCreation.faceLeft',
      'charCreation.faceRight',
      'charCreation.hairLeft',
      'charCreation.hairRight',
      'charCreation.hairColorLeft',
      'charCreation.hairColorRight',
      'charCreation.skinLeft',
      'charCreation.skinRight',
      'charCreation.topLeft',
      'charCreation.topRight',
      'charCreation.bottomLeft',
      'charCreation.bottomRight',
      'charCreation.shoesLeft',
      'charCreation.shoesRight',
      'charCreation.weaponLeft',
      'charCreation.weaponRight',
      'charCreation.genderLeft',
      'charCreation.genderRight',
    ],
    custom: [...customPoints.keys()].sort(),
  });

  const dispatchMouseAtClient = (type, client, options = {}) => {
    const element = requireCanvas();
    const event = new MouseEvent(type, {
      bubbles: true,
      cancelable: true,
      view: globalThis,
      clientX: client.x,
      clientY: client.y,
      screenX: globalThis.screenX + client.x,
      screenY: globalThis.screenY + client.y,
      button: options.button ?? 0,
      buttons: options.buttons ?? (type === 'mousedown' ? 1 : 0),
      detail: options.detail ?? (type === 'dblclick' ? 2 : 1),
    });
    element.dispatchEvent(event);
    return event;
  };

  const dispatchPointerAtClient = (type, client, options = {}) => {
    if (typeof PointerEvent !== 'function') {
      return null;
    }

    const element = requireCanvas();
    const event = new PointerEvent(type, {
      bubbles: true,
      cancelable: true,
      view: globalThis,
      pointerId: options.pointerId ?? 1,
      pointerType: options.pointerType ?? 'mouse',
      isPrimary: true,
      clientX: client.x,
      clientY: client.y,
      screenX: globalThis.screenX + client.x,
      screenY: globalThis.screenY + client.y,
      button: options.button ?? (type === 'pointermove' ? -1 : 0),
      buttons: options.buttons ?? (type === 'pointerdown' ? 1 : 0),
      width: 1,
      height: 1,
      pressure: type === 'pointerdown' || type === 'pointermove' && (options.buttons ?? 0) ? 0.5 : 0,
    });
    element.dispatchEvent(event);
    return event;
  };

  const moveClient = async (client, options = {}) => {
    const moveOptions = { ...options, button: -1, buttons: 0 };
    if (options.pointerEvents !== false) {
      dispatchPointerAtClient('pointermove', client, moveOptions);
    }
    dispatchMouseAtClient('mousemove', client, moveOptions);
    await sleep(options.afterMs ?? 25);
    return client;
  };

  const clickClient = async (clientX, clientY, options = {}) => {
    const client = { x: Number(clientX), y: Number(clientY) };
    const beforeMs = options.beforeMs ?? 25;
    const holdMs = options.holdMs ?? 50;
    const afterMs = options.afterMs ?? 100;
    const element = requireCanvas();
    const includePointerEvents = options.pointerEvents !== false;
    const detail = options.detail ?? 1;
    element.focus?.();

    await moveClient(client, { ...options, afterMs: beforeMs });
    if (includePointerEvents) {
      dispatchPointerAtClient('pointerdown', client, { ...options, button: 0, buttons: 1, detail });
    }
    dispatchMouseAtClient('mousedown', client, { ...options, button: 0, buttons: 1, detail });
    await sleep(holdMs);
    if (includePointerEvents) {
      dispatchPointerAtClient('pointerup', client, { ...options, button: 0, buttons: 0, detail });
    }
    dispatchMouseAtClient('mouseup', client, { ...options, button: 0, buttons: 0, detail });
    dispatchMouseAtClient('click', client, { ...options, button: 0, buttons: 0, detail });
    await sleep(afterMs);
    return { ...client, ...gamePointFromClient(client.x, client.y) };
  };

  const clickGame = async (pointOrName, options = {}) => {
    const client = clientPoint(pointOrName);
    return clickClient(client.x, client.y, options);
  };

  const doubleClickGame = async (pointOrName, options = {}) => {
    const client = clientPoint(pointOrName);
    const first = await clickClient(client.x, client.y, {
      ...options,
      detail: 1,
      afterMs: options.betweenMs ?? 75,
    });
    await clickClient(client.x, client.y, {
      ...options,
      detail: 2,
      afterMs: options.afterMs ?? 100,
    });
    dispatchMouseAtClient('dblclick', client, { ...options, button: 0, buttons: 0, detail: 2 });
    await sleep(options.afterDoubleClickMs ?? 75);
    return first;
  };

  const clickFraction = async (xFraction, yFraction, options = {}) => {
    const info = metrics();
    return clickClient(
      info.left + (info.cssWidth * Number(xFraction)),
      info.top + (info.cssHeight * Number(yFraction)),
      options,
    );
  };

  const dragGame = async (fromPoint, toPoint, options = {}) => {
    const from = clientPoint(fromPoint);
    const to = clientPoint(toPoint);
    const steps = Math.max(1, Number(options.steps ?? 10));
    const element = requireCanvas();
    element.focus?.();
    dispatchMouseAtClient('mousemove', from, options);
    await sleep(options.beforeMs ?? 25);
    dispatchMouseAtClient('mousedown', from, options);
    for (let index = 1; index <= steps; index += 1) {
      const ratio = index / steps;
      dispatchMouseAtClient('mousemove', {
        x: from.x + ((to.x - from.x) * ratio),
        y: from.y + ((to.y - from.y) * ratio),
      }, options);
      await sleep(options.stepDelayMs ?? 15);
    }
    dispatchMouseAtClient('mouseup', to, options);
    await sleep(options.afterMs ?? 100);
    return { from, to };
  };

  const keyCodeFor = (key) => {
    if (key.length === 1) {
      const upper = key.toUpperCase();
      if (upper >= 'A' && upper <= 'Z') return upper.charCodeAt(0);
      if (upper >= '0' && upper <= '9') return upper.charCodeAt(0);
      if (key === '[') return 219;
      if (key === ']') return 221;
      if (key === '\\') return 220;
      return key.charCodeAt(0);
    }
    const special = {
      Backspace: 8,
      Tab: 9,
      Enter: 13,
      Shift: 16,
      Control: 17,
      Alt: 18,
      Escape: 27,
      Space: 32,
      ArrowLeft: 37,
      ArrowUp: 38,
      ArrowRight: 39,
      ArrowDown: 40,
      Delete: 46,
    };
    return special[key] ?? 0;
  };

  const codeFor = (key) => {
    if (key.length === 1) {
      const upper = key.toUpperCase();
      if (upper >= 'A' && upper <= 'Z') return `Key${upper}`;
      if (upper >= '0' && upper <= '9') return `Digit${upper}`;
      if (key === '[') return 'BracketLeft';
      if (key === ']') return 'BracketRight';
      if (key === '\\') return 'Backslash';
    }
    return key;
  };

  const dispatchKey = (type, key, options = {}) => {
    const keyCode = options.keyCode ?? keyCodeFor(key);
    const event = new KeyboardEvent(type, {
      bubbles: true,
      cancelable: true,
      key,
      code: options.code ?? codeFor(key),
      keyCode,
      which: keyCode,
      charCode: type === 'keypress' && key.length === 1 ? key.charCodeAt(0) : 0,
      ctrlKey: Boolean(options.ctrlKey),
      shiftKey: Boolean(options.shiftKey),
      altKey: Boolean(options.altKey),
      metaKey: Boolean(options.metaKey),
    });
    (document.activeElement || requireCanvas()).dispatchEvent(event);
    document.dispatchEvent(event);
    globalThis.dispatchEvent(event);
    return event;
  };

  const pressKey = async (key, options = {}) => {
    dispatchKey('keydown', key, options);
    if (key.length === 1) {
      dispatchKey('keypress', key, options);
    }
    await sleep(options.holdMs ?? 20);
    dispatchKey('keyup', key, options);
    await sleep(options.afterMs ?? 20);
  };

  const keyChord = async (keys, options = {}) => {
    const parts = Array.isArray(keys) ? keys : String(keys).split('+');
    const modifiers = parts.slice(0, -1);
    const key = parts[parts.length - 1];
    const modifierState = {
      ctrlKey: modifiers.some((part) => /^ctrl|control$/i.test(part)),
      shiftKey: modifiers.some((part) => /^shift$/i.test(part)),
      altKey: modifiers.some((part) => /^alt$/i.test(part)),
      metaKey: modifiers.some((part) => /^meta|cmd|command$/i.test(part)),
    };
    for (const modifier of modifiers) {
      await pressKey(modifier, { ...options, afterMs: 0 });
    }
    await pressKey(key, { ...options, ...modifierState });
    for (const modifier of modifiers.reverse()) {
      dispatchKey('keyup', modifier, options);
    }
  };

  const typeText = async (text, options = {}) => {
    requireCanvas().focus?.();
    for (const character of String(text)) {
      await pressKey(character, { afterMs: options.keyDelayMs ?? 30 });
    }
  };

  const openPanel = async (name, options = {}) => {
    const key = panelHotkeys[String(name)];
    if (!key) {
      throw new Error(`Unknown panel '${name}'. Known panels: ${Object.keys(panelHotkeys).sort().join(', ')}`);
    }
    await pressKey(key, { afterMs: options.afterMs ?? 250 });
    return { panel: String(name), key };
  };

  const focusChat = async (options = {}) => {
    await pressKey('Enter', { afterMs: options.afterMs ?? 150 });
  };

  const sendChatViaUi = async (message, options = {}) => {
    await focusChat(options);
    await typeText(String(message), { keyDelayMs: options.keyDelayMs ?? 15 });
    await pressKey('Enter', { afterMs: options.afterMs ?? 150 });
  };

  const runSequence = async (steps, options = {}) => {
    const results = [];
    for (const [index, step] of steps.entries()) {
      const action = typeof step === 'string' ? step : step.action;
      if (action === 'click') {
        results.push({ index, action, result: await clickGame(step.point ?? step.name ?? step) });
      } else if (action === 'clickClient') {
        results.push({ index, action, result: await clickClient(step.x, step.y, step) });
      } else if (action === 'clickFraction') {
        results.push({ index, action, result: await clickFraction(step.x, step.y, step) });
      } else if (action === 'doubleClick') {
        results.push({ index, action, result: await doubleClickGame(step.point, step) });
      } else if (action === 'drag') {
        results.push({ index, action, result: await dragGame(step.from, step.to, step) });
      } else if (action === 'key') {
        await pressKey(step.key, step);
        results.push({ index, action, key: step.key });
      } else if (action === 'chord') {
        await keyChord(step.keys, step);
        results.push({ index, action, keys: step.keys });
      } else if (action === 'type') {
        await typeText(step.text, step);
        results.push({ index, action, text: step.text });
      } else if (action === 'panel') {
        results.push({ index, action, result: await openPanel(step.panel, step) });
      } else if (action === 'chat') {
        await sendChatViaUi(step.message, step);
        results.push({ index, action, message: step.message });
      } else if (action === 'wait') {
        await sleep(step.ms ?? options.defaultWaitMs ?? 250);
        results.push({ index, action, ms: step.ms ?? options.defaultWaitMs ?? 250 });
      } else {
        throw new Error(`Unsupported UI sequence action at index ${index}: ${action}`);
      }
    }
    return results;
  };

  const callOptional = (name, returnType = 'number', argTypes = [], args = [], fallback = null) => {
    const module = globalThis.Module;
    if (typeof module?.ccall !== 'function' || typeof module[`_${name}`] !== 'function') {
      return fallback;
    }
    try {
      return module.ccall(name, returnType, argTypes, args);
    } catch (error) {
      return fallback;
    }
  };

  const playerSnapshot = () => ({
    name: callOptional('maple_get_player_name', 'string', [], [], null),
    mapId: callOptional('maple_get_player_mapid', 'number', [], [], null),
    x: callOptional('maple_get_player_position_x', 'number', [], [], null),
    y: callOptional('maple_get_player_position_y', 'number', [], [], null),
    level: callOptional('maple_get_player_level', 'number', [], [], null),
    job: callOptional('maple_get_player_job', 'number', [], [], null),
  });

  const waitForPlayerMap = async (timeoutMs = 15000) => {
    const deadline = Date.now() + Number(timeoutMs);
    while (Date.now() < deadline) {
      const player = playerSnapshot();
      if (Number.isInteger(player.mapId) && player.mapId > 0) {
        return player;
      }
      await sleep(250);
    }
    return playerSnapshot();
  };

  const api = {
    version: 3,
    points,
    panelHotkeys,
    sleep,
    canvas,
    metrics,
    normalizePoint,
    clientPoint,
    gamePointFromClient,
    registerPoint,
    listPoints,
    moveClient,
    clickClient,
    clickGame,
    doubleClickGame,
    clickFraction,
    dragGame,
    pressKey,
    keyChord,
    typeText,
    openPanel,
    focusChat,
    sendChatViaUi,
    runSequence,
    playerSnapshot,
    waitForPlayerMap,
  };

  globalThis.MapleUi = Object.freeze(api);
  return globalThis.MapleUi;
})();
