import hashlib
import json
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parent
PROGRAM_KEY = hashlib.sha256(b"#SingularityAtomicProgram").digest()
CURSOR = hashlib.sha256(b"atomic.cursor.index").digest()
ACTION_NAMES = ("down", "up", "delete", "insert", "backspace", "clear", "save")


def load_tokens():
    result = {}
    raw = (ROOT / "name_cache.bin").read_bytes()
    count = struct.unpack_from("<I", raw)[0]
    if len(raw) != 4 + count * 128:
        raise RuntimeError("invalid name_cache.bin")
    for index in range(count):
        record = raw[4 + index * 128:4 + (index + 1) * 128]
        name = record[32:].split(b"\0", 1)[0].decode("ascii", "ignore")
        if name and name not in result and (ROOT / "mods" / f"{record[:32].hex()}.dll").exists():
            result[name] = record[:32]
    for line in (ROOT / "atomic_mod_tokens.txt").read_text(encoding="ascii").splitlines():
        name, value = line.split("=", 1)
        result[name] = bytes.fromhex(value)
    records = []
    for name, token in sorted(result.items()):
        encoded = name.encode("ascii")[:95]
        records.append(token + encoded + bytes(96 - len(encoded)))
    (ROOT / "instruction_names.bin").write_bytes(struct.pack("<I", len(records)) + b"".join(records))
    return result


def instruction(token, payload=b""):
    return token + struct.pack("<I", len(payload)) + payload


def make_block(items):
    return b"".join(instruction(*item) for item in items) + bytes(32)


def u32(value):
    return struct.pack("<I", value)


def static_text(x, y, color, size, text):
    return struct.pack("<iiIf", x, y, color, size) + text


def stack_text(x, y, color, size, count=8):
    return struct.pack("<iiIfI", x, y, color, size, count)


def var_text(var_id, x, y, color, size):
    return var_id + struct.pack("<iiIf", x, y, color, size)


def main():
    t = load_tokens()
    required = {
        "frame_begin", "frame_clear", "frame_end", "reexec", "camera_set", "drawtext",
        "drawtext_stack", "drawtext_var", "const_payload", "var_set_payload", "var_read_payload",
        "var_write_payload", "add", "and", "key_down", "key_pressed", "cond", "u32_dec_sat",
        "block_select_payload", "block_offset_at_index", "block_token_name", "block_payload_summary",
        "registry_find", "registry_token_name", "text_input", "string_append_var",
        "string_backspace_var", "string_clear_var", "block_insert_stack",
        "block_delete", "block_flush", "jump_payload",
    }
    missing = sorted(required - t.keys())
    if missing:
        raise RuntimeError(f"missing tokens: {', '.join(missing)}")

    input_var = hashlib.sha256(b"atomic.editor.input").digest()
    action_keys = {name: hashlib.sha256(("#atomic.action." + name).encode()).digest()
                   for name in ACTION_NAMES}
    actions = {
        "down": make_block([
            (t["var_read_payload"], CURSOR), (t["const_payload"], u32(1)),
            (t["add"], b""), (t["var_write_payload"], CURSOR),
        ]),
        "up": make_block([
            (t["var_read_payload"], CURSOR), (t["u32_dec_sat"], b""),
            (t["var_write_payload"], CURSOR),
        ]),
        "delete": make_block([
            (t["block_select_payload"], PROGRAM_KEY), (t["var_read_payload"], CURSOR),
            (t["block_offset_at_index"], b""), (t["block_delete"], b""),
            (t["jump_payload"], PROGRAM_KEY),
        ]),
        "insert": make_block([
            (t["block_select_payload"], PROGRAM_KEY), (t["var_read_payload"], CURSOR),
            (t["block_offset_at_index"], b""), (t["var_read_payload"], input_var),
            (t["registry_find"], b""), (t["block_insert_stack"], b""),
            (t["string_clear_var"], input_var), (t["jump_payload"], PROGRAM_KEY),
        ]),
        "backspace": make_block([(t["string_backspace_var"], input_var)]),
        "clear": make_block([(t["string_clear_var"], input_var)]),
        "save": make_block([
            (t["block_select_payload"], PROGRAM_KEY), (t["block_flush"], b""),
        ]),
    }

    program = [
        (t["frame_begin"], b""), (t["camera_set"], struct.pack("<fff", 640.0, 360.0, 1.0)),
        (t["frame_clear"], u32(0xff11161b)),
        (t["drawtext"], static_text(20, 16, 0xff9da7b3, 16.0, b"[0] self-editing program")),
        (t["drawtext"], static_text(20, 45, 0xff66717d, 14.0,
            b"Type a command, Space/Tab insert, arrows move, Delete remove, Ctrl+S save")),
        (t["text_input"], b""), (t["string_append_var"], input_var),
        (t["drawtext_var"], var_text(input_var, 20, 668, 0xffffffff, 17.0)),
        (t["var_read_payload"], input_var), (t["registry_find"], b""),
        (t["registry_token_name"], b""),
        (t["drawtext_stack"], stack_text(210, 668, 0xff73808c, 17.0, 96)),
        (t["block_select_payload"], PROGRAM_KEY),
    ]
    for row in range(22):
        program.extend([
            (t["var_read_payload"], CURSOR), (t["const_payload"], u32(row)),
            (t["add"], b""), (t["block_offset_at_index"], b""),
            (t["block_token_name"], b""),
            (t["drawtext_stack"], stack_text(48, 82 + row * 24, 0xffe8ecef, 17.0, 96)),
            (t["var_read_payload"], CURSOR), (t["const_payload"], u32(row)),
            (t["add"], b""), (t["block_offset_at_index"], b""),
            (t["block_payload_summary"], b""),
            (t["drawtext_stack"], stack_text(260, 82 + row * 24, 0xff7fb8d8, 15.0, 96)),
        ])
    program.extend([
        (t["drawtext"], static_text(20, 82, 0xff69c58f, 17.0, b">")),
        (t["const_payload"], action_keys["down"]), (t["key_pressed"], u32(0x28)), (t["cond"], b""),
        (t["const_payload"], action_keys["up"]), (t["key_pressed"], u32(0x26)), (t["cond"], b""),
        (t["const_payload"], action_keys["delete"]), (t["key_pressed"], u32(0x2e)), (t["cond"], b""),
        (t["const_payload"], action_keys["insert"]), (t["key_pressed"], u32(0x20)), (t["cond"], b""),
        (t["const_payload"], action_keys["insert"]), (t["key_pressed"], u32(0x09)), (t["cond"], b""),
        (t["const_payload"], action_keys["backspace"]), (t["key_pressed"], u32(0x08)), (t["cond"], b""),
        (t["const_payload"], action_keys["clear"]), (t["key_pressed"], u32(0x1b)), (t["cond"], b""),
        (t["const_payload"], action_keys["save"]), (t["key_down"], u32(0x11)),
        (t["key_pressed"], u32(ord("S"))), (t["and"], b""), (t["cond"], b""),
        (t["frame_end"], b""), (t["reexec"], b""),
    ])
    first = make_block([(t["var_set_payload"], CURSOR + u32(4)), (t["var_set_payload"], input_var + u32(256)), (PROGRAM_KEY, b"")])
    program_raw = make_block(program)
    (ROOT / "first_block.bin").write_bytes(first)
    (ROOT / "first_program_block.bin").write_bytes(program_raw)
    action_dir = ROOT / "atomic_action_blocks"
    action_dir.mkdir(exist_ok=True)
    manifest = {"program_key": PROGRAM_KEY.hex(), "native": {}, "actions": {}}
    for name in required:
        manifest["native"][name] = t[name].hex()
    for name, raw in actions.items():
        path = action_dir / f"{name}.bin"
        path.write_bytes(raw)
        manifest["actions"][name] = {"key": action_keys[name].hex(), "hash": hashlib.sha256(raw).hexdigest()}
    manifest["first_hash"] = hashlib.sha256(first).hexdigest()
    manifest["program_hash"] = hashlib.sha256(program_raw).hexdigest()
    (ROOT / "atomic_first_boot_manifest.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="ascii")
    print("first", manifest["first_hash"], len(first))
    print("program", manifest["program_hash"], len(program_raw), len(program), "instructions")
    for name, value in manifest["actions"].items():
        print(name, value["hash"])


if __name__ == "__main__":
    main()
