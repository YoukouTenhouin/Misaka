import json
import os
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import subprocess
import sys
import threading
from urllib.parse import parse_qs, urlsplit


class Fixture(BaseHTTPRequestHandler):
    def do_GET(self):
        url = urlsplit(self.path)
        query = parse_qs(url.query)
        base = f"http://127.0.0.1:{self.server.server_port}"
        if url.path in ("/video.mp4", "/audio.mp4"):
            content = (Path(__file__).parent / "media" / url.path[1:]).read_bytes()
            mime = "video/mp4"
        elif url.path.startswith("/qr/"):
            if url.path.endswith("/generate"):
                data = {"url": "https://example.test/login", "qrcode_key": "fixture"}
            elif url.path.endswith("/poll"):
                data = {"code": 0}
            elif url.path.endswith("/nav") and "SESSDATA=test" in self.headers.get("Cookie", ""):
                data = {"isLogin": True, "uname": "测试账号"}
            else:
                return self.send_error(400)
            content = json.dumps({"code": 0, "data": data}).encode()
            mime = "application/json"
        elif url.path.startswith("/account/"):
            if url.path.endswith("/nav"):
                if not getattr(self.server, "refused", False):
                    self.server.refused = True
                    return self.send_error(412)
                result = {"code": -101}
            else:
                return self.send_error(404)
            content = json.dumps(result).encode()
            mime = "application/json"
        else:
            if query.get("bvid") != ["BV17x411w7KC"]:
                return self.send_error(404)
            if url.path == "/x/player/pagelist":
                data = [{"page": 1, "cid": 12, "part": "First"},
                        {"page": 2, "cid": 23, "part": "Second"}]
            elif url.path == "/x/player/wbi/playurl" and query.get("cid") == ["12"]:
                data = {"dash": {kind: [{"baseUrl": f"{base}/{kind}.mp4"}]
                                 for kind in ("video", "audio")}}
            else:
                return self.send_error(404)
            content = json.dumps({"code": 0, "data": data}).encode()
            mime = "application/json"
        total = len(content)
        requested = self.headers.get("Range")
        if requested and mime == "video/mp4":
            start, end = requested.removeprefix("bytes=").split("-", 1)
            start, end = int(start), int(end) if end else total - 1
            content = content[start:end + 1]
            self.send_response(206)
            self.send_header("Content-Range", f"bytes {start}-{end}/{total}")
        else:
            self.send_response(200)
        if url.path.endswith("/poll"):
            self.send_header("Set-Cookie", "SESSDATA=test; Path=/; HttpOnly")
        self.send_header("Accept-Ranges", "bytes")
        self.send_header("Content-Type", mime)
        self.send_header("Content-Length", str(len(content)))
        self.end_headers()
        self.wfile.write(content)


with ThreadingHTTPServer(("127.0.0.1", 0), Fixture) as server:
    threading.Thread(target=server.serve_forever, daemon=True).start()
    env = dict(os.environ, MISAKA_API=f"http://127.0.0.1:{server.server_port}")
    sys.exit(subprocess.run(sys.argv[1:], env=env).returncode)
