#!/usr/bin/env python3
# get_id.py — open http://localhost:8080 , solve the widget, id.bin is written
import http.server, socketserver, struct, socket, threading, webbrowser, sys

SERVER, PORT = ("118.25.42.70", 9000)
SITEKEY = "0x4AAAAAADNgS66XXyfkgQMZ"
OP_REGISTER, OK = 1, 0

HTML = """<!doctype html><html><head>
<script src="https://challenges.cloudflare.com/turnstile/v0/api.js" async defer></script>
</head><body>
<div class="cf-turnstile" data-sitekey="%s" data-callback="cb"></div>
<pre id="o"></pre>
<script>
function cb(t){fetch("/reg",{method:"POST",body:t})
 .then(r=>r.text()).then(x=>document.getElementById('o').textContent=x);}
</script></body></html>""" % SITEKEY

def recvn(s, n):
    buf = b""
    while len(buf) < n:
        c = s.recv(n - len(buf))
        if not c:
            raise ConnectionError("eof")
        buf += c
    return buf

def register(token: str) -> bytes:
    body = token.encode()
    frame = struct.pack(">BI", OP_REGISTER, len(body)) + body
    with socket.create_connection((SERVER, PORT), timeout=15) as s:
        s.sendall(frame)
        hdr = recvn(s, 5)
        status, n = hdr[0], struct.unpack(">I", hdr[1:5])[0]
        data = recvn(s, n) if n else b""
    if status != OK:
        raise RuntimeError(f"server status={status}")
    if len(data) != 32:
        raise RuntimeError(f"bad id len={len(data)}")
    return data

class H(http.server.BaseHTTPRequestHandler):
    def log_message(self, *a): pass
    def do_GET(self):
        self.send_response(200)
        self.send_header("content-type", "text/html; charset=utf-8")
        self.end_headers()
        self.wfile.write(HTML.encode())
    def do_POST(self):
        token = self.rfile.read(int(self.headers["content-length"])).decode()
        try:
            idb = register(token)
            with open("id.bin", "wb") as f:
                f.write(idb)
            msg = "OK id.bin written: " + idb.hex()
            print("\n" + msg)
            threading.Timer(1.0, lambda: sys.exit(0)).start()
        except Exception as e:
            msg = "ERR " + str(e)
            print("\n" + msg)
        self.send_response(200); self.end_headers()
        self.wfile.write(msg.encode())

if __name__ == "__main__":
    socketserver.TCPServer.allow_reuse_address = True
    with socketserver.TCPServer(("127.0.0.1", 8080), H) as httpd:
        webbrowser.open("http://localhost:8080")
        print("open http://localhost:8080 and solve the widget")
        httpd.serve_forever()