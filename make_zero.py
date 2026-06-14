import struct
import socket
import os
import sys
import random

# ========================== Block assembler ==========================
def tok(s):
    b = s.encode('ascii', errors='ignore')[:32]
    return b.ljust(32, b'\x00')

def u32le(x):
    return struct.pack('<I', x)

def item(op, payload=b''):
    token = tok(op)
    span = u32le(4 + len(payload))
    return token + span + payload

# Labels dictionary (global to the build process)
labels = {}
pending = []
# track byte offset of each emitted item index
item_offs = []

def byte_off(idx):
    """byte offset of block_list item at index idx"""
    return sum(len(block_list[i]) for i in range(idx))

def label(name):
    """Record label at current position (len of block_list)"""
    labels[name] = len(block_list)

def emit(op, payload=b''):
    """Append item directly to block_list"""
    it = item(op, payload)
    block_list.append(it)
    return it

def emit_const(x):
    emit("U32:CONST", u32le(x))

def emit_push_text(text):
    emit("ST:PUSH", text.encode('utf-8'))

def emit_draw_text(x, y, text):
    emit_const(x)
    emit_const(y)
    emit_push_text(text)
    emit("DRAW:TEXT")

def emit_jmp(target):
    if target in labels:
        emit("FLOW:JMP", u32le(byte_off(labels[target])))
    else:
        pending.append({'target': target, 'op': 'FLOW:JMP', 'fixup': len(block_list)})
        emit("FLOW:JMP", u32le(0))  # placeholder

def emit_jz(target):
    if target in labels:
        emit("FLOW:JZ", u32le(byte_off(labels[target])))
    else:
        pending.append({'target': target, 'op': 'FLOW:JZ', 'fixup': len(block_list)})
        emit("FLOW:JZ", u32le(0))

def emit_jnz(target):
    if target in labels:
        emit("FLOW:JNZ", u32le(byte_off(labels[target])))
    else:
        pending.append({'target': target, 'op': 'FLOW:JNZ', 'fixup': len(block_list)})
        emit("FLOW:JNZ", u32le(0))

def resolve_labels():
    for p in pending:
        target_off = byte_off(labels[p['target']])
        fixup_idx = p['fixup']
        block_list[fixup_idx] = item(p['op'], u32le(target_off))

# ========================== Build zero block ==========================
block_list = []

# ---- Init ----
label("START")
emit_push_text("init")
emit("TOK:MAKE")
emit("VAR:HAS")
emit_jnz("LOOP")

emit_push_text("init")
emit("TOK:MAKE")
emit_const(1)
emit("VAR:SET")

emit_push_text("parent")
emit("TOK:MAKE")
emit("TOK:ZERO")
emit("VAR:SET")

emit_push_text("prev")
emit("TOK:MAKE")
emit("LST:NEW")
emit("VAR:SET")

emit_push_text("sel")
emit("TOK:MAKE")
emit_const(0)
emit("VAR:SET")

emit_push_text("mode")
emit("TOK:MAKE")
emit_const(0)
emit("VAR:SET")

emit_push_text("CVM zero")
emit("UI:OPEN")

emit_jmp("LOOP")

# ---- Main loop ----
label("LOOP")
emit("UI:POLL")

emit_const(0x202020)
emit("DRAW:CLEAR")

emit_const(10)
emit_const(10)
emit_push_text("parent ")
emit_push_text("parent")
emit("TOK:MAKE")
emit("VAR:GET")
emit("BY:HEX")
emit("BY:CAT")
emit("DRAW:TEXT")

emit_push_text("parent")
emit("TOK:MAKE")
emit("VAR:GET")
emit("G:CHILDS")

emit("ST:DUP")
emit("CH:COUNT")
emit_push_text("count")
emit("TOK:MAKE")
emit("ST:SWAP")
emit("VAR:SET")

emit_const(10)
emit_const(30)
emit_push_text("children ")
emit_push_text("count")
emit("TOK:MAKE")
emit("VAR:GET")
emit("U32:DEC")
emit("BY:CAT")
emit("DRAW:TEXT")

emit_const(10)
emit_const(50)
emit_push_text("sel ")
emit_push_text("sel")
emit("TOK:MAKE")
emit("VAR:GET")
emit("U32:DEC")
emit("BY:CAT")
emit("DRAW:TEXT")

emit_push_text("count")
emit("TOK:MAKE")
emit("VAR:GET")
emit_const(0)
emit("U32:GT")
emit_jz("NO_CHILD")

emit_push_text("sel")
emit("TOK:MAKE")
emit("VAR:GET")
emit("ST:SWAP")
emit("CH:HASH")
emit_push_text("child")
emit("TOK:MAKE")
emit("ST:SWAP")
emit("VAR:SET")

emit_const(10)
emit_const(70)
emit_push_text("-> ")
emit_push_text("child")
emit("TOK:MAKE")
emit("VAR:GET")
emit("BY:HEX")
emit("BY:CAT")
emit("DRAW:TEXT")
emit_jmp("AFTER_DISP")

label("NO_CHILD")
emit("ST:POP")

label("AFTER_DISP")
emit_const(10)
emit_const(110)
emit_push_text("UP/DOWN:sel ENTER:enter BACK:back TAB:editor O:ov S:save P:pub R:reload")
emit("DRAW:TEXT")

emit("IN:KEY")

emit("ST:DUP")
emit("BY:LEN")
emit_const(0)
emit("U32:EQ")
emit_jz("HAS_KEY")

emit("ST:POP")
emit_jmp("SLEEP")

label("HAS_KEY")
emit("ST:DUP")
emit("KEY:CODE")
emit("KEY:UP")
emit("U32:EQ")
emit_jnz("DO_UP")

emit("ST:DUP")
emit("KEY:CODE")
emit("KEY:DOWN")
emit("U32:EQ")
emit_jnz("DO_DOWN")

emit("ST:DUP")
emit("KEY:CODE")
emit("KEY:ENTER")
emit("U32:EQ")
emit_jnz("DO_ENTER")

emit("ST:DUP")
emit("KEY:CODE")
emit("KEY:BACK")
emit("U32:EQ")
emit_jnz("DO_BACK")

emit("ST:DUP")
emit("KEY:CODE")
emit("KEY:TAB")
emit("U32:EQ")
emit_jnz("TOGGLE_MODE")

emit("ST:DUP")
emit("KEY:ASCII")
emit("ST:DUP")
emit_const(79)   # 'O'
emit("U32:EQ")
emit_jnz("DO_OV")

emit("ST:DUP")
emit_const(83)   # 'S'
emit("U32:EQ")
emit_jnz("DO_SAVE")

emit("ST:DUP")
emit_const(80)   # 'P'
emit("U32:EQ")
emit_jnz("DO_PUBLISH")

emit("ST:DUP")
emit_const(82)   # 'R'
emit("U32:EQ")
emit_jnz("DO_RELOAD")

emit("ST:POP")
emit("ST:POP")
emit("ST:POP")
emit_jmp("SLEEP")

# ---- Handlers ----
label("DO_UP")
emit("ST:POP")
emit_push_text("sel")
emit("TOK:MAKE")
emit("VAR:GET")
emit_const(0)
emit("U32:GT")
emit_jz("SLEEP")
emit_push_text("sel")
emit("TOK:MAKE")
emit_push_text("sel")
emit("TOK:MAKE")
emit("VAR:GET")
emit_const(1)
emit("U32:SUB")
emit("VAR:SET")
emit_jmp("SLEEP")

label("DO_DOWN")
emit("ST:POP")
emit_push_text("sel")
emit("TOK:MAKE")
emit("VAR:GET")
emit_const(1)
emit("U32:ADD")
emit_push_text("count")
emit("TOK:MAKE")
emit("VAR:GET")
emit_const(1)
emit("U32:SUB")
emit("U32:GT")
emit_jz("SLEEP")
emit_push_text("sel")
emit("TOK:MAKE")
emit_push_text("sel")
emit("TOK:MAKE")
emit("VAR:GET")
emit_const(1)
emit("U32:ADD")
emit("VAR:SET")
emit_jmp("SLEEP")

label("DO_ENTER")
emit("ST:POP")
emit_push_text("prev")
emit("TOK:MAKE")
emit_push_text("prev")
emit("TOK:MAKE")
emit("VAR:GET")
emit_push_text("parent")
emit("TOK:MAKE")
emit("VAR:GET")
emit("LST:PUSH")
emit("VAR:SET")
emit_push_text("parent")
emit("TOK:MAKE")
emit_push_text("child")
emit("TOK:MAKE")
emit("VAR:GET")
emit("VAR:SET")
emit_push_text("sel")
emit("TOK:MAKE")
emit_const(0)
emit("VAR:SET")
emit_jmp("SLEEP")

label("DO_BACK")
emit("ST:POP")
emit_push_text("prev")
emit("TOK:MAKE")
emit("VAR:GET")
emit("LST:COUNT")
emit_const(0)
emit("U32:EQ")
emit_jz("DO_BACK1")
emit_jmp("SLEEP")

label("DO_BACK1")
emit_push_text("prev")
emit("TOK:MAKE")
emit("VAR:GET")
emit("LST:COUNT")
emit_const(1)
emit("U32:SUB")
emit("LST:GET")
emit_push_text("parent")
emit("TOK:MAKE")
emit("ST:SWAP")
emit("VAR:SET")
emit_push_text("prev")
emit("TOK:MAKE")
emit("VAR:GET")
emit("LST:COUNT")
emit_const(1)
emit("U32:SUB")
emit("LST:DEL")
emit_push_text("prev")
emit("TOK:MAKE")
emit("ST:SWAP")
emit("VAR:SET")
emit_push_text("sel")
emit("TOK:MAKE")
emit_const(0)
emit("VAR:SET")
emit_jmp("SLEEP")

label("TOGGLE_MODE")
emit("ST:POP")
emit_push_text("mode")
emit("TOK:MAKE")
emit_push_text("mode")
emit("TOK:MAKE")
emit("VAR:GET")
emit_const(1)
emit("U32:XOR")
emit("VAR:SET")
emit_jmp("SLEEP")

label("DO_OV")
emit("ST:POP")
emit("ST:POP")
emit_push_text("parent")
emit("TOK:MAKE")
emit("VAR:GET")
emit("CUR:FILE")
emit("OV:SET")
emit_jmp("SLEEP")

label("DO_SAVE")
emit("ST:POP")
emit("ST:POP")
# TODO: needs user identity, skipped
emit_jmp("SLEEP")

label("DO_PUBLISH")
emit("ST:POP")
emit("ST:POP")
# TODO
emit_jmp("SLEEP")

label("DO_RELOAD")
emit("ST:POP")
emit("ST:POP")
emit_push_text("parent")
emit("TOK:MAKE")
emit("VAR:GET")
emit("CUR:KEY")
emit("OV:SET")
emit_jmp("SLEEP")

label("SLEEP")
emit_const(16)
emit("TIME:SLEEP")
emit_jmp("LOOP")

# End marker
emit("FLOW:END")

# Resolve forward labels
resolve_labels()

# -------------------- Build binary --------------------
zero_bin = b''.join(block_list) + b'\x00' * 32  # append 32-byte end marker

with open("zero.bin", "wb") as f:
    f.write(zero_bin)

print(f"zero.bin written, {len(zero_bin)} bytes")

# -------------------- TCP binary RPC --------------------
SERVER = "118.25.42.70"
PORT = 9000

def send_frame(sock, op, body):
    header = struct.pack(">BI", op, len(body))
    sock.sendall(header + body)

def recv_frame(sock):
    header = b""
    while len(header) < 5:
        chunk = sock.recv(5 - len(header))
        if not chunk:
            raise ConnectionError("Connection closed")
        header += chunk
    op, length = struct.unpack(">BI", header)
    body = b""
    while len(body) < length:
        chunk = sock.recv(min(4096, length - len(body)))
        if not chunk:
            raise ConnectionError("Connection closed")
        body += chunk
    return op, body

def upload(sock, data):
    # OP_UPLOAD = 2
    send_frame(sock, 2, data)
    status, body = recv_frame(sock)
    if status != 0:
        raise Exception(f"Upload failed, status={status}")
    if len(body) != 32:
        raise Exception(f"Upload returned {len(body)} bytes, want 32")
    return body

def edge(sock, parent, child):
    send_frame(sock, 4, parent + child)
    status, _ = recv_frame(sock)
    if status != 0:
        raise Exception(f"Edge failed, status={status}")

def vote(sock, user, parent, child):
    send_frame(sock, 6, user + parent + child)
    status, _ = recv_frame(sock)
    if status != 0:
        raise Exception(f"Vote failed, status={status}")

def register(sock, token_bytes):
    send_frame(sock, 1, token_bytes)
    status, body = recv_frame(sock)
    if status != 0:
        raise Exception(f"Register failed, status={status}")
    if len(body) != 32:
        raise Exception(f"Register returned {len(body)} bytes, want 32")
    return body

def main():
    if not os.path.exists("zero.bin"):
        print("Error: zero.bin not found. Build section must have failed.")
        sys.exit(1)

    print(f"Connecting to {SERVER}:{PORT}...")
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.settimeout(10)
    try:
        sock.connect((SERVER, PORT))
        print("Connected")
    except Exception as e:
        print(f"Cannot connect: {e}")
        sys.exit(1)

    try:
        with open("zero.bin", "rb") as f:
            data = f.read()

        h = upload(sock, data)
        hash_hex = h.hex()
        print(f"Uploaded zero, hash = {hash_hex}")

        zero = b'\x00' * 32
        edge(sock, zero, h)
        print("Edge ZERO -> zero created")

        token = f"temp-{random.randint(0, 2**32)}".encode()
        user = register(sock, token)
        user_hex = user.hex()
        print(f"Temp user registered: {user_hex}")

        vote(sock, user, zero, h)
        print("Vote made")

    except Exception as e:
        print(f"Error: {e}")
        sys.exit(1)
    finally:
        sock.close()

    print("Done. Start cvm.exe to see zero.")

if __name__ == "__main__":
    main()
