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
GRAB_MX = hashlib.sha256(b"atomic.cam.grab_mx").digest()
GRAB_MY = hashlib.sha256(b"atomic.cam.grab_my").digest()
GRAB_CX = hashlib.sha256(b"atomic.cam.grab_cx").digest()
GRAB_CY = hashlib.sha256(b"atomic.cam.grab_cy").digest()
GRAB_VX = hashlib.sha256(b"atomic.cam.grab_vx").digest()
GRAB_VY = hashlib.sha256(b"atomic.cam.grab_vy").digest()
PAN_ACTIVE = hashlib.sha256(b"atomic.cam.pan_active").digest()
DRAG_ANCHORED = hashlib.sha256(b"atomic.cam.drag_anchored").digest()
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


def u32(v):
    return struct.pack("<I", v & 0xFFFFFFFF)


def i32(v):
    return struct.pack("<i", int(v))


def f32(v):
    return struct.pack("<f", float(v))


def static_text(x, y, color, size, text):
    return struct.pack("<iiIf", x, y, color, size) + text


def alloc_var(t, var_id, size):
    return (t["var_set_payload"], var_id + u32(size))


def views_op(t, op, args=b""):
    return (t["views"], VIEWS + u32(op) + args)


def cond_action(t, key):
    return (t["cond_payload"], key)


def main():
    t = load_tokens()
    required = {
        "frame_begin", "frame_clear", "frame_end", "reexec", "camera_set_stack",
        "drawtext_screen", "drawtext_var_xy_screen", "drawtext_xy_stack_screen",
        "const_payload", "var_set_payload", "var_read_payload", "var_write_payload",
        "and", "key_down", "key_pressed", "cond_payload", "registry_find",
        "registry_token_name", "text_input", "string_append_var", "string_backspace_var",
        "string_clear_var", "block_insert_stack", "block_delete", "block_flush",
        "jump_payload", "mouse_f", "mouse_wheel", "mouse_button_down", "mouse_button_pressed",
        "i32_to_f32", "f32_add", "f32_sub", "f32_mul", "f32_const", "f32_clamp",
        "world_mouse", "drop_u32", "swap_u32", "views", "views_render",
        "block_select_stack", "block_offset_at_index", "measure_text_var", "not",
        "screen_size",
    }
    missing = sorted(required - t.keys())
    if missing:
        raise RuntimeError("missing tokens: " + ", ".join(missing))

    action_keys = {n: hashlib.sha256(("#atomic.action." + n).encode()).digest() for n in ACTION_CORE}
    for n in ("pan", "click", "rmb", "drag", "end_drag", "begin_pan", "end_pan", "begin_drag_anchor"):
        action_keys[n] = hashlib.sha256(("#atomic.action." + n).encode()).digest()

    hit_args = f32(32.0) + f32(520.0) + f32(24.0) + u32(256)

    # Capture pan anchors once when pan becomes active.
    begin_pan = make_block([
        (t["mouse_f"], b""),
        (t["var_write_payload"], GRAB_MY),
        (t["var_write_payload"], GRAB_MX),
        (t["var_read_payload"], CAM_X), (t["var_write_payload"], GRAB_CX),
        (t["var_read_payload"], CAM_Y), (t["var_write_payload"], GRAB_CY),
        (t["const_payload"], u32(1)), (t["var_write_payload"], PAN_ACTIVE),
    ])

    end_pan = make_block([
        (t["const_payload"], u32(0)), (t["var_write_payload"], PAN_ACTIVE),
    ])

    # Absolute pan: cam = grab_cam - (mouse - grab_mouse) / zoom
    # Path-independent: mouse back to grab_mouse => cam back to grab_cam.
    pan = make_block([
        # if not pan_active: begin_pan (capture anchors)
        (t["var_read_payload"], PAN_ACTIVE),
        (t["not"], b""),
        cond_action(t, action_keys["begin_pan"]),
        # cam_x
        (t["var_read_payload"], GRAB_CX),
        (t["mouse_f"], b""), (t["drop_u32"], b""),
        (t["var_read_payload"], GRAB_MX), (t["f32_sub"], b""),
        (t["var_read_payload"], CAM_Z), (t["f32_div"], b""),
        (t["f32_sub"], b""),
        (t["var_write_payload"], CAM_X),
        # cam_y
        (t["var_read_payload"], GRAB_CY),
        (t["mouse_f"], b""), (t["swap_u32"], b""), (t["drop_u32"], b""),
        (t["var_read_payload"], GRAB_MY), (t["f32_sub"], b""),
        (t["var_read_payload"], CAM_Z), (t["f32_div"], b""),
        (t["f32_sub"], b""),
        (t["var_write_payload"], CAM_Y),
    ])

    begin_drag_anchor = make_block([
        (t["mouse_f"], b""),
        (t["var_write_payload"], GRAB_MY),
        (t["var_write_payload"], GRAB_MX),
        views_op(t, 32),
        (t["var_write_payload"], GRAB_VY),
        (t["var_write_payload"], GRAB_VX),
        (t["const_payload"], u32(1)), (t["var_write_payload"], DRAG_ANCHORED),
    ])

    rmb = make_block([
        (t["world_mouse"], b""),
        views_op(t, 30, hit_args),
        (t["drop_u32"], b""),
        # always re-anchor after rmb interaction (open/title drag)
        (t["const_payload"], u32(0)), (t["var_write_payload"], DRAG_ANCHORED),
        (t["mouse_f"], b""),
        (t["var_write_payload"], GRAB_MY),
        (t["var_write_payload"], GRAB_MX),
        views_op(t, 32),
        (t["var_write_payload"], GRAB_VY),
        (t["var_write_payload"], GRAB_VX),
        (t["const_payload"], u32(1)), (t["var_write_payload"], DRAG_ANCHORED),
    ])

    # Absolute drag: view = grab_view + (mouse - grab_mouse) / zoom
    drag = make_block([
        (t["var_read_payload"], DRAG_ANCHORED),
        (t["not"], b""),
        cond_action(t, action_keys["begin_drag_anchor"]),
        (t["var_read_payload"], GRAB_VX),
        (t["mouse_f"], b""), (t["drop_u32"], b""),
        (t["var_read_payload"], GRAB_MX), (t["f32_sub"], b""),
        (t["var_read_payload"], CAM_Z), (t["f32_div"], b""),
        (t["f32_add"], b""),
        (t["var_read_payload"], GRAB_VY),
        (t["mouse_f"], b""), (t["swap_u32"], b""), (t["drop_u32"], b""),
        (t["var_read_payload"], GRAB_MY), (t["f32_sub"], b""),
        (t["var_read_payload"], CAM_Z), (t["f32_div"], b""),
        (t["f32_add"], b""),
        views_op(t, 33),
    ])

    end_drag = make_block([
        views_op(t, 13),
        (t["const_payload"], u32(0)), (t["var_write_payload"], DRAG_ANCHORED),
    ])

    click = make_block([(t["world_mouse"], b""), views_op(t, 29, hit_args), (t["drop_u32"], b"")])

    def sel():
        return [views_op(t, 22), (t["block_select_stack"], b"")]

    def cur():
        return [views_op(t, 21), (t["block_offset_at_index"], b"")]

    actions = {
        "down": make_block([views_op(t, 23, i32(1))]),
        "up": make_block([views_op(t, 24)]),
        "delete": make_block(sel() + cur() + [(t["block_delete"], b""), (t["jump_payload"], PROGRAM_KEY)]),
        "insert": make_block(sel() + cur() + [
            (t["var_read_payload"], INPUT_VAR), (t["registry_find"], b""),
            (t["block_insert_stack"], b""), (t["string_clear_var"], INPUT_VAR),
            (t["jump_payload"], PROGRAM_KEY),
        ]),
        "backspace": make_block([(t["string_backspace_var"], INPUT_VAR)]),
        "clear": make_block([(t["string_clear_var"], INPUT_VAR)]),
        "save": make_block(sel() + [(t["block_flush"], b"")]),
        "begin_pan": begin_pan,
        "end_pan": end_pan,
        "begin_drag_anchor": begin_drag_anchor,
        "pan": pan,
        "click": click,
        "rmb": rmb,
        "drag": drag,
        "end_drag": end_drag,
    }

    program = [
        (t["frame_begin"], b""),
        views_op(t, 18, PROGRAM_KEY + f32(40.0) + f32(70.0)),
        (t["var_read_payload"], CAM_X), (t["var_read_payload"], CAM_Y), (t["var_read_payload"], CAM_Z),
        (t["camera_set_stack"], b""),
        (t["frame_clear"], u32(0xff11161b)),

        (t["mouse_button_pressed"], u32(1)), cond_action(t, action_keys["click"]),
        (t["mouse_button_pressed"], u32(2)), cond_action(t, action_keys["rmb"]),

        # MMB absolute pan while held; clear active on release
        (t["mouse_button_down"], u32(4)), cond_action(t, action_keys["pan"]),
        (t["mouse_button_down"], u32(4)), (t["not"], b""), cond_action(t, action_keys["end_pan"]),

        # RMB absolute view drag while held
        (t["mouse_button_down"], u32(2)), cond_action(t, action_keys["drag"]),
        (t["mouse_button_down"], u32(2)), (t["not"], b""), cond_action(t, action_keys["end_drag"]),

        (t["var_read_payload"], CAM_Z),
        (t["mouse_wheel"], b""), (t["i32_to_f32"], b""),
        (t["f32_const"], f32(0.1)), (t["f32_mul"], b""),
        (t["f32_const"], f32(1.0)), (t["f32_add"], b""), (t["f32_mul"], b""),
        (t["f32_clamp"], f32(0.15) + f32(6.0)),
        (t["var_write_payload"], CAM_Z),

        (t["var_read_payload"], CAM_X), (t["var_read_payload"], CAM_Y), (t["var_read_payload"], CAM_Z),
        (t["camera_set_stack"], b""),

        (t["views_render"], VIEWS),

        (t["drawtext_screen"], static_text(20, 16, 0xff9da7b3, 16.0,
            b"multi-view | absolute grab pan/drag | RMB open payload-hash | Ctrl+S")),
        (t["text_input"], b""), (t["string_append_var"], INPUT_VAR),

        (t["screen_size"], b""), (t["i32_to_f32"], b""),
        (t["f32_const"], f32(52.0)), (t["f32_sub"], b""),
        (t["swap_u32"], b""), (t["drop_u32"], b""),
        (t["f32_const"], f32(20.0)), (t["swap_u32"], b""),
        (t["drawtext_var_xy_screen"], INPUT_VAR + struct.pack("<If", 0xffffffff, 17.0)),

        (t["screen_size"], b""), (t["i32_to_f32"], b""),
        (t["f32_const"], f32(52.0)), (t["f32_sub"], b""),
        (t["swap_u32"], b""), (t["drop_u32"], b""),
        (t["measure_text_var"], INPUT_VAR + f32(17.0)),
        (t["f32_const"], f32(32.0)), (t["f32_add"], b""),
        (t["swap_u32"], b""),
        (t["var_read_payload"], INPUT_VAR), (t["registry_find"], b""),
        (t["registry_token_name"], b""),
        (t["drawtext_xy_stack_screen"], struct.pack("<IfI", 0xff73808c, 17.0, 96)),

        (t["key_pressed"], u32(0x28)), cond_action(t, action_keys["down"]),
        (t["key_pressed"], u32(0x26)), cond_action(t, action_keys["up"]),
        (t["key_pressed"], u32(0x2e)), cond_action(t, action_keys["delete"]),
        (t["key_pressed"], u32(0x20)), cond_action(t, action_keys["insert"]),
        (t["key_pressed"], u32(0x09)), cond_action(t, action_keys["insert"]),
        (t["key_pressed"], u32(0x08)), cond_action(t, action_keys["backspace"]),
        (t["key_pressed"], u32(0x1b)), cond_action(t, action_keys["clear"]),
        (t["key_down"], u32(0x11)), (t["key_pressed"], u32(ord("S"))), (t["and"], b""),
        cond_action(t, action_keys["save"]),
        (t["frame_end"], b""), (t["reexec"], b""),
    ]

    first = make_block([
        alloc_var(t, INPUT_VAR, 256),
        alloc_var(t, CAM_X, 4), alloc_var(t, CAM_Y, 4), alloc_var(t, CAM_Z, 4),
        alloc_var(t, GRAB_MX, 4), alloc_var(t, GRAB_MY, 4),
        alloc_var(t, GRAB_CX, 4), alloc_var(t, GRAB_CY, 4),
        alloc_var(t, GRAB_VX, 4), alloc_var(t, GRAB_VY, 4),
        alloc_var(t, PAN_ACTIVE, 4), alloc_var(t, DRAG_ANCHORED, 4),
        alloc_var(t, VIEWS, 2320),
        (t["const_payload"], f32(640.0)), (t["var_write_payload"], CAM_X),
        (t["const_payload"], f32(360.0)), (t["var_write_payload"], CAM_Y),
        (t["const_payload"], f32(1.0)), (t["var_write_payload"], CAM_Z),
        (t["const_payload"], f32(0.0)), (t["var_write_payload"], GRAB_MX),
        (t["const_payload"], f32(0.0)), (t["var_write_payload"], GRAB_MY),
        (t["const_payload"], f32(640.0)), (t["var_write_payload"], GRAB_CX),
        (t["const_payload"], f32(360.0)), (t["var_write_payload"], GRAB_CY),
        (t["const_payload"], f32(0.0)), (t["var_write_payload"], GRAB_VX),
        (t["const_payload"], f32(0.0)), (t["var_write_payload"], GRAB_VY),
        (t["const_payload"], u32(0)), (t["var_write_payload"], PAN_ACTIVE),
        (t["const_payload"], u32(0)), (t["var_write_payload"], DRAG_ANCHORED),
        (PROGRAM_KEY, b""),
    ])
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
        (action_dir / f"{name}.bin").write_bytes(raw)
        collect(raw)

    name_by_token = {token: name for name, token in t.items()}
    manifest = {"program_key": PROGRAM_KEY.hex(), "native": {}, "actions": {}}
    for tok in used:
        if tok in logical:
            continue
        name = name_by_token.get(tok)
        if name is None:
            raise RuntimeError("unknown native token " + tok.hex())
        manifest["native"][name] = tok.hex()
    for name, raw in actions.items():
        manifest["actions"][name] = {
            "key": action_keys[name].hex(),
            "hash": hashlib.sha256(raw).hexdigest(),
        }
    manifest["first_hash"] = hashlib.sha256(first).hexdigest()
    manifest["program_hash"] = hashlib.sha256(program_raw).hexdigest()
    (ROOT / "atomic_first_boot_manifest.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="ascii")
    print("first", manifest["first_hash"], len(first))
    print("program", manifest["program_hash"], len(program_raw), len(program), "instructions")
    for name, value in manifest["actions"].items():
        print(name, value["hash"][:16])
    print("natives", len(manifest["native"]))


if __name__ == "__main__":
    main()
