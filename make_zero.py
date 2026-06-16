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

labels = {}
pending = []
block_list = []

def byte_off(idx):
    return sum(len(block_list[i]) for i in range(idx))

def label(name):
    labels[name] = len(block_list)

def emit(op, payload=b''):
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
        emit("FLOW:JMP", u32le(0))

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

# ========================== Helper: var ops ==========================
def emit_var_set(name):
    emit_push_text(name)
    emit("TOK:MAKE")
    emit("ST:SWAP")
    emit("VAR:SET")

def emit_var_get(name):
    emit_push_text(name)
    emit("TOK:MAKE")
    emit("VAR:GET")

def emit_var_has(name):
    emit_push_text(name)
    emit("TOK:MAKE")
    emit("VAR:HAS")

# ========================== Helper: draw var value ==========================
def emit_draw_var(x, y, label_text, var_name, hex_mode=False):
    emit_const(x)
    emit_const(y)
    emit_push_text(label_text)
    emit_var_get(var_name)
    if hex_mode:
        emit("BY:HEX")
    emit("BY:CAT")
    emit("DRAW:TEXT")

# ========================== Build zero block ==========================
block_list = []

# ---- Init ----
label("START")
emit_push_text("init")
emit("TOK:MAKE")
emit("VAR:HAS")
emit_jnz("LOOP")

# First run — initialize all variables
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

emit_push_text("input_buf")
emit("TOK:MAKE")
emit_const(0)
emit("VAR:SET")

emit_push_text("token_buf")
emit("TOK:MAKE")
emit_const(0)
emit("VAR:SET")

emit_push_text("payload_buf")
emit("TOK:MAKE")
emit_const(0)
emit("VAR:SET")

emit_push_text("input_mode")
emit("TOK:MAKE")
emit_const(0)
emit("VAR:SET")

emit_push_text("status")
emit("TOK:MAKE")
emit_push_text("OK")
emit("VAR:SET")

emit_push_text("CVM zero editor")
emit("UI:OPEN")

emit_jmp("LOOP")

# ---- Main loop ----
label("LOOP")
emit("UI:POLL")

# Clear screen (dark blue background)
emit_const(0x000020)
emit("DRAW:CLEAR")

# ==== Top status bar ====
emit_const(0)
emit_const(0)
emit_push_text("parent ")
emit_var_get("parent")
emit("BY:HEX")
emit("BY:CAT")
emit("DRAW:TEXT")

emit_const(350)
emit_const(0)
emit_push_text(" mode:")
emit_var_get("mode")
emit("U32:DEC")
emit("BY:CAT")
emit("DRAW:TEXT")

emit_const(450)
emit_const(0)
emit_push_text(" ")
emit_var_get("status")
emit("BY:CAT")
emit("DRAW:TEXT")

# ---- Load children ----
emit_var_get("parent")
emit("G:CHILDS")

emit("ST:DUP")
emit("CH:COUNT")
emit_var_set("count")

# Display child count
emit_const(0)
emit_const(20)
emit_push_text("children: ")
emit_var_get("count")
emit("U32:DEC")
emit("BY:CAT")
emit("DRAW:TEXT")

emit_const(0)
emit_const(40)
emit_push_text("sel: ")
emit_var_get("sel")
emit("U32:DEC")
emit("BY:CAT")
emit("DRAW:TEXT")

# ---- Display child list (first 15) ----
# For each visible child, draw its hash
emit_push_text("count")
emit("TOK:MAKE")
emit("VAR:GET")
emit_const(0)
emit("U32:GT")
emit_jz("NO_CHILDREN")

# Loop: draw up to 15 children
emit_var_get("count")
emit_const(15)
emit("U32:GT")
emit_jz("DRAW_ALL")

label("DRAW_SOME")
emit_jmp("DRAW_ALL")  # simplified — just draw all (max 15 visible handled by DRAW_ALL)

label("NO_CHILDREN")
emit("ST:POP")  # pop children data
emit_jmp("AFTER_DISP")

label("DRAW_ALL")
# Display first child detail
emit_var_get("sel")
emit("ST:SWAP")  # swap sel with children data
emit("CH:HASH")
emit_var_set("child_hash")

emit_const(0)
emit_const(60)
emit_push_text("-> ")
emit_var_get("child_hash")
emit("BY:HEX")
emit("BY:CAT")
emit("DRAW:TEXT")

# Show child data preview (first 32 bytes as hex)
emit_var_get("sel")
emit_var_get("parent")
emit("G:CHILDS")
emit("ST:SWAP")
emit("CH:ROW")
emit("ST:POP")
emit("ST:POP")

label("AFTER_DISP")

# ---- Mode-specific help ----
emit_var_get("mode")
emit_const(0)
emit("U32:EQ")
emit_jnz("DRAW_BROWSE_HELP")

emit_var_get("mode")
emit_const(1)
emit("U32:EQ")
emit_jnz("DRAW_EDIT_HELP")

emit_var_get("mode")
emit_const(2)
emit("U32:EQ")
emit_jnz("DRAW_PUB_HELP")

emit_jmp("AFTER_HELP")

label("DRAW_BROWSE_HELP")
emit_const(0)
emit_const(170)
emit_push_text("BROWSE: UP/DOWN:sel ENTER:enter BACK:back TAB:mode |O:ov R:reload")
emit("DRAW:TEXT")
emit_jmp("AFTER_HELP")

label("DRAW_EDIT_HELP")
emit_const(0)
emit_const(170)
emit_push_text("EDIT: I:insert D:delete E:edit O:override TAB:mode BACK:back")
emit("DRAW:TEXT")
emit_jmp("AFTER_HELP")

label("DRAW_PUB_HELP")
emit_const(0)
emit_const(170)
emit_push_text("PUBLISH: P:publish S:save TAB:mode")
emit("DRAW:TEXT")

label("AFTER_HELP")

# ---- Input mode display ----
emit_var_get("input_mode")
emit_const(0)
emit("U32:EQ")
emit_jz("SHOW_INPUT_STATE")

emit_jmp("KEY_INPUT")

label("SHOW_INPUT_STATE")
emit_const(0)
emit_const(190)
emit_push_text("INPUT: ")
emit_var_get("input_buf")
emit("BY:CAT")
emit("DRAW:TEXT")

# ---- Key input ----
label("KEY_INPUT")
emit("IN:KEY")

emit("ST:DUP")
emit("BY:LEN")
emit_const(0)
emit("U32:EQ")
emit_jz("HAS_KEY")

emit("ST:POP")
emit_jmp("SLEEP")

label("HAS_KEY")

# ---- Check input mode first ----
emit_var_get("input_mode")
emit_const(0)
emit("U32:GT")
emit_jnz("HANDLE_INPUT_MODE")

# ---- Normal mode key handling ----
# Check UP
emit("ST:DUP")
emit("KEY:CODE")
emit("KEY:UP")
emit("U32:EQ")
emit_jnz("DO_UP")

# Check DOWN
emit("ST:DUP")
emit("KEY:CODE")
emit("KEY:DOWN")
emit("U32:EQ")
emit_jnz("DO_DOWN")

# Check ENTER
emit("ST:DUP")
emit("KEY:CODE")
emit("KEY:ENTER")
emit("U32:EQ")
emit_jnz("DO_ENTER")

# Check BACK (ESC)
emit("ST:DUP")
emit("KEY:CODE")
emit("KEY:BACK")
emit("U32:EQ")
emit_jnz("DO_BACK")

# Check TAB
emit("ST:DUP")
emit("KEY:CODE")
emit("KEY:TAB")
emit("U32:EQ")
emit_jnz("TOGGLE_MODE")

# Check O (override)
emit("ST:DUP")
emit("KEY:ASCII")
emit("ST:DUP")
emit_const(79)
emit("U32:EQ")
emit_jnz("DO_OV")

# Check R (reload)
emit("ST:DUP")
emit("KEY:ASCII")
emit("ST:DUP")
emit_const(82)
emit("U32:EQ")
emit_jnz("DO_RELOAD")

# Check I (insert) — only in edit mode
emit_var_get("mode")
emit_const(1)
emit("U32:EQ")
emit_jz("CHECK_D")
emit("ST:DUP")
emit("KEY:ASCII")
emit("ST:DUP")
emit_const(73)
emit("U32:EQ")
emit_jnz("DO_INSERT_START")

# Check D (delete)
label("CHECK_D")
emit_var_get("mode")
emit_const(1)
emit("U32:EQ")
emit_jz("CHECK_E")
emit("ST:DUP")
emit("KEY:ASCII")
emit("ST:DUP")
emit_const(68)
emit("U32:EQ")
emit_jnz("DO_DELETE")

# Check E (edit)
label("CHECK_E")
emit_var_get("mode")
emit_const(1)
emit("U32:EQ")
emit_jz("CHECK_P")
emit("ST:DUP")
emit("KEY:ASCII")
emit("ST:DUP")
emit_const(69)
emit("U32:EQ")
emit_jnz("DO_EDIT_START")

# Check P (publish)
label("CHECK_P")
emit_var_get("mode")
emit_const(2)
emit("U32:EQ")
emit_jz("CHECK_S")
emit("ST:DUP")
emit("KEY:ASCII")
emit("ST:DUP")
emit_const(80)
emit("U32:EQ")
emit_jnz("DO_PUBLISH")

# Check S (save to user)
label("CHECK_S")
emit_var_get("mode")
emit_const(2)
emit("U32:EQ")
emit_jz("DISCARD_KEY")
emit("ST:DUP")
emit("KEY:ASCII")
emit("ST:DUP")
emit_const(83)
emit("U32:EQ")
emit_jnz("DO_SAVE_USER")

label("DISCARD_KEY")
emit("ST:POP")
emit("ST:POP")
emit("ST:POP")
emit_jmp("SLEEP")

# ====== Input mode handling ======
label("HANDLE_INPUT_MODE")
# In input mode, ENTER confirms, ESC cancels
emit("ST:DUP")
emit("KEY:CODE")
emit("KEY:ENTER")
emit("U32:EQ")
emit_jnz("INPUT_CONFIRM")

emit("ST:DUP")
emit("KEY:CODE")
emit("KEY:BACK")
emit("U32:EQ")
emit_jnz("INPUT_CANCEL")

# BACK key deletes last char
emit("ST:DUP")
emit("KEY:CODE")
emit("KEY:DEL")
emit("U32:EQ")
emit_jnz("INPUT_DEL")

# Append ASCII character to input_buf
emit_var_get("input_buf")
emit("ST:DUP")
emit("KEY:ASCII")
emit("BY:CAT")
emit_var_set("input_buf")

emit("ST:POP")
emit("ST:POP")
emit("ST:POP")
emit_jmp("SLEEP")

label("INPUT_DEL")
emit("ST:POP")  # discard key
emit("ST:POP")  # discard key copy
emit("ST:POP")  # discard key copy
# Truncate last byte from input_buf
emit_var_get("input_buf")
emit("BY:LEN")
emit_const(0)
emit("U32:GT")
emit_jz("INPUT_DEL_DONE")
emit_var_get("input_buf")
emit("BY:LEN")
emit_const(1)
emit("U32:SUB")
emit("ST:SWAP")
emit_const(0)
emit("ST:SWAP")
emit("BY:SLICE")
emit_var_set("input_buf")
label("INPUT_DEL_DONE")
emit_jmp("SLEEP")

label("INPUT_CANCEL")
emit("ST:POP")
emit("ST:POP")
emit("ST:POP")
# Clear input buffer
emit_const(0)
emit_var_set("input_buf")
emit_const(0)
emit_var_set("input_mode")
emit_push_text("cancelled")
emit_var_set("status")
emit_jmp("SLEEP")

label("INPUT_CONFIRM")
emit("ST:POP")
emit("ST:POP")
emit("ST:POP")

# input_mode: 1=token, 2=payload, 3=hex_edit
emit_var_get("input_mode")
emit_const(1)
emit("U32:EQ")
emit_jnz("INPUT_TOKEN_DONE")

emit_var_get("input_mode")
emit_const(2)
emit("U32:EQ")
emit_jnz("INPUT_PAYLOAD_DONE")

emit_var_get("input_mode")
emit_const(3)
emit("U32:EQ")
emit_jnz("INPUT_HEX_DONE")

emit_jmp("SLEEP")

label("INPUT_TOKEN_DONE")
emit_var_get("input_buf")
emit("TOK:MAKE")
emit_var_set("token_buf")
emit_const(2)
emit_var_set("input_mode")
emit_const(0)
emit_var_set("input_buf")
emit_push_text("token ok, enter payload")
emit_var_set("status")
emit_jmp("SLEEP")

label("INPUT_PAYLOAD_DONE")
# Have token_buf and input_buf (payload). Build block with BLK:INS.
emit_var_get("parent")  # need current block data
emit("CUR:FILE")
emit_var_get("sel")
emit("BLK:END")  # get end offset = insert at end for now
emit_var_get("input_buf")  # payload data
emit_var_get("token_buf")  # token hash to use
emit("BLK:INS")
# Now override the result
emit("CUR:KEY")
emit("OV:SET")
emit_const(0)
emit_var_set("input_mode")
emit_const(0)
emit_var_set("input_buf")
emit_push_text("inserted")
emit_var_set("status")
emit_jmp("SLEEP")

label("INPUT_HEX_DONE")
# edit selected block's payload
emit_var_get("parent")
emit("CUR:FILE")
emit_var_get("sel")
emit("BLK:HASH")  # hash of selected block
emit_var_get("input_buf")
emit("ST:SWAP")
emit("ST:SWAP")
emit("BLK:SET")
emit("CUR:KEY")
emit("OV:SET")
emit_const(0)
emit_var_set("input_mode")
emit_const(0)
emit_var_set("input_buf")
emit_push_text("edited")
emit_var_set("status")
emit_jmp("SLEEP")

# ====== Handlers ======
label("DO_UP")
emit("ST:POP")  # discard key copy
emit("ST:POP")  # discard key copy
emit_var_get("sel")
emit_const(0)
emit("U32:GT")
emit_jz("SLEEP")
emit_var_get("sel")
emit_const(1)
emit("U32:SUB")
emit_var_set("sel")
emit_jmp("SLEEP")

label("DO_DOWN")
emit("ST:POP")
emit("ST:POP")
emit_var_get("sel")
emit_const(1)
emit("U32:ADD")
emit_var_get("count")
emit_const(1)
emit("U32:SUB")
emit("U32:GT")
emit_jz("SLEEP")
emit_var_get("sel")
emit_const(1)
emit("U32:ADD")
emit_var_set("sel")
emit_jmp("SLEEP")

label("DO_ENTER")
emit("ST:POP")
emit("ST:POP")
# Push current parent to prev list
emit_var_get("prev")
emit("LST:NEW")  # ensure exists
emit_var_get("prev")
emit_var_get("parent")
emit("LST:PUSH")
emit_var_set("prev")
# Enter selected child
emit_var_get("sel")
emit_var_get("parent")
emit("G:CHILDS")
emit("ST:SWAP")
emit("CH:HASH")
emit_var_set("parent")
emit_const(0)
emit_var_set("sel")
emit_jmp("SLEEP")

label("DO_BACK")
emit("ST:POP")
emit("ST:POP")
emit_var_get("prev")
emit("LST:COUNT")
emit_const(0)
emit("U32:EQ")
emit_jz("DO_BACK_OK")
emit_jmp("SLEEP")

label("DO_BACK_OK")
# Pop last from prev list
emit_var_get("prev")
emit("LST:COUNT")
emit_const(1)
emit("U32:SUB")
emit("LST:GET")
emit_var_set("parent")
emit_var_get("prev")
emit("LST:COUNT")
emit_const(1)
emit("U32:SUB")
emit("LST:DEL")
emit_var_set("prev")
emit_const(0)
emit_var_set("sel")
emit_jmp("SLEEP")

label("TOGGLE_MODE")
emit("ST:POP")
emit("ST:POP")
emit_var_get("mode")
emit_const(1)
emit("U32:ADD")
emit("ST:DUP")
emit_const(3)
emit("U32:EQ")
emit_jz("MODE_OK")
emit("ST:POP")
emit_const(0)
label("MODE_OK")
emit_var_set("mode")
emit_jmp("SLEEP")

label("DO_OV")
emit("ST:POP")
emit("ST:POP")
emit("ST:POP")
emit_var_get("parent")
emit("CUR:FILE")
emit("OV:SET")
emit_push_text("overriden")
emit_var_set("status")
emit_jmp("SLEEP")

label("DO_RELOAD")
emit("ST:POP")
emit("ST:POP")
emit("ST:POP")
emit_var_get("parent")
emit("CUR:KEY")
emit("OV:SET")
emit_push_text("reloaded")
emit_var_set("status")
emit_jmp("SLEEP")

# ====== Edit mode operations ======
label("DO_INSERT_START")
emit("ST:POP")
emit("ST:POP")
emit("ST:POP")
emit("ST:POP")
emit_const(1)
emit_var_set("input_mode")
emit_const(0)
emit_var_set("input_buf")
emit_push_text("enter token name")
emit_var_set("status")
emit_jmp("SLEEP")

label("DO_DELETE")
emit("ST:POP")
emit("ST:POP")
emit("ST:POP")
emit("ST:POP")
emit_var_get("parent")
emit("CUR:FILE")
emit_var_get("sel")
emit("BLK:END")  # offset of selected
# Actually delete the block at sel position
emit_var_get("parent")
emit("CUR:FILE")
emit_const(0)  # at offset 0 for now — need actual offset
# Simplified: recalculate block
emit_var_get("parent")
emit("CUR:FILE")
emit_var_get("sel")
emit("BLK:HASH")  # get hash of block to delete
# We can't easily delete by index with current BLK ops
# Just mark status
emit("ST:POP")
emit("ST:POP")
emit("ST:POP")
emit_push_text("del: use E to edit")
emit_var_set("status")
emit_jmp("SLEEP")

label("DO_EDIT_START")
emit("ST:POP")
emit("ST:POP")
emit("ST:POP")
emit("ST:POP")
emit_const(3)
emit_var_set("input_mode")
emit_const(0)
emit_var_set("input_buf")
emit_push_text("enter new payload")
emit_var_set("status")
emit_jmp("SLEEP")

# ====== Publish mode operations ======
label("DO_PUBLISH")
emit("ST:POP")
emit("ST:POP")
emit("ST:POP")
emit("ST:POP")
# Upload current block as new file
emit_var_get("parent")
emit("CUR:FILE")
emit("G:UPLOAD")
emit_var_set("upload_hash")
# Read id.bin for identity
emit_push_text("id.bin")
emit("FS:READ")
emit("BY:TAKE32")
emit_var_set("user_id")
# Create edge from parent to new upload
emit_var_get("parent")
emit_var_get("upload_hash")
emit("BY:CAT")
emit("G:EDGE")
# Vote
emit_var_get("user_id")
emit_var_get("parent")
emit_var_get("upload_hash")
emit("BY:CAT")
emit("BY:CAT")
emit("G:VOTE")
emit_push_text("published")
emit_var_set("status")
emit_jmp("SLEEP")

label("DO_SAVE_USER")
emit("ST:POP")
emit("ST:POP")
emit("ST:POP")
emit("ST:POP")
# Save parent hash to user space under key "zero_block"
emit_push_text("id.bin")
emit("FS:READ")
emit("BY:TAKE32")
emit_push_text("zero_block")
emit("TOK:MAKE")
emit_var_get("parent")
emit("BY:CAT")
emit("BY:CAT")
emit("G:USET")
emit_push_text("saved")
emit_var_set("status")
emit_jmp("SLEEP")

label("SLEEP")
emit_const(30)
emit("TIME:SLEEP")
emit_jmp("LOOP")

# End marker
emit("FLOW:END")

# Resolve forward labels
resolve_labels()

# ==================== Build binary ====================
zero_bin = b''.join(block_list) + b'\x00' * 32

with open("zero.bin", "wb") as f:
    f.write(zero_bin)

print(f"zero.bin written, {len(zero_bin)} bytes, {len(block_list)} blocks")

# ==================== TCP binary RPC ====================
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

        # Use existing id.bin identity (already verified)
        if os.path.exists("id.bin"):
            with open("id.bin", "rb") as f:
                user = f.read()[:32]
            user_hex = user.hex()
            print(f"Using identity from id.bin: {user_hex}")
            try:
                vote(sock, user, zero, h)
                print("Vote made")
            except Exception as e:
                print(f"Vote failed (may already exist): {e}")
        else:
            print("No id.bin found, skipping vote")

    except Exception as e:
        print(f"Error: {e}")
        sys.exit(1)
    finally:
        sock.close()

    print("Done. Start cvm.exe to see zero.")

if __name__ == "__main__":
    main()