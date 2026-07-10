import hashlib
import json
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parent
PROGRAM_KEY = hashlib.sha256(b"#SingularityAtomicProgram").digest()
INPUT_VAR = hashlib.sha256(b"atomic.editor.input").digest()
CAM_X = hashlib.sha256(b"atomic.cam.x").digest()
CAM_Y = hashlib.sha256(b"atomic.cam.y").digest()
CAM_Z = hashlib.sha256(b"atomic.cam.z").digest()
LAST_MX = hashlib.sha256(b"atomic.cam.last_mx").digest()
LAST_MY = hashlib.sha256(b"atomic.cam.last_my").digest()
VIEWS = hashlib.sha256(b"atomic.views.table").digest()

ACTION_CORE = ("down", "up", "delete", "insert", "backspace", "clear", "save")


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
    return struct.pack("<I", value & 0xFFFFFFFF)


def i32(value):
    return struct.pack("<i", int(value))


def f32(value):
    return struct.pack("<f", float(value))


def static_text(x, y, color, size, text):
    return struct.pack("<iiIf", x, y, color, size) + text


def stack_text(x, y, color, size, count=8):
    return struct.pack("<iiIfI", x, y, color, size, count)


def var_text(var_id, x, y, color, size):
    return var_id + struct.pack("<iiIf", x, y, color, size)


def alloc_var(t, var_id, size):
    return (t["var_set_payload"], var_id + u32(size))


def views_op(t, op, args=b""):
    return (t["views"], VIEWS + u32(op) + args)


def main():
    t = load_tokens()
    required = {
        "frame_begin", "frame_clear", "frame_end", "reexec", "camera_set_stack", "drawtext",
        "drawtext_stack", "drawtext_var", "const_payload", "var_set_payload",
        "var_read_payload", "var_write_payload", "add", "and", "key_down", "key_pressed",
        "cond", "registry_find", "registry_token_name", "text_input",
        "string_append_var", "string_backspace_var", "string_clear_var", "block_insert_stack",
        "block_delete", "block_flush", "jump_payload", "mouse_x", "mouse_y", "mouse_wheel",
        "mouse_button_down", "mouse_button_pressed", "i32_to_f32", "f32_add", "f32_sub",
        "f32_mul", "f32_div", "f32_const", "f32_clamp", "world_mouse", "drop_u32",
        "views", "views_render", "block_select_stack", "block_offset_at_index",
    }
    missing = sorted(required - t.keys())
    if missing:
        raise RuntimeError(f"missing tokens: {', '.join(missing)}")

    action_keys = {name: hashlib.sha256(("#atomic.action." + name).encode()).digest()
                   for name in ACTION_CORE}
    for name in ("pan", "click", "rmb", "drag", "end_drag"):
        action_keys[name] = hashlib.sha256(("#atomic.action." + name).encode()).digest()

    hit_args = f32(32.0) + f32(520.0) + f32(24.0) + u32(256)

    pan = make_block([
        (t["var_read_payload"], CAM_X),
        (t["mouse_x"], b""), (t["i32_to_f32"], b""),
        (t["var_read_payload"], LAST_MX), (t["f32_sub"], b""),
        (t["var_read_payload"], CAM_Z), (t["f32_div"], b""),
        (t["f32_sub"], b""),
        (t["var_write_payload"], CAM_X),
        (t["var_read_payload"], CAM_Y),
        (t["mouse_y"], b""), (t["i32_to_f32"], b""),
        (t["var_read_payload"], LAST_MY), (t["f32_sub"], b""),
        (t["var_read_payload"], CAM_Z), (t["f32_div"], b""),
        (t["f32_sub"], b""),
        (t["var_write_payload"], CAM_Y),
    ])

    drag = make_block([
        (t["mouse_x"], b""), (t["i32_to_f32"], b""),
        (t["var_read_payload"], LAST_MX), (t["f32_sub"], b""),
        (t["var_read_payload"], CAM_Z), (t["f32_div"], b""),
        (t["mouse_y"], b""), (t["i32_to_f32"], b""),
        (t["var_read_payload"], LAST_MY), (t["f32_sub"], b""),
        (t["var_read_payload"], CAM_Z), (t["f32_div"], b""),
        views_op(t, 12),  # stack dx,dy
    ])

    end_drag = make_block([views_op(t, 13)])

    click = make_block([
        (t["world_mouse"], b""),
        views_op(t, 29, hit_args),  # handled
        (t["drop_u32"], b""),
    ])

    rmb = make_block([
        (t["world_mouse"], b""),
        views_op(t, 30, hit_args),
        (t["drop_u32"], b""),
    ])

    def select_active_block():
        return [
            views_op(t, 22),  # active key[32]
            (t["block_select_stack"], b""),
        ]

    def active_cursor_to_offset():
        return [
            views_op(t, 21),  # cursor
            (t["block_offset_at_index"], b""),
        ]

    actions = {
        "down": make_block([
            views_op(t, 23, i32(1)),
        ]),
        "up": make_block([
            views_op(t, 24),
        ]),
        "delete": make_block(
            select_active_block() + active_cursor_to_offset() + [
                (t["block_delete"], b""),
                (t["jump_payload"], PROGRAM_KEY),
            ]
        ),
        "insert": make_block(
            select_active_block() + active_cursor_to_offset() + [
                (t["var_read_payload"], INPUT_VAR),
                (t["registry_find"], b""),
                (t["block_insert_stack"], b""),
                (t["string_clear_var"], INPUT_VAR),
                (t["jump_payload"], PROGRAM_KEY),
            ]
        ),
        "backspace": make_block([(t["string_backspace_var"], INPUT_VAR)]),
        "clear": make_block([(t["string_clear_var"], INPUT_VAR)]),
        "save": make_block(
            select_active_block() + [
                (t["block_flush"], b""),
            ]
        ),
        "pan": pan,
        "click": click,
        "rmb": rmb,
        "drag": drag,
        "end_drag": end_drag,
    }

    if "not" not in t:
        raise RuntimeError("missing not")

    program = [
        (t["frame_begin"], b""),
        # seed view0 once (no-op after first time)
        views_op(t, 18, PROGRAM_KEY + f32(40.0) + f32(70.0)),
        (t["var_read_payload"], CAM_X),
        (t["var_read_payload"], CAM_Y),
        (t["var_read_payload"], CAM_Z),
        (t["camera_set_stack"], b""),
        (t["frame_clear"], u32(0xff11161b)),

        # MMB camera pan (uses last_* from previous frame)
        (t["const_payload"], action_keys["pan"]),
        (t["mouse_button_down"], u32(4)),
        (t["cond"], b""),

        # RMB view drag while held
        (t["const_payload"], action_keys["drag"]),
        (t["mouse_button_down"], u32(2)),
        (t["cond"], b""),
        # clear drag when RMB up
        (t["const_payload"], action_keys["end_drag"]),
        (t["mouse_button_down"], u32(2)),
        (t["not"], b""),
        (t["cond"], b""),

        # zoom
        (t["var_read_payload"], CAM_Z),
        (t["mouse_wheel"], b""), (t["i32_to_f32"], b""),
        (t["f32_const"], f32(0.1)), (t["f32_mul"], b""),
        (t["f32_const"], f32(1.0)), (t["f32_add"], b""),
        (t["f32_mul"], b""),
        (t["f32_clamp"], f32(0.15) + f32(6.0)),
        (t["var_write_payload"], CAM_Z),

        (t["var_read_payload"], CAM_X),
        (t["var_read_payload"], CAM_Y),
        (t["var_read_payload"], CAM_Z),
        (t["camera_set_stack"], b""),

        # pointer edges
        (t["const_payload"], action_keys["click"]),
        (t["mouse_button_pressed"], u32(1)),
        (t["cond"], b""),
        (t["const_payload"], action_keys["rmb"]),
        (t["mouse_button_pressed"], u32(2)),
        (t["cond"], b""),

        # sample mouse after movement handlers
        (t["mouse_x"], b""), (t["i32_to_f32"], b""), (t["var_write_payload"], LAST_MX),
        (t["mouse_y"], b""), (t["i32_to_f32"], b""), (t["var_write_payload"], LAST_MY),

        # draw all views in world space
        (t["views_render"], VIEWS),

        # HUD screen space
        (t["f32_const"], f32(640.0)),
        (t["f32_const"], f32(360.0)),
        (t["f32_const"], f32(1.0)),
        (t["camera_set_stack"], b""),
        (t["drawtext"], static_text(20, 16, 0xff9da7b3, 16.0,
            b"multi-view  |  wheel zoom  MMB pan  LMB select  RMB open/drag  Ctrl+S save")),
        (t["drawtext"], static_text(20, 48, 0xff73808c, 14.0, b"views active/count (status via table)")),
        (t["text_input"], b""), (t["string_append_var"], INPUT_VAR),
        (t["drawtext_var"], var_text(INPUT_VAR, 20, 668, 0xffffffff, 17.0)),
        (t["var_read_payload"], INPUT_VAR), (t["registry_find"], b""),
        (t["registry_token_name"], b""),
        (t["drawtext_stack"], stack_text(210, 668, 0xff73808c, 17.0, 96)),

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
    ]

    # Init: vars + seed view0 with program key
    first_items = [
        alloc_var(t, INPUT_VAR, 256),
        alloc_var(t, CAM_X, 4),
        alloc_var(t, CAM_Y, 4),
        alloc_var(t, CAM_Z, 4),
        alloc_var(t, LAST_MX, 4),
        alloc_var(t, LAST_MY, 4),
        alloc_var(t, VIEWS, 2320),
        (t["const_payload"], f32(640.0)), (t["var_write_payload"], CAM_X),
        (t["const_payload"], f32(360.0)), (t["var_write_payload"], CAM_Y),
        (t["const_payload"], f32(1.0)), (t["var_write_payload"], CAM_Z),
        (t["const_payload"], f32(0.0)), (t["var_write_payload"], LAST_MX),
        (t["const_payload"], f32(0.0)), (t["var_write_payload"], LAST_MY),
        (PROGRAM_KEY, b""),
    ]
    first = make_block(first_items)
    program_raw = make_block(program)
    (ROOT / "first_block.bin").write_bytes(first)
    (ROOT / "first_program_block.bin").write_bytes(program_raw)

    action_dir = ROOT / "atomic_action_blocks"
    action_dir.mkdir(exist_ok=True)
    for old in action_dir.glob("*.bin"):
        old.unlink()

    used = set()
    logical = {PROGRAM_KEY}
    logical.update(action_keys.values())

    def collect(raw):
        off = 0
        while off + 32 <= len(raw):
            tok = raw[off:off + 32]
            if tok == bytes(32):
                break
            size = struct.unpack_from("<I", raw, off + 32)[0]
            used.add(tok)
            off += 36 + size

    collect(first)
    collect(program_raw)
    for name, raw in actions.items():
        path = action_dir / f"{name}.bin"
        path.write_bytes(raw)
        collect(raw)

    name_by_token = {token: name for name, token in t.items()}
    manifest = {"program_key": PROGRAM_KEY.hex(), "native": {}, "actions": {}}
    for tok in used:
        if tok in logical:
            continue
        name = name_by_token.get(tok)
        if name is None:
            raise RuntimeError(f"unknown native token {tok.hex()}")
        manifest["native"][name] = tok.hex()
    for name, raw in actions.items():
        manifest["actions"][name] = {
            "key": action_keys[name].hex(),
            "hash": hashlib.sha256(raw).hexdigest(),
        }
    manifest["first_hash"] = hashlib.sha256(first).hexdigest()
    manifest["program_hash"] = hashlib.sha256(program_raw).hexdigest()
    (ROOT / "atomic_first_boot_manifest.json").write_text(
        json.dumps(manifest, indent=2) + "\n", encoding="ascii")
    print("first", manifest["first_hash"], len(first))
    print("program", manifest["program_hash"], len(program_raw), len(program), "instructions")
    for name, value in manifest["actions"].items():
        print(name, value["hash"][:16])
    print("natives", len(manifest["native"]))


if __name__ == "__main__":
    main()
