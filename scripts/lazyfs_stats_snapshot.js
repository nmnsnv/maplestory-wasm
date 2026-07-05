(() => {
  const lazyFs = globalThis.LazyFS ?? globalThis.Module?.LazyFS;
  const generatedAt = new Date().toISOString();

  const download = (value) => {
    const blob = new Blob([JSON.stringify(value, null, 2), '\n'], { type: 'application/json' });
    const url = URL.createObjectURL(blob);
    const link = document.createElement('a');
    link.href = url;
    link.download = 'lazyfs_stats_snapshot.json';
    link.click();
    URL.revokeObjectURL(url);
  };

  if (!lazyFs) {
    const unavailable = { generatedAt, available: false, error: 'LazyFS unavailable.' };
    globalThis.lazyFsStatsSnapshot = unavailable;
    if (globalThis.lazyFsStatsDownload) {
      download(unavailable);
    }
    return unavailable;
  }

  const stats = lazyFs.stats ?? {};
  const totalRequests = Number(stats.totalRequests ?? 0);
  const cacheHits = Number(stats.cacheHits ?? 0);
  const startTime = Number(stats.startTime ?? 0);
  const snapshot = {
    generatedAt,
    available: true,
    wsConnected: Boolean(lazyFs.wsConnected),
    wsConnecting: Boolean(lazyFs.wsConnecting),
    queuedRequests: Array.isArray(lazyFs.wsQueue) ? lazyFs.wsQueue.length : 0,
    pendingRequests: lazyFs.pendingRequests && typeof lazyFs.pendingRequests === 'object'
      ? Object.keys(lazyFs.pendingRequests).length
      : 0,
    requestIdCounter: Number(lazyFs.requestIdCounter ?? 0),
    chunkCacheEntries: lazyFs.chunkCache && typeof lazyFs.chunkCache === 'object'
      ? Object.keys(lazyFs.chunkCache).length
      : 0,
    registeredFiles: lazyFs.files && typeof lazyFs.files === 'object'
      ? Object.keys(lazyFs.files).length
      : 0,
    dbReady: Boolean(lazyFs.dbReady),
    stats: {
      totalRequests,
      totalBytes: Number(stats.totalBytes ?? 0),
      cacheHits,
      cacheMisses: Number(stats.cacheMisses ?? 0),
      hitRate: totalRequests > 0 ? cacheHits / totalRequests : null,
      elapsedMs: startTime > 0 ? Date.now() - startTime : null,
    },
  };

  globalThis.lazyFsStatsSnapshot = snapshot;
  console.log('[lazyfs-stats] Snapshot captured.', snapshot);
  if (globalThis.lazyFsStatsDownload) {
    download(snapshot);
  }
  return snapshot;
})();
