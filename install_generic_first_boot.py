import hashlib
import json
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


def load_manifest():
    path = ROOT / "atomic_first_boot_manifest.json"
    manifest = json.loads(path.read_text(encoding="ascii"))
    required = {"program_key", "native", "actions", "first_hash", "program_hash"}
    if not required <= manifest.keys():
        raise RuntimeError("atomic first-boot manifest is incomplete")
    return manifest


def require_atomic_mods(manifest, blocks):
    forbidden = (b"ui_state", b"ui_reset", b"editor_state_init", b"UI_MAX_VIEWS")
    allowed = {bytes.fromhex(value): name for name, value in manifest["native"].items()}
    logical = {bytes.fromhex(manifest["program_key"])}
    logical.update(bytes.fromhex(value["key"]) for value in manifest["actions"].values())
    logical.update(bytes.fromhex(value["key"]) for value in manifest.get("modules", {}).values())
    bootstrap_tokens = {token for token, _ in instructions(blocks["first_bootstrap_block.bin"])}
    for block_name, raw in blocks.items():
        for token, _ in instructions(raw):
            if token in logical:
                continue
            name = allowed.get(token)
            if name is None and token in bootstrap_tokens and block_name == "first_bootstrap_block.bin":
                name = "bootstrap"
            if name is None:
                raise RuntimeError(f"{block_name} uses undeclared native token {token.hex()}")
            dll = ROOT / "mods" / f"{token.hex()}.dll"
            if not dll.exists():
                raise RuntimeError(f"missing hash-named DLL for {name}")
            data = dll.read_bytes()
            if hashlib.sha256(data).digest() != token:
                raise RuntimeError(f"DLL filename hash mismatch for {name}")
            if any(marker in data for marker in forbidden):
                raise RuntimeError(f"integrated editor mod forbidden: {name}")


def main():
    identity = load_verified_identity()
    manifest = load_manifest()
    first = (ROOT / "first_block.bin").read_bytes()
    program = (ROOT / "first_program_block.bin").read_bytes()
    bootstrap = (ROOT / "first_bootstrap_block.bin").read_bytes()
    actions = {name: (ROOT / "atomic_action_blocks" / f"{name}.bin").read_bytes()
               for name in manifest["actions"]}
    modules = {name: (ROOT / "atomic_module_blocks" / f"{name}.bin").read_bytes()
               for name in manifest.get("modules", {})}
    surfaces = {}
    for name, meta in manifest.get("surfaces", {}).items():
        raw = (ROOT / "atomic_surface_blocks" / f"{name}.bin").read_bytes()
        if hashlib.sha256(raw).hexdigest() != meta["hash"]:
            raise RuntimeError(f"surface hash mismatch for {name}")
        surfaces[name] = raw
    blocks = {"first_block.bin": first, "first_program_block.bin": program,
              "first_bootstrap_block.bin": bootstrap}
    blocks.update({f"atomic_action_blocks/{name}.bin": raw for name, raw in actions.items()})
    blocks.update({f"atomic_module_blocks/{name}.bin": raw for name, raw in modules.items()})
    blocks.update({f"atomic_surface_blocks/{name}.bin": raw for name, raw in surfaces.items()})
    require_atomic_mods(manifest, blocks)

    if hashlib.sha256(first).hexdigest() != manifest["first_hash"]:
        raise RuntimeError("first block differs from manifest")
    if hashlib.sha256(program).hexdigest() != manifest["program_hash"]:
        raise RuntimeError("program block differs from manifest")
    bootstrap_ins = instructions(bootstrap)
    if len(bootstrap_ins) != 1 or bootstrap_ins[0][1]:
        raise RuntimeError("first_bootstrap_block.bin must contain one payload-free bootstrap mod")
    first_ins = instructions(first)
    program_key = bytes.fromhex(manifest["program_key"])
    if len(first_ins) < 2 or first_ins[-1] != (program_key, b""):
        raise RuntimeError("first block must initialize variables and execute the atomic program key")
    # first block may allocate vars (var_set_payload) and write initials (const/var_write).
    allowed_init = {
        bytes.fromhex(manifest["native"][name])
        for name in ("var_set_payload", "const_payload", "var_write_payload")
        if name in manifest["native"]
    }
    if any(token not in allowed_init for token, _ in first_ins[:-1]):
        raise RuntimeError("first block contains unexpected non-init instructions")
    program_ins = instructions(program)
    module_ins_total = sum(len(instructions(raw)) for raw in modules.values())
    if manifest.get("modules") and not manifest.get("modules_inlined", False):
        # Modular composition: thin orchestrator + logical (non-DLL) module blocks.
        # Prefer bare module tokens in the stream (no exec); accept either form.
        if len(program_ins) < 8:
            raise RuntimeError("modular frame orchestrator is unexpectedly collapsed")
        if module_ins_total < 20:
            raise RuntimeError("modular frame modules are unexpectedly collapsed")
        module_keys = {bytes.fromhex(v["key"]) for v in manifest["modules"].values()}
        bare = sum(1 for tok, _ in program_ins if tok in module_keys)
        exec_tok = bytes.fromhex(manifest["native"]["exec"]) if "exec" in manifest.get("native", {}) else None
        has_exec = exec_tok is not None and any(tok == exec_tok for tok, _ in program_ins)
        if bare < 1 and not has_exec:
            raise RuntimeError("modular frame must reference logical module tokens")
    elif len(program_ins) < 40:
        raise RuntimeError("atomic frame program is unexpectedly collapsed")

    bootstrap_token = bootstrap_ins[0][0]
    with socket.create_connection(SERVER, timeout=10) as sock:
        for name, raw in actions.items():
            action_hash = upload(sock, raw)
            expected = manifest["actions"][name]
            if action_hash.hex() != expected["hash"]:
                raise RuntimeError(f"action hash mismatch for {name}")
            action_key = bytes.fromhex(expected["key"])
            add_edge(sock, action_key, action_hash)
            set_override(sock, identity, action_key, action_hash)

        for name, raw in modules.items():
            module_hash = upload(sock, raw)
            expected = manifest["modules"][name]
            if module_hash.hex() != expected["hash"]:
                raise RuntimeError(f"module hash mismatch for {name}")
            module_key = bytes.fromhex(expected["key"])
            add_edge(sock, module_key, module_hash)
            set_override(sock, identity, module_key, module_hash)

        # Specialized native surfaces: override native token -> definition block.
        # find(native) still runs the DLL; editor resolve uses override for content.
        for name, raw in surfaces.items():
            surface_hash = upload(sock, raw)
            expected = manifest["surfaces"][name]
            if surface_hash.hex() != expected["hash"]:
                raise RuntimeError(f"surface hash mismatch for {name}")
            native_token = bytes.fromhex(expected["token"])
            add_edge(sock, native_token, surface_hash)
            set_override(sock, identity, native_token, surface_hash)
            invalidate_children_cache(native_token)

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
        atomic_tag = upload(sock, b"#atomic")
        add_edge(sock, tag, atomic_tag)
        for name, value in manifest["native"].items():
            token = bytes.fromhex(value)
            dll = (ROOT / "mods" / f"{value}.dll").read_bytes()
            if upload(sock, dll) != token:
                raise RuntimeError(f"uploaded token mismatch for {name}")
            name_hash = upload(sock, name.encode("ascii"))
            add_edge(sock, atomic_tag, token)
            add_edge(sock, token, name_hash)

    invalidate_children_cache(tag)
    invalidate_children_cache(atomic_tag)
    for value in manifest["native"].values():
        invalidate_children_cache(bytes.fromhex(value))
    print("installed atomic first boot")
    print("bootstrap token:", bootstrap_token.hex())
    print("first block:    ", first_hash.hex())
    print("program key:    ", program_key.hex())
    print("program block:  ", program_hash.hex())
    for name, value in manifest["actions"].items():
        print(f"action {name:6}:", value["hash"])
    for name, value in manifest.get("modules", {}).items():
        print(f"module {name:6}:", value["hash"])
    for name, value in manifest.get("surfaces", {}).items():
        print(f"surface {name}:", value["hash"], "facets", len(value.get("facets", [])))


if __name__ == "__main__":
    main()
