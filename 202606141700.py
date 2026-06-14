import struct
import socket
import os
import sys
import random
import string

# ========================== Block assembler ==========================
def tok(s):
    """Convert string to 32-byte token (ASCII, zero-padded)"""
    b = s.encode('ascii', errors='ignore')[:32]
    return b.ljust(32, b'\x00')

def u32le(x):
    """uint32 little-endian"""
    return struct.pack('<I', x)

def item(op, payload=b''):
    """Build one block item: token[32] + span[u32le] + payload"""
    token = tok(op)
    span = u32le(4 + len(payload))
    return token + span + payload

def emit(block_list, op, payload=b''):
    """Append item to block list"""
    block_list.append(item(op, payload))

def emit_const(block_list, x):
    """Emit U32:CONST x"""
    emit(block_list, "U32:CONST", u32le(x))

def emit_push_text(block_list, text):
    """Emit ST:PUSH "text" """
    emit(block_list, "ST:PUSH", text.encode('utf-8'))

def emit_draw_text(block_list, x, y, text):
    """Emit draw text at (x,y)"""
    emit_const(block_list, x)
    emit_const(block_list, y)
    emit_push_text(block_list, text)
    emit(block_list, "DRAW:TEXT")

# Labels
labels = {}
pending = []

def label(name):
    """Define label at current position"""
    labels[name] = len(block_list)

def emit_jmp(block_list, target):
    """Emit FLOW:JMP target (forward label support)"""
    if target in labels:
        emit(block_list, "FLOW:JMP", u32le(labels[target]))
    else:
        pending.append({'list': block_list, 'target': target, 'type': 'JMP'})

def emit_jz(block_list, target):
    """Emit FLOW:JZ target"""
    if target in labels:
        emit(block_list, "FLOW:JZ", u32le(labels[target]))
    else:
        pending.append({'list': block_list, 'target': target, 'type': 'JZ'})

def emit_jnz(block_list, target):
    """Emit FLOW:JNZ target"""
    if target in labels:
        emit(block_list, "FLOW:JNZ", u32le(labels[target]))
    else:
        pending.append({'list': block_list, 'target': target, 'type': 'JNZ'})

def resolve_labels():
    """Patch forward labels"""
    for p in pending:
        if p['target'] not in labels:
            raise ValueError(f"Undefined label: {p['target']}")
        off = labels[p['target']]
        payload = u32le(off)
        # Replace last item in list
        if p['type'] == 'JMP':
            p['list'][-1] = item("FLOW:JMP", payload)
        elif p['type'] == 'JZ':
            p['list'][-1] = item("FLOW:JZ", payload)
        elif p['type'] == 'JNZ':
            p['list'][-1] = item("FLOW:JNZ", payload)
    pending.clear()

# ========================== Build zero block ==========================
block_list = []

# --- Init ---
label("START")
    emit_push_text(block_list, "init")
    emit(block_list, "TOK:MAKE")
    emit(block_list, "VAR:HAS")
    emit_jnz(block_list, "LOOP")

    # init = 1
    emit_push_text(block_list, "init")
    emit(block_list, "TOK:MAKE")
    emit_const(block_list, 1)
    emit(block_list, "VAR:SET")

    # parent = ZERO
    emit_push_text(block_list, "parent")
    emit(block_list, "TOK:MAKE")
    emit(block_list, "TOK:ZERO")
    emit(block_list, "VAR:SET")

    # prev = empty list
    emit_push_text(block_list, "prev")
    emit(block_list, "TOK:MAKE")
    emit(block_list, "LST:NEW")
    emit(block_list, "VAR:SET")

    # sel = 0
    emit_push_text(block_list, "sel")
    emit(block_list, "TOK:MAKE")
    emit_const(block_list, 0)
    emit(block_list, "VAR:SET")

    # mode = 0
    emit_push_text(block_list, "mode")
    emit(block_list, "TOK:MAKE")
    emit_const(block_list, 0)
    emit(block_list, "VAR:SET")

    # UI:OPEN "CVM zero"
    emit_push_text(block_list, "CVM zero")
    emit(block_list, "UI:OPEN")

    emit_jmp(block_list, "LOOP")

# --- Main loop ---
label("LOOP")
    emit(block_list, "UI:POLL")

    # clear screen
    emit_const(block_list, 0x202020)
    emit(block_list, "DRAW:CLEAR")

    # Display parent key
    emit_const(block_list, 10)
    emit_const(block_list, 10)
    emit_push_text(block_list, "parent ")
    emit_push_text(block_list, "parent")
    emit(block_list, "TOK:MAKE")
    emit(block_list, "VAR:GET")
    emit(block_list, "BY:HEX")
    emit(block_list, "BY:CAT")
    emit(block_list, "DRAW:TEXT")

    # Get children
    emit_push_text(block_list, "parent")
    emit(block_list, "TOK:MAKE")
    emit(block_list, "VAR:GET")
    emit(block_list, "G:CHILDS")
    # children data now on stack

    # children count
    emit(block_list, "ST:DUP")
    emit(block_list, "CH:COUNT")
    emit_push_text(block_list, "count")
    emit(block_list, "TOK:MAKE")
    emit(block_list, "ST:SWAP")
    emit(block_list, "VAR:SET")

    emit_const(block_list, 10)
    emit_const(block_list, 30)
    emit_push_text(block_list, "children ")
    emit_push_text(block_list, "count")
    emit(block_list, "TOK:MAKE")
    emit(block_list, "VAR:GET")
    emit(block_list, "U32:DEC")
    emit(block_list, "BY:CAT")
    emit(block_list, "DRAW:TEXT")

    # sel
    emit_const(block_list, 10)
    emit_const(block_list, 50)
    emit_push_text(block_list, "sel ")
    emit_push_text(block_list, "sel")
    emit(block_list, "TOK:MAKE")
    emit(block_list, "VAR:GET")
    emit(block_list, "U32:DEC")
    emit(block_list, "BY:CAT")
    emit(block_list, "DRAW:TEXT")

    # selected child hash if count>0
    emit_push_text(block_list, "count")
    emit(block_list, "TOK:MAKE")
    emit(block_list, "VAR:GET")
    emit_const(block_list, 0)
    emit(block_list, "U32:GT")
    emit_jz(block_list, "NO_CHILD")

    emit_push_text(block_list, "sel")
    emit(block_list, "TOK:MAKE")
    emit(block_list, "VAR:GET")
    emit(block_list, "ST:SWAP")
    emit(block_list, "CH:HASH")
    emit_push_text(block_list, "child")
    emit(block_list, "TOK:MAKE")
    emit(block_list, "ST:SWAP")
    emit(block_list, "VAR:SET")

    emit_const(block_list, 10)
    emit_const(block_list, 70)
    emit_push_text(block_list, "-> ")
    emit_push_text(block_list, "child")
    emit(block_list, "TOK:MAKE")
    emit(block_list, "VAR:GET")
    emit(block_list, "BY:HEX")
    emit(block_list, "BY:CAT")
    emit(block_list, "DRAW:TEXT")
    emit_jmp(block_list, "AFTER_DISP")

label("NO_CHILD")
    # pop children data
    emit(block_list, "ST:POP")

label("AFTER_DISP")
    # help bar
    emit_const(block_list, 10)
    emit_const(block_list, 110)
    emit_push_text(block_list, "UP/DOWN:sel ENTER:enter BACK:back TAB:editor O:ov S:save P:pub R:reload")
    emit(block_list, "DRAW:TEXT")

    # Input
    emit(block_list, "IN:KEY")

    emit(block_list, "ST:DUP")
    emit(block_list, "BY:LEN")
    emit_const(block_list, 0)
    emit(block_list, "U32:EQ")
    emit_jz(block_list, "HAS_KEY")

    # no key
    emit(block_list, "ST:POP")
    emit_jmp(block_list, "SLEEP")

label("HAS_KEY")
    # key_event on stack
    emit(block_list, "ST:DUP")
    emit(block_list, "KEY:CODE")
    emit(block_list, "KEY:UP")
    emit(block_list, "U32:EQ")
    emit_jnz(block_list, "DO_UP")

    emit(block_list, "ST:DUP")
    emit(block_list, "KEY:CODE")
    emit(block_list, "KEY:DOWN")
    emit(block_list, "U32:EQ")
    emit_jnz(block_list, "DO_DOWN")

    emit(block_list, "ST:DUP")
    emit(block_list, "KEY:CODE")
    emit(block_list, "KEY:ENTER")
    emit(block_list, "U32:EQ")
    emit_jnz(block_list, "DO_ENTER")

    emit(block_list, "ST:DUP")
    emit(block_list, "KEY:CODE")
    emit(block_list, "KEY:BACK")
    emit(block_list, "U32:EQ")
    emit_jnz(block_list, "DO_BACK")

    emit(block_list, "ST:DUP")
    emit(block_list, "KEY:CODE")
    emit(block_list, "KEY:TAB")
    emit(block_list, "U32:EQ")
    emit_jnz(block_list, "TOGGLE_MODE")

    emit(block_list, "ST:DUP")
    emit(block_list, "KEY:ASCII")
    emit(block_list, "ST:DUP")
    emit_const(block_list, 79)   # 'O'
    emit(block_list, "U32:EQ")
    emit_jnz(block_list, "DO_OV")

    emit(block_list, "ST:DUP")
    emit_const(block_list, 83)   # 'S'
    emit(block_list, "U32:EQ")
    emit_jnz(block_list, "DO_SAVE")

    emit(block_list, "ST:DUP")
    emit_const(block_list, 80)   # 'P'
    emit(block_list, "U32:EQ")
    emit_jnz(block_list, "DO_PUBLISH")

    emit(block_list, "ST:DUP")
    emit_const(block_list, 82)   # 'R'
    emit(block_list, "U32:EQ")
    emit_jnz(block_list, "DO_RELOAD")

    # unknown key, pop everything
    emit(block_list, "ST:POP")
    emit(block_list, "ST:POP")
    emit(block_list, "ST:POP")
    emit_jmp(block_list, "SLEEP")

# ====================== Handlers ======================

label("DO_UP")
    emit(block_list, "ST:POP")   # key_event
    emit_push_text(block_list, "sel")
    emit(block_list, "TOK:MAKE")
    emit(block_list, "VAR:GET")
    emit_const(block_list, 0)
    emit(block_list, "U32:GT")
    emit_jz(block_list, "SLEEP")   # already 0
    emit_push_text(block_list, "sel")
    emit(block_list, "TOK:MAKE")
    emit_push_text(block_list, "sel")
    emit(block_list, "TOK:MAKE")
    emit(block_list, "VAR:GET")
    emit_const(block_list, 1)
    emit(block_list, "U32:SUB")
    emit(block_list, "VAR:SET")
    emit_jmp(block_list, "SLEEP")

label("DO_DOWN")
    emit(block_list, "ST:POP")
    emit_push_text(block_list, "sel")
    emit(block_list, "TOK:MAKE")
    emit(block_list, "VAR:GET")
    emit_const(block_list, 1)
    emit(block_list, "U32:ADD")
    emit_push_text(block_list, "count")
    emit(block_list, "TOK:MAKE")
    emit(block_list, "VAR:GET")
    emit_const(block_list, 1)
    emit(block_list, "U32:SUB")
    emit(block_list, "U32:GT")
    emit_jz(block_list, "SLEEP")   # already max
    emit_push_text(block_list, "sel")
    emit(block_list, "TOK:MAKE")
    emit_push_text(block_list, "sel")
    emit(block_list, "TOK:MAKE")
    emit(block_list, "VAR:GET")
    emit_const(block_list, 1)
    emit(block_list, "U32:ADD")
    emit(block_list, "VAR:SET")
    emit_jmp(block_list, "SLEEP")

label("DO_ENTER")
    emit(block_list, "ST:POP")
    # push current parent to prev list
    emit_push_text(block_list, "prev")
    emit(block_list, "TOK:MAKE")
    emit_push_text(block_list, "prev")
    emit(block_list, "TOK:MAKE")
    emit(block_list, "VAR:GET")
    emit_push_text(block_list, "parent")
    emit(block_list, "TOK:MAKE")
    emit(block_list, "VAR:GET")
    emit(block_list, "LST:PUSH")
    emit(block_list, "VAR:SET")
    # parent = selected child
    emit_push_text(block_list, "parent")
    emit(block_list, "TOK:MAKE")
    emit_push_text(block_list, "child")
    emit(block_list, "TOK:MAKE")
    emit(block_list, "VAR:GET")
    emit(block_list, "VAR:SET")
    # sel = 0
    emit_push_text(block_list, "sel")
    emit(block_list, "TOK:MAKE")
    emit_const(block_list, 0)
    emit(block_list, "VAR:SET")
    emit_jmp(block_list, "SLEEP")

label("DO_BACK")
    emit(block_list, "ST:POP")
    emit_push_text(block_list, "prev")
    emit(block_list, "TOK:MAKE")
    emit(block_list, "VAR:GET")
    emit(block_list, "LST:COUNT")
    emit_const(block_list, 0)
    emit(block_list, "U32:EQ")
    emit_jz(block_list, "DO_BACK1")
    emit_jmp(block_list, "SLEEP")
label("DO_BACK1")
    # parent = prev[-1]
    emit_push_text(block_list, "prev")
    emit(block_list, "TOK:MAKE")
    emit(block_list, "VAR:GET")
    emit(block_list, "LST:COUNT")
    emit_const(block_list, 1)
    emit(block_list, "U32:SUB")
    emit(block_list, "LST:GET")
    emit_push_text(block_list, "parent")
    emit(block_list, "TOK:MAKE")
    emit(block_list, "ST:SWAP")
    emit(block_list, "VAR:SET")
    # pop prev
    emit_push_text(block_list, "prev")
    emit(block_list, "TOK:MAKE")
    emit(block_list, "VAR:GET")
    emit(block_list, "LST:COUNT")
    emit_const(block_list, 1)
    emit(block_list, "U32:SUB")
    emit(block_list, "LST:DEL")
    emit_push_text(block_list, "prev")
    emit(block_list, "TOK:MAKE")
    emit(block_list, "ST:SWAP")
    emit(block_list, "VAR:SET")
    emit_push_text(block_list, "sel")
    emit(block_list, "TOK:MAKE")
    emit_const(block_list, 0)
    emit(block_list, "VAR:SET")
    emit_jmp(block_list, "SLEEP")

label("TOGGLE_MODE")
    emit(block_list, "ST:POP")
    emit_push_text(block_list, "mode")
    emit(block_list, "TOK:MAKE")
    emit_push_text(block_list, "mode")
    emit(block_list, "TOK:MAKE")
    emit(block_list, "VAR:GET")
    emit_const(block_list, 1)
    emit(block_list, "U32:XOR")
    emit(block_list, "VAR:SET")
    emit_jmp(block_list, "SLEEP")

label("DO_OV")
    emit(block_list, "ST:POP")
    emit(block_list, "ST:POP")
    emit_push_text(block_list, "parent")
    emit(block_list, "TOK:MAKE")
    emit(block_list, "VAR:GET")
    emit(block_list, "CUR:FILE")
    emit(block_list, "OV:SET")
    emit_jmp(block_list, "SLEEP")

label("DO_SAVE")
    emit(block_list, "ST:POP")
    emit(block_list, "ST:POP")
    # TODO: needs user identity, skip for now
    emit_jmp(block_list, "SLEEP")

label("DO_PUBLISH")
    emit(block_list, "ST:POP")
    emit(block_list, "ST:POP")
    # TODO: needs user identity, skip for now
    emit_jmp(block_list, "SLEEP")

label("DO_RELOAD")
    emit(block_list, "ST:POP")
    emit(block_list, "ST:POP")
    # clear override then force refresh
    emit_push_text(block_list, "parent")
    emit(block_list, "TOK:MAKE")
    emit(block_list, "VAR:GET")
    emit(block_list, "CUR:KEY")
    emit(block_list, "OV:SET")
    emit_jmp(block_list, "SLEEP")

label("SLEEP")
    emit_const(block_list, 16)
    emit(block_list, "TIME:SLEEP")
    emit_jmp(block_list, "LOOP")

# final end marker
emit(block_list, "FLOW:END")

# Resolve forward labels
resolve_labels()

# ====================== Build binary ======================
zero_bin = b''.join(block_list) + b'\x00' * 32  # end marker

with open("zero.bin", "wb") as f:
    f.write(zero_bin)

print(f"zero.bin written, {len(zero_bin)} bytes")

# ====================== Upload via TCP binary RPC ======================
SERVER = "118.25.42.70"
PORT = 9000

def send_frame(sock, op, body):
    # frame: op(u8) + len(u32be) + body
    header = struct.pack(">BI", op, len(body))
    sock.sendall(header + body)

def recv_frame(sock):
    # recv op + len
    header = b""
    while len(header) < 5:
        chunk = sock.recv(5 - len(header))
        if not chunk:
            raise ConnectionError("Connection closed")
        header += chunk
    op, length = struct.unpack(">BI", header)
    # recv body
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
    # OP_EDGE = 4
    send_frame(sock, 4, parent + child)
    status, body = recv_frame(sock)
    if status != 0:
        raise Exception(f"Edge failed, status={status}")

def vote(sock, user, parent, child):
    # OP_VOTE = 6
    send_frame(sock, 6, user + parent + child)
    status, body = recv_frame(sock)
    if status != 0:
        raise Exception(f"Vote failed, status={status}")

def register(sock, token_bytes):
    # OP_REGISTER = 1
    send_frame(sock, 1, token_bytes)
    status, body = recv_frame(sock)
    if status != 0:
        raise Exception(f"Register failed, status={status}")
    if len(body) != 32:
        raise Exception(f"Register returned {len(body)} bytes, want 32")
    return body

def main():
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
        # upload
        h = upload(sock, zero_bin)
        hash_hex = h.hex()
        print(f"Uploaded zero, hash = {hash_hex}")

        # edge ZERO -> zero
        zero = b'\x00' * 32
        edge(sock, zero, h)
        print("Edge ZERO -> zero created")

        # register a temporary user
        token = f"temp-{random.randint(0, 2**32)}".encode()
        user = register(sock, token)
        user_hex = user.hex()
        print(f"Temp user registered: {user_hex}")

        # vote
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
