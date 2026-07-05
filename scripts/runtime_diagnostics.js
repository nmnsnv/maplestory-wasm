(() => {
  const existing = globalThis.MapleRuntimeDiagnostics;
  if (existing?.version >= 1) {
    return existing.snapshot();
  }

  const state = globalThis.__MapleRuntimeDiagnosticsState ?? {
    installedAt: new Date().toISOString(),
    entries: [],
    errors: [],
    rejections: [],
    original: { console: {}, module: {} },
    installed: false,
  };
  globalThis.__MapleRuntimeDiagnosticsState = state;

  const limit = () => {
    const value = Number(globalThis.runtimeDiagnosticsLimit ?? 500);
    return Number.isInteger(value) && value > 0 ? value : 500;
  };

  const trim = (array) => {
    const max = limit();
    if (array.length > max) {
      array.splice(0, array.length - max);
    }
  };

  const formatArg = (value) => {
    if (value instanceof Error) {
      return value.stack || value.message;
    }
    if (typeof value === 'string') {
      return value;
    }
    try {
      return JSON.stringify(value);
    } catch (error) {
      return String(value);
    }
  };

  const pushEntry = (source, level, args) => {
    const entry = {
      at: new Date().toISOString(),
      source,
      level,
      message: Array.from(args).map(formatArg).join(' '),
    };
    state.entries.push(entry);
    trim(state.entries);
    if (level === 'error') {
      state.errors.push(entry);
      trim(state.errors);
    }
    return entry;
  };

  const wrapConsole = () => {
    for (const level of ['log', 'warn', 'error', 'debug']) {
      if (state.original.console[level]) {
        continue;
      }
      const original = typeof console[level] === 'function' ? console[level].bind(console) : console.log.bind(console);
      state.original.console[level] = original;
      console[level] = (...args) => {
        pushEntry('console', level, args);
        return original(...args);
      };
    }
  };

  const wrapWindowErrors = () => {
    if (!state.original.onerrorCaptured) {
      state.original.onerrorCaptured = true;
      state.original.onerror = globalThis.onerror;
      globalThis.onerror = function onMapleRuntimeError(message, source, lineno, colno, error) {
        pushEntry('window.onerror', 'error', [message, source, lineno, colno, error]);
        if (typeof state.original.onerror === 'function') {
          return state.original.onerror.apply(this, arguments);
        }
        return false;
      };
    }

    if (!state.original.onunhandledrejectionCaptured) {
      state.original.onunhandledrejectionCaptured = true;
      state.original.onunhandledrejection = globalThis.onunhandledrejection;
      globalThis.onunhandledrejection = function onMapleUnhandledRejection(event) {
        const reason = event?.reason ?? event;
        const entry = pushEntry('window.onunhandledrejection', 'error', [reason]);
        state.rejections.push(entry);
        trim(state.rejections);
        if (typeof state.original.onunhandledrejection === 'function') {
          return state.original.onunhandledrejection.apply(this, arguments);
        }
        return undefined;
      };
    }
  };

  const wrapModuleFunction = (name, source, levelForArgs) => {
    const module = globalThis.Module;
    if (!module || state.original.module[name] || typeof module[name] !== 'function') {
      return;
    }
    const original = module[name].bind(module);
    state.original.module[name] = original;
    module[name] = (...args) => {
      const level = typeof levelForArgs === 'function' ? levelForArgs(args) : levelForArgs;
      pushEntry(source, level, args);
      return original(...args);
    };
  };

  const wrapModuleLogging = () => {
    wrapModuleFunction('print', 'Module.print', 'log');
    wrapModuleFunction('printErr', 'Module.printErr', 'error');
    wrapModuleFunction('addCppLog', 'Module.addCppLog', (args) => String(args[0] ?? '').toLowerCase() === 'error' ? 'error' : 'log');
  };

  wrapConsole();
  wrapWindowErrors();
  wrapModuleLogging();

  const diagnostics = Object.freeze({
    version: 1,
    note: 'Captures logs only after scripts/runtime_diagnostics.js is loaded.',
    snapshot() {
      wrapModuleLogging();
      return {
        generatedAt: new Date().toISOString(),
        installedAt: state.installedAt,
        version: 1,
        note: diagnostics.note,
        entries: state.entries.slice(),
        errors: state.errors.slice(),
        rejections: state.rejections.slice(),
        runtime: {
          url: String(globalThis.location?.href ?? ''),
          moduleReady: typeof globalThis.Module?.ccall === 'function',
          lazyFsAvailable: Boolean(globalThis.LazyFS ?? globalThis.Module?.LazyFS),
        },
      };
    },
    clear() {
      state.entries.length = 0;
      state.errors.length = 0;
      state.rejections.length = 0;
    },
  });

  globalThis.MapleRuntimeDiagnostics = diagnostics;
  console.log('[runtime-diagnostics] Installed. Future console, window, and Module logs will be captured.');
  return diagnostics.snapshot();
})();
