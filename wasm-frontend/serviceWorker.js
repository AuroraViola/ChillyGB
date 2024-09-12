const cacheName = "chillygb";
const assets = [
    "/",
    "/icon.svg",
    "/index.data",
    "/index.js",
    "/index.wasm"
]

self.addEventListener("install", installEvent => {
    console.log("Service Worker Installed");
    installEvent.waitUntil(
        caches.open(cacheName).then(cache => {
            cache.addAll(assets)
        })
    )
});

self.addEventListener("fetch", fetchEvent => {
    fetchEvent.respondWith(
        caches.match(fetchEvent.request).then(res => {
            return res || fetch(fetchEvent.request)
        })
    )
});
