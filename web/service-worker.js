// MapleStory WASM - Service Worker
// Cache-first for large static assets (WASM, JS), stale-while-revalidate for HTML/config

const CACHE_VERSION = 'maple-v1';
const WASM_CACHE = `${CACHE_VERSION}-wasm`;
const STATIC_CACHE = `${CACHE_VERSION}-static`;

// Assets that benefit from aggressive caching
const WASM_PATTERNS = ['/build/JourneyClient.wasm', '/build/JourneyClient.js'];
const STATIC_PATTERNS = ['/web/config.json'];

self.addEventListener('install', (event) => {
  console.log('[SW] Installing service worker', CACHE_VERSION);
  self.skipWaiting();
});

self.addEventListener('activate', (event) => {
  console.log('[SW] Activating service worker');
  event.waitUntil(
    caches.keys().then((keys) => {
      return Promise.all(
        keys
          .filter((key) => !key.startsWith(CACHE_VERSION))
          .map((key) => {
            console.log('[SW] Deleting old cache:', key);
            return caches.delete(key);
          })
      );
    }).then(() => self.clients.claim())
  );
});

self.addEventListener('fetch', (event) => {
  const url = new URL(event.request.url);

  // Only handle same-origin GET requests
  if (event.request.method !== 'GET' || url.origin !== self.location.origin) {
    return;
  }

  // WASM/JS build assets: cache-first (these are large and rarely change)
  if (WASM_PATTERNS.some((p) => url.pathname === p)) {
    event.respondWith(cacheFirst(event.request, WASM_CACHE));
    return;
  }

  // HTML: network-first so updates are picked up quickly
  if (url.pathname === '/' || url.pathname.endsWith('.html')) {
    event.respondWith(networkFirst(event.request, STATIC_CACHE));
    return;
  }

  // Config: stale-while-revalidate
  if (STATIC_PATTERNS.some((p) => url.pathname === p)) {
    event.respondWith(staleWhileRevalidate(event.request, STATIC_CACHE));
    return;
  }

  // Everything else: pass through (asset chunks use WebSocket, not fetch)
});

async function cacheFirst(request, cacheName) {
  const cached = await caches.match(request);
  if (cached) {
    console.log('[SW] Cache hit:', request.url);
    return cached;
  }

  console.log('[SW] Cache miss, fetching:', request.url);
  const response = await fetch(request);
  if (response.ok) {
    const cache = await caches.open(cacheName);
    cache.put(request, response.clone());
  }
  return response;
}

async function networkFirst(request, cacheName) {
  try {
    const response = await fetch(request);
    if (response.ok) {
      const cache = await caches.open(cacheName);
      cache.put(request, response.clone());
    }
    return response;
  } catch (e) {
    const cached = await caches.match(request);
    if (cached) {
      return cached;
    }
    throw e;
  }
}

async function staleWhileRevalidate(request, cacheName) {
  const cache = await caches.open(cacheName);
  const cached = await cache.match(request);

  const fetchPromise = fetch(request).then((response) => {
    if (response.ok) {
      cache.put(request, response.clone());
    }
    return response;
  }).catch(() => cached);

  return cached || fetchPromise;
}
