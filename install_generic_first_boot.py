import hashlib
import socket
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parent
SERVER = ("118.25.42.70", 9000)

OP_UPLOAD = 2
OP_FILE = 3
OP_EDGE = 4
OP_CHILDREN = 5
OP_VOTE = 6
OK = 0


def request(sock, op, body=b""):
    sock.sendall(bytes((op,)) + len(body).to_bytes(4, "big") + body)
    header = recv_exact(sock, 5)
    status = header[0]
    size = int.from_bytes(header[1:5], "big")
    return status, recv_exact(sock, size)


def recv_exact(sock, size):
    out = bytearray()
    while len(out) < size:
        part = sock.recv(size - len(out))
        if not part:
            raise ConnectionError("server closed the connection")
        out += part
    return bytes(out)


def upload(sock, raw):
    status, body = request(sock, OP_UPLOAD, raw)
    if status != OK or len(body) != 32:
        raise RuntimeError(f"upload failed: status={status}, bytes={len(body)}")
    expected = hashlib.sha256(raw).digest()
    if body != expected:
        raise RuntimeError("server returned an unexpected content hash")
    return body


def add_edge(sock, parent, child):
    status, _ = request(sock, OP_EDGE, parent + child)
    if status != OK:
        raise RuntimeError(f"edge failed: status={status}")


def vote(sock, identity, parent, child):
    status, _ = request(sock, OP_VOTE, identity + parent + child)
    if status != OK:
        raise RuntimeError(f"vote failed: status={status}")


def set_override(sock, identity, key, value):
    status, _ = request(sock, 7, identity + key + value)
    if status != OK:
        raise RuntimeError(f"USET failed: status={status}")


def identity_is_registered(identity):
    # UGET with an arbitrary key returns ERR_NF (3) for a registered identity
    # with no value, and ERR_DENY (2) for an unknown identity.
    with socket.create_connection(SERVER, timeout=10) as sock:
        status, _ = request(sock, 8, identity + bytes(32))
    return status != 2


def load_verified_identity():
    id_path = ROOT / "id.bin"
    identity = id_path.read_bytes()
    if len(identity) != 32:
        raise RuntimeError("id.bin must contain exactly one 32-byte identity")
    if identity_is_registered(identity):
        return identity

    # A prior failed USET repair left this project with a preserved identity.
    # Only restore it after the live server confirms the current file is denied
    # and the backup is registered; never guess from the filename alone.
    candidates = sorted(ROOT.glob("id.bin.bak*"))
    for candidate in candidates:
        saved = candidate.read_bytes()
        if len(saved) == 32 and identity_is_registered(saved):
            rejected = ROOT / ("id.bin.bak_unregistered_" + identity.hex()[:8])
            if not rejected.exists():
                rejected.write_bytes(identity)
            id_path.write_bytes(saved)
            print("restored server-registered identity from", candidate.name)
            return saved
    raise RuntimeError("id.bin is denied by the live server and no registered backup was found")


def instructions(raw):
    off = 0
    result = []
    while off + 32 <= len(raw):
        token = raw[off:off + 32]
        if token == bytes(32):
            if off + 32 != len(raw):
                raise ValueError("data follows the zero-token terminator")
            return result
        if off + 36 > len(raw):
            raise ValueError("truncated instruction header")
        payload_size = struct.unpack_from("<I", raw, off + 32)[0]
        end = off + 36 + payload_size
        if end > len(raw):
            raise ValueError("truncated instruction payload")
        result.append((token, raw[off + 36:end]))
        off = end
    raise ValueError("block has no zero-token terminator")


def invalidate_children_cache(token):
    cache = ROOT / "cache" / f"{token.hex()}_ch.bin"
    cache.unlink(missing_ok=True)


def require_native_mods(blocks):
    """Every directly used native token must be a normal hash-named DLL.

    Logical block keys are allowed to have no DLL.  The removed integrated
    editor exported editor_state_init; no first-boot block may reference it.
    """
    for block_name, raw in blocks.items():
        for token, _ in instructions(raw):
            dll = ROOT / "mods" / f"{token.hex()}.dll"
            if not dll.exists():
                continue  # logical block key, resolved through the graph
            data = dll.read_bytes()
            if b"editor_state_init" in data:
                raise RuntimeError(
                    f"{block_name} references removed integrated editor mod {token.hex()}"
                )


def main():
    identity = load_verified_identity()
    first = (ROOT / "first_block.bin").read_bytes()
    program = (ROOT / "first_program_block.bin").read_bytes()
    bootstrap = (ROOT / "first_bootstrap_block.bin").read_bytes()

    blocks = {
        "first_block.bin": first,
        "first_program_block.bin": program,
        "first_bootstrap_block.bin": bootstrap,
    }
    require_native_mods(blocks)
    first_ins = instructions(first)
    program_ins = instructions(program)
    bootstrap_ins = instructions(bootstrap)
    if len(bootstrap_ins) != 1 or bootstrap_ins[0][1]:
        raise RuntimeError("first_bootstrap_block.bin must contain one payload-free bootstrap mod")
    if len(first_ins) != 3 or len(first_ins[0][1]) != 32:
        raise RuntimeError("first block must initialize and execute one logical program key")
    program_key = first_ins[0][1]
    if first_ins[-1] != (program_key, b""):
        raise RuntimeError("first block does not execute its initialized program key")
    if len(program_ins) < 6:
        raise RuntimeError("modular first-run program is incomplete")

    bootstrap_token = bootstrap_ins[0][0]
    module_tokens = {
        "ui_init": first_ins[0][0],
        "ui_registry": first_ins[1][0],
        "frame_begin": program_ins[0][0],
        "frame_clear": program_ins[1][0],
        "ui_input": program_ins[2][0],
        "ui_edit": program_ins[3][0],
        "ui_render": program_ins[4][0],
        "frame_end": program_ins[5][0],
        "reexec": program_ins[6][0],
    }
    for name, token in module_tokens.items():
        if not (ROOT / "mods" / f"{token.hex()}.dll").exists():
            raise RuntimeError(f"missing hash-named DLL for {name}")

    with socket.create_connection(SERVER, timeout=10) as sock:
        program_hash = upload(sock, program)
        first_hash = upload(sock, first)
        add_edge(sock, program_key, program_hash)
        set_override(sock, identity, program_key, program_hash)
        status, installed_program = request(sock, 8, identity + program_key)
        if status != OK or installed_program != program_hash:
            raise RuntimeError("program override verification failed")
        add_edge(sock, first_hash, first_hash)
        set_override(sock, identity, first_hash, first_hash)
        add_edge(sock, bootstrap_token, first_hash)
        vote(sock, identity, bootstrap_token, first_hash)

        tag = hashlib.sha256(b"#TAG").digest()
        ui_tag = upload(sock, b"#ui")
        add_edge(sock, tag, ui_tag)
        for name, token in module_tokens.items():
            dll = (ROOT / "mods" / f"{token.hex()}.dll").read_bytes()
            if upload(sock, dll) != token:
                raise RuntimeError(f"uploaded token mismatch for {name}")
            name_hash = upload(sock, name.encode("ascii"))
            add_edge(sock, ui_tag, token)
            add_edge(sock, token, name_hash)

    invalidate_children_cache(tag)
    invalidate_children_cache(ui_tag)
    for token in module_tokens.values():
        invalidate_children_cache(token)

    print("installed modular first boot")
    print("bootstrap token:", bootstrap_token.hex())
    print("first block:    ", first_hash.hex())
    print("program key:    ", program_key.hex())
    print("program block:  ", program_hash.hex())


if __name__ == "__main__":
    main()
