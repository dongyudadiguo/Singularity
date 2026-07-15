#!/usr/bin/env python3
"""Local CVM server (protocol-compatible with server.go). Data file: cvm_local.json"""
import json, hashlib, os, socket, struct, threading, time
from pathlib import Path

H = 32
OP_REGISTER, OP_UPLOAD, OP_FILE, OP_EDGE, OP_CHILDREN, OP_VOTE, OP_USET, OP_UGET = range(1, 9)
OK, ERR_BAD, ERR_DENY, ERR_NF, ERR_BIG, ERR_INNER = range(6)

DB_PATH = Path(__file__).resolve().parent / "cvm_local.json"
lock = threading.Lock()

def load():
    if DB_PATH.exists():
        try:
            return json.loads(DB_PATH.read_text(encoding="utf-8"))
        except Exception:
            pass
    return {"files": {}, "graph": {}, "users": {}, "vals": {}, "score": {}, "voted": {}, "seq": {}, "next": 0}

def save(db):
    tmp = DB_PATH.with_suffix(".tmp")
    tmp.write_text(json.dumps(db), encoding="utf-8")
    tmp.replace(DB_PATH)

db = load()

def hx(b: bytes) -> str:
    return b.hex()

def read_exact(conn, n):
    buf = b""
    while len(buf) < n:
        chunk = conn.recv(n - len(buf))
        if not chunk:
            return None
        buf += chunk
    return buf

def read_frame(conn):
    h = read_exact(conn, 5)
    if not h:
        return None, None
    op = h[0]
    n = struct.unpack(">I", h[1:5])[0]
    if n > 256 << 20:
        return op, None
    body = read_exact(conn, n) if n else b""
    if body is None:
        return None, None
    return op, body

def write_frame(conn, status, body=b""):
    if body is None:
        body = b""
    conn.sendall(bytes([status]) + struct.pack(">I", len(body)) + body)

def handle(op, body):
    global db
    with lock:
        if op == OP_REGISTER:
            ident = os.urandom(32)
            db["users"][hx(ident)] = True
            save(db)
            return OK, ident

        if op == OP_UPLOAD:
            if not body:
                return ERR_BAD, b""
            h = hashlib.sha256(body).digest()
            key = hx(h)
            if key not in db["files"]:
                db["files"][key] = body.hex()
                save(db)
            return OK, h

        if op == OP_FILE:
            if len(body) != 32:
                return ERR_BAD, b""
            key = hx(body)
            raw = db["files"].get(key)
            if raw is None:
                return ERR_NF, b""
            return OK, bytes.fromhex(raw)

        if op == OP_EDGE:
            if len(body) != 64:
                return ERR_BAD, b""
            parent, child = hx(body[:32]), hx(body[32:])
            xs = db["graph"].setdefault(parent, [])
            if child not in xs:
                xs.append(child)
                save(db)
            return OK, b""

        if op == OP_CHILDREN:
            if len(body) != 32:
                return ERR_BAD, b""
            parent = hx(body)
            kids = list(db["graph"].get(parent, []))
            def score(c):
                return int(db["score"].get(f"{parent}:{c}", 0))
            def seq(c):
                return int(db["seq"].get(f"{parent}:{c}", 0))
            kids.sort(key=lambda c: (-score(c), -seq(c)))
            out = struct.pack(">I", len(kids))
            for c in kids:
                out += bytes.fromhex(c)
                out += struct.pack(">Q", score(c) & 0xFFFFFFFFFFFFFFFF)
            return OK, out

        if op == OP_VOTE:
            if len(body) != 96:
                return ERR_BAD, b""
            user, parent, child = hx(body[:32]), hx(body[32:64]), hx(body[64:])
            if user not in db["users"]:
                db["users"][user] = True
            if child not in db["graph"].get(parent, []):
                return ERR_BAD, b""
            vk = f"{user}:{parent}:{child}"
            e = f"{parent}:{child}"
            if not db["voted"].get(vk):
                db["voted"][vk] = True
                db["score"][e] = int(db["score"].get(e, 0)) + 1
            db["next"] = int(db.get("next", 0)) + 1
            db["seq"][e] = db["next"]
            save(db)
            return OK, b""

        if op == OP_USET:
            if len(body) != 96:
                return ERR_BAD, b""
            user, key, val = hx(body[:32]), hx(body[32:64]), hx(body[64:])
            if user not in db["users"]:
                db["users"][user] = True
            db["vals"][f"{user}:{key}"] = val
            save(db)
            return OK, b""

        if op == OP_UGET:
            if len(body) != 64:
                return ERR_BAD, b""
            user, key = hx(body[:32]), hx(body[32:])
            if user not in db["users"]:
                db["users"][user] = True
            val = db["vals"].get(f"{user}:{key}")
            if val is None:
                return ERR_NF, b""
            return OK, bytes.fromhex(val)

        return ERR_BAD, b""

def client(conn, addr):
    try:
        while True:
            op, body = read_frame(conn)
            if op is None:
                break
            if body is None:
                write_frame(conn, ERR_BIG, b"")
                continue
            status, out = handle(op, body)
            write_frame(conn, status, out)
    except Exception:
        pass
    finally:
        conn.close()

def main():
    host, port = "127.0.0.1", 9000
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    s.bind((host, port))
    s.listen(32)
    print(f"CVM local Python server on {host}:{port}", flush=True)
    while True:
        c, a = s.accept()
        threading.Thread(target=client, args=(c, a), daemon=True).start()

if __name__ == "__main__":
    main()
