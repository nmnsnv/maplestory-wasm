(async () => {
  const loadMapleAi = async () => {
    if (globalThis.MapleAi?.version >= 2) return globalThis.MapleAi;
    const response = await fetch('/scripts/maple_ai_common.js', { cache: 'no-store' });
    if (!response.ok) throw new Error(`Could not load maple_ai_common.js: HTTP ${response.status}`);
    await globalThis.eval(await response.text());
    return globalThis.MapleAi;
  };

  const MapleAi = await loadMapleAi();
  const errors = [];

  const copyValue = (value) => {
    if (value === undefined || value === null) {
      return null;
    }
    try {
      return JSON.parse(JSON.stringify(value));
    } catch (error) {
      return { error: error instanceof Error ? error.message : String(error) };
    }
  };

  const summarizeReport = (report) => {
    if (!report || typeof report !== 'object') {
      return null;
    }

    return {
      generatedAt: report.generatedAt ?? null,
      totalQuests: Number.isInteger(report.totalQuests) ? report.totalQuests : null,
      planned: Number.isInteger(report.planned) ? report.planned : null,
      tested: Number.isInteger(report.tested) ? report.tested : null,
      passed: Number.isInteger(report.passed) ? report.passed : null,
      failed: Number.isInteger(report.failed) ? report.failed : null,
      firstFailure: copyValue(report.failures?.[0]),
    };
  };

  const captureSection = (label, fn, fallback) => {
    try {
      return fn();
    } catch (error) {
      const message = error instanceof Error ? error.message : String(error);
      errors.push(`${label}: ${message}`);
      return fallback;
    }
  };

  const snapshot = {
    generatedAt: new Date().toISOString(),
    url: String(globalThis.location?.href ?? ''),
    player: captureSection('player', () => MapleAi.getPlayerState(), null),
    runtimeDiagnostics: captureSection('runtimeDiagnostics', () => MapleAi.getRuntimeDiagnostics(), null),
    npcs: captureSection('npcs', () => MapleAi.listNpcs(), []),
    dialogue: captureSection('dialogue', () => {
      const mode = MapleAi.getDialogueMode();
      return mode !== 0 ? MapleAi.readDialoguePage(0) : null;
    }, null),
    questTestProgress: captureSection('questTestProgress', () => copyValue(globalThis.questTestProgress), null),
    questTestReportSummary: captureSection('questTestReportSummary', () => summarizeReport(globalThis.questTestReport), null),
    fullQuestTestReportSummary: captureSection('fullQuestTestReportSummary', () => summarizeReport(globalThis.fullQuestTestReport), null),
    errors,
  };

  globalThis.mapleAutomationSnapshot = snapshot;
  console.log('[automation-snapshot]', snapshot);

  if (globalThis.automationSnapshotDownload) {
    MapleAi.downloadJson(snapshot, 'automation_snapshot.json');
  }

  return snapshot;
})();
