"""
Simple HTTP server for serving MapleStory WASM client files.

This server provides the necessary CORS and COOP/COEP headers for SharedArrayBuffer
support, which is required for the WASM client with threading.

For the WASM client to connect to the MapleStory server, you also need to run
the WebSocket-to-TCP proxy:
    python web/ws_proxy.py

The proxy bridges WebSocket connections from the browser to TCP connections
to the actual MapleStory server.
"""

import http.server
import socketserver
import os

PORT = 8000
# Serve from parent directory (project root) to access assets/
DIRECTORY = os.path.join(os.path.dirname(__file__), "..")

class Handler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=DIRECTORY, **kwargs)

    def end_headers(self):
        self.send_header("Cross-Origin-Opener-Policy", "same-origin")
        self.send_header("Cross-Origin-Embedder-Policy", "require-corp")
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Cache-Control", "no-store, no-cache, must-revalidate")
        super().end_headers()

    def do_GET(self):
        """Serve a GET request."""
        range_header = self.headers.get("Range")
        if range_header:
            self.send_range_response(range_header)
        else:
            super().do_GET()

    def send_range_response(self, range_header):
        path = self.translate_path(self.path)
        if not os.path.exists(path) or os.path.isdir(path):
            self.send_error(404, "File not found")
            return

        file_size = os.path.getsize(path)
        try:
            start, end = self.parse_range_header(range_header, file_size)
        except ValueError:
            self.send_error(416, "Range Not Satisfiable")
            self.send_header("Content-Range", f"bytes */{file_size}")
            self.end_headers()
            return

        chunk_size = end - start + 1

        self.send_response(206)
        self.send_header("Content-Type", self.guess_type(path))
        self.send_header("Content-Range", f"bytes {start}-{end}/{file_size}")
        self.send_header("Content-Length", str(chunk_size))
        self.send_header("Accept-Ranges", "bytes")
        self.end_headers()

        with open(path, "rb") as f:
            f.seek(start)
            self.wfile.write(f.read(chunk_size))

        # Log Range request for LazyFS debugging
        chunk_mb = chunk_size / (1024 * 1024)
        print(f"[Range] {self.path} - bytes {start}-{end}/{file_size} ({chunk_mb:.2f} MB)")

    def parse_range_header(self, range_header, file_size):
        if not range_header.startswith("bytes="):
            raise ValueError("Invalid Range header")

        ranges = range_header.replace("bytes=", "").split("-")
        if len(ranges) != 2:
             raise ValueError("Invalid Range header format")

        start_str, end_str = ranges

        if start_str and end_str:
            start = int(start_str)
            end = int(end_str)
        elif start_str:
            start = int(start_str)
            end = file_size - 1
        elif end_str:
             # suffix byte range: bytes=-500 means last 500 bytes
            end = file_size - 1
            start = file_size - int(end_str)
        else:
            raise ValueError("Invalid Range header values")

        if start < 0: start = 0
        if end >= file_size: end = file_size - 1
        if start > end:
             raise ValueError("Invalid Range header range")

        return start, end

    def do_HEAD(self):
        """Handle HEAD requests for file metadata."""
        super().do_HEAD()
        path = self.translate_path(self.path)
        if os.path.exists(path) and not os.path.isdir(path):
            file_size = os.path.getsize(path)
            size_mb = file_size / (1024 * 1024)
            print(f"[HEAD] {self.path} - {file_size} bytes ({size_mb:.2f} MB)")

if __name__ == "__main__":
    # Allow address reuse to avoid "Address already in use" errors on restart
    socketserver.TCPServer.allow_reuse_address = True
    with socketserver.TCPServer(("", PORT), Handler) as httpd:
        print("=" * 60)
        print("MapleStory WASM Server with HTTP Range Request Support")
        print("=" * 60)
        print(f"Serving at http://localhost:{PORT}")
        print(f"Directory: {os.path.abspath(DIRECTORY)}")
        print()
        print("Features:")
        print("  ✓ HTTP Range requests (206 Partial Content)")
        print("  ✓ CORS headers for cross-origin access")
        print("  ✓ COOP/COEP headers for SharedArrayBuffer")
        print("  ✓ LazyFS asset loading support")
        print()
        print("Press Ctrl+C to stop the server.")
        print("=" * 60)
        print()
        httpd.serve_forever()
