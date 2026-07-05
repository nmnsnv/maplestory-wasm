(async () => {
  const module = globalThis.Module;

  const sleep = (ms) => new Promise((resolve) => setTimeout(resolve, ms));
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

  const waitForDialogue = async (timeoutMs) => {
    const deadline = Date.now() + timeoutMs;
    while (Date.now() < deadline) {
      const mode = call('maple_get_dialogue_mode');
      if (mode !== 0) {
        return mode;
      }
      await sleep(100);
    }
    return 0;
  };

  const readSelections = () => {
    const count = call('maple_get_dialogue_selection_count');
    const selections = [];
    for (let i = 0; i < count; i += 1) {
      selections.push(call('maple_get_dialogue_selection', 'string', ['number'], [i]) ?? '');
    }
    return selections;
  };

  const chooseAction = (mode) => {
    if (mode === 4) return 6;      // select first option
    if (mode === 2 || mode === 3) return 4; // yes / accept
    return 1;                      // next / ok for text and unknown modes
  };

  if (!globalThis.dialogueInspectorSkipTalk) {
    const talked = call('maple_talk_to_nearest_npc') !== 0;
    if (!talked) {
      throw new Error('No nearby scripted NPC responded to talk request.');
    }
  }

  const timeoutMs = Number(globalThis.dialogueTimeoutMs ?? 5000);
  const maxSteps = Number(globalThis.dialogueMaxSteps ?? 40);
  let mode = await waitForDialogue(timeoutMs);
  if (mode === 0) {
    throw new Error(`NPC did not respond with dialogue within ${timeoutMs}ms.`);
  }

  const conversation = [];
  for (let step = 0; step < maxSteps; step += 1) {
    mode = call('maple_get_dialogue_mode');
    if (mode === 0) {
      break;
    }

    const page = {
      step,
      mode,
      npcId: call('maple_get_dialogue_npcid'),
      type: call('maple_get_dialogue_type'),
      text: call('maple_get_dialogue_text', 'string', [], []) ?? '',
      selections: readSelections(),
    };
    conversation.push(page);
    console.log(`[dialogue] #${step} mode=${mode} npc=${page.npcId}:`, page.text, page.selections);

    const advanced = call('maple_advance_dialogue', 'number', ['number'], [chooseAction(mode)]) !== 0;
    if (!advanced) {
      break;
    }
    await sleep(650);
  }

  globalThis.dialogueInspection = conversation;
  console.log(`[dialogue] Captured ${conversation.length} dialogue page(s).`);
  return conversation;
})();
