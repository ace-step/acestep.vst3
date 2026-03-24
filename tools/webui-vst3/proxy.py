#!/usr/bin/env python3
"""Simple reverse proxy: serves static files + proxies API calls to ace-server."""

import http.server
import urllib.request
import os
import sys

ACE_SERVER = os.environ.get("ACE_SERVER", "http://127.0.0.1:8080")
PORT = int(os.environ.get("PORT", "8085"))
DIR = os.path.dirname(os.path.abspath(__file__))

API_PATHS = ("/props", "/lm", "/synth", "/understand", "/health")


class ProxyHandler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=DIR, **kwargs)

    def do_GET(self):
        path_base = self.path.split("?")[0]
        if path_base in API_PATHS:
            self._proxy()
        elif path_base == "/":
            self.path = "/index.html"
            super().do_GET()
        else:
            super().do_GET()

    def do_POST(self):
        self._proxy()

    def _proxy(self):
        url = ACE_SERVER + self.path
        body = None
        if self.command == "POST":
            length = int(self.headers.get("Content-Length", 0))
            body = self.rfile.read(length) if length else None

        req = urllib.request.Request(url, data=body, method=self.command)
        for key in ("Content-Type", "Accept"):
            val = self.headers.get(key)
            if val:
                req.add_header(key, val)

        try:
            with urllib.request.urlopen(req, timeout=300) as resp:
                self.send_response(resp.status)
                for key, val in resp.getheaders():
                    if key.lower() not in ("transfer-encoding", "connection"):
                        self.send_header(key, val)
                self.end_headers()
                while True:
                    chunk = resp.read(65536)
                    if not chunk:
                        break
                    self.wfile.write(chunk)
        except urllib.error.HTTPError as e:
            self.send_response(e.code)
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            self.wfile.write(e.read())
        except Exception as e:
            self.send_response(502)
            self.send_header("Content-Type", "text/plain")
            self.end_headers()
            self.wfile.write(str(e).encode())


if __name__ == "__main__":
    print(f"[VST3 UI] Serving {DIR} on http://127.0.0.1:{PORT}")
    print(f"[VST3 UI] Proxying API to {ACE_SERVER}")
    server = http.server.HTTPServer(("", PORT), ProxyHandler)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
