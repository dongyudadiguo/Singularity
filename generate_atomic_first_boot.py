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

# Library module keys (published as reusable blocks; frame inlines them to avoid
# vmstore  cache thrash / nested-exec frame pressure during pan).
MOD_CAMERA = hashlib.sha256(b"#atomic.mod.camera").digest()
MOD_INPUT = hashlib.sha256(b"#atomic.mod.input").digest()
MOD_ZOOM = hashlib.sha256(b"#atomic.mod.zoom").digest()
MOD_HUD = hashlib.sha256(b"#atomic.mod.hud").digest()
MOD_EDITOR = hashlib.sha256(b"#atomic.mod.editor").digest()

ACTION_CORE = ("down", "up", "delete", "insert", "backspace", "clear", "save")
ACTION_POINTER = (
    "pan", "click", "rmb", "drag", "end_drag",
    "begin_pan", "end_pan", "begin_drag_anchor",
)


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


# ---------------------------------------------------------------------------
# Modular builders (Python + published library blocks). Frame INLINES these.
# ---------------------------------------------------------------------------

def build_begin_pan(t):
    return [
        (t["mouse_f"], b""),
        (t["var_write_payload"], GRAB_MY),
        (t["var_write_payload"], GRAB_MX),
        (t["var_read_payload"], CAM_X), (t["var_write_payload"], GRAB_CX),
        (t["var_read_payload"], CAM_Y), (t["var_write_payload"], GRAB_CY),
        (t["const_payload"], u32(1)), (t["var_write_payload"], PAN_ACTIVE),
    ]


def build_end_pan(t):
    return [
        (t["const_payload"], u32(0)), (t["var_write_payload"], PAN_ACTIVE),
    ]


def build_pan(t, action_keys):
    return [
        (t["var_read_payload"], PAN_ACTIVE),
        (t["not"], b""),
        cond_action(t, action_keys["begin_pan"]),
        (t["var_read_payload"], GRAB_CX),
        (t["mouse_f"], b""), (t["drop_u32"], b""),
        (t["var_read_payload"], GRAB_MX), (t["f32_sub"], b""),
        (t["var_read_payload"], CAM_Z), (t["f32_div"], b""),
        (t["f32_sub"], b""),
        (t["var_write_payload"], CAM_X),
        (t["var_read_payload"], GRAB_CY),
        (t["mouse_f"], b""), (t["swap_u32"], b""), (t["drop_u32"], b""),
        (t["var_read_payload"], GRAB_MY), (t["f32_sub"], b""),
        (t["var_read_payload"], CAM_Z), (t["f32_div"], b""),
        (t["f32_sub"], b""),
        (t["var_write_payload"], CAM_Y),
    ]


def build_begin_drag_anchor(t):
    return [
        (t["mouse_f"], b""),
        (t["var_write_payload"], GRAB_MY),
        (t["var_write_payload"], GRAB_MX),
        views_op(t, 32),
        (t["var_write_payload"], GRAB_VY),
        (t["var_write_payload"], GRAB_VX),
        (t["const_payload"], u32(1)), (t["var_write_payload"], DRAG_ANCHORED),
    ]


def build_rmb(t):
    hit_args = f32(32.0) + f32(0.0) + f32(24.0) + u32(256)
    return [
        (t["world_mouse"], b""),
        views_op(t, 30, hit_args),
        (t["drop_u32"], b""),
        (t["const_payload"], u32(0)), (t["var_write_payload"], DRAG_ANCHORED),
        (t["mouse_f"], b""),
        (t["var_write_payload"], GRAB_MY),
        (t["var_write_payload"], GRAB_MX),
        views_op(t, 32),
        (t["var_write_payload"], GRAB_VY),
        (t["var_write_payload"], GRAB_VX),
        (t["const_payload"], u32(1)), (t["var_write_payload"], DRAG_ANCHORED),
    ]


def build_drag(t, action_keys):
    return [
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
    ]


def build_end_drag(t):
    return [
        views_op(t, 13),
        (t["const_payload"], u32(0)), (t["var_write_payload"], DRAG_ANCHORED),
    ]


def build_click(t):
    hit_args = f32(32.0) + f32(0.0) + f32(24.0) + u32(256)
    return [(t["world_mouse"], b""), views_op(t, 29, hit_args), (t["drop_u32"], b"")]


def sel(t):
    return [views_op(t, 22), (t["block_select_stack"], b"")]


def cur(t):
    return [views_op(t, 21), (t["block_offset_at_index"], b"")]


def build_editor_actions(t):
    return {
        "down": make_block([views_op(t, 23, i32(1))]),
        "up": make_block([views_op(t, 24)]),
        "delete": make_block(sel(t) + cur(t) + [(t["block_delete"], b""), (t["jump_payload"], PROGRAM_KEY)]),
        "insert": make_block(sel(t) + cur(t) + [
            (t["var_read_payload"], INPUT_VAR), (t["registry_find"], b""),
            (t["block_insert_stack"], b""), (t["string_clear_var"], INPUT_VAR),
            (t["jump_payload"], PROGRAM_KEY),
        ]),
        "backspace": make_block([(t["string_backspace_var"], INPUT_VAR)]),
        "clear": make_block([(t["string_clear_var"], INPUT_VAR)]),
        "save": make_block(sel(t) + [(t["block_flush"], b"")]),
    }


def mod_camera_apply(t):
    return [
        (t["var_read_payload"], CAM_X),
        (t["var_read_payload"], CAM_Y),
        (t["var_read_payload"], CAM_Z),
        (t["camera_set_stack"], b""),
    ]


def mod_zoom(t):
    """Mouse-centered zoom: factor=1+wheel*0.08, cam keeps world under cursor."""
    return [
        (t["var_read_payload"], CAM_Z),
        (t["dup_u32"], b""),
        (t["mouse_wheel"], b""),
        (t["i32_to_f32"], b""),
        (t["f32_const"], f32(0.08)),
        (t["f32_mul"], b""),
        (t["f32_const"], f32(1.0)),
        (t["f32_add"], b""),
        (t["f32_mul"], b""),
        (t["f32_clamp"], f32(0.15) + f32(6.0)),
        (t["dup_u32"], b""),
        (t["var_write_payload"], CAM_Z),
        (t["f32_div"], b""),  # ratio = old/new
        (t["dup_u32"], b""),
        # cam_x
        (t["var_read_payload"], CAM_X),
        (t["world_mouse"], b""),
        (t["drop_u32"], b""),
        (t["f32_sub"], b""),
        (t["f32_mul"], b""),
        (t["world_mouse"], b""),
        (t["drop_u32"], b""),
        (t["f32_add"], b""),
        (t["var_write_payload"], CAM_X),
        # cam_y
        (t["var_read_payload"], CAM_Y),
        (t["world_mouse"], b""),
        (t["swap_u32"], b""),
        (t["drop_u32"], b""),
        (t["f32_sub"], b""),
        (t["f32_mul"], b""),
        (t["world_mouse"], b""),
        (t["swap_u32"], b""),
        (t["drop_u32"], b""),
        (t["f32_add"], b""),
        (t["var_write_payload"], CAM_Y),
    ]


def mod_input_pointer(t, action_keys):
    return [
        (t["mouse_button_pressed"], u32(1)), cond_action(t, action_keys["click"]),
        (t["mouse_button_pressed"], u32(2)), cond_action(t, action_keys["rmb"]),
        (t["mouse_button_down"], u32(4)), cond_action(t, action_keys["pan"]),
        (t["mouse_button_down"], u32(4)), (t["not"], b""), cond_action(t, action_keys["end_pan"]),
        (t["mouse_button_down"], u32(2)), cond_action(t, action_keys["drag"]),
        (t["mouse_button_down"], u32(2)), (t["not"], b""), cond_action(t, action_keys["end_drag"]),
    ]


def mod_hud(t):
    return [
        (t["drawtext_screen"], static_text(20, 16, 0xff9da7b3, 16.0,
            b"multi-view | text-width hit | mouse-zoom | modules | Ctrl+S")),
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
    ]


def mod_editor_keys(t, action_keys):
    return [
        (t["key_pressed"], u32(0x28)), cond_action(t, action_keys["down"]),
        (t["key_pressed"], u32(0x26)), cond_action(t, action_keys["up"]),
        (t["key_pressed"], u32(0x2e)), cond_action(t, action_keys["delete"]),
        (t["key_pressed"], u32(0x20)), cond_action(t, action_keys["insert"]),
        (t["key_pressed"], u32(0x09)), cond_action(t, action_keys["insert"]),
        (t["key_pressed"], u32(0x08)), cond_action(t, action_keys["backspace"]),
        (t["key_pressed"], u32(0x1b)), cond_action(t, action_keys["clear"]),
        (t["key_down"], u32(0x11)), (t["key_pressed"], u32(ord("S"))), (t["and"], b""),
        cond_action(t, action_keys["save"]),
    ]


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
        "i32_to_f32", "f32_add", "f32_sub", "f32_mul", "f32_div", "f32_const", "f32_clamp",
        "world_mouse", "drop_u32", "swap_u32", "dup_u32", "views", "views_render",
        "block_select_stack", "block_offset_at_index", "measure_text_var", "not",
        "screen_size",
    }
    missing = sorted(required - t.keys())
    if missing:
        raise RuntimeError("missing tokens: " + ", ".join(missing))

    action_keys = {n: hashlib.sha256(("#atomic.action." + n).encode()).digest() for n in ACTION_CORE}
    for n in ACTION_POINTER:
        action_keys[n] = hashlib.sha256(("#atomic.action." + n).encode()).digest()

    actions = {}
    actions.update(build_editor_actions(t))
    actions["begin_pan"] = make_block(build_begin_pan(t))
    actions["end_pan"] = make_block(build_end_pan(t))
    actions["begin_drag_anchor"] = make_block(build_begin_drag_anchor(t))
    actions["pan"] = make_block(build_pan(t, action_keys))
    actions["click"] = make_block(build_click(t))
    actions["rmb"] = make_block(build_rmb(t))
    actions["drag"] = make_block(build_drag(t, action_keys))
    actions["end_drag"] = make_block(build_end_drag(t))

    # Library modules (same instruction lists) — published for reuse/swap
    modules = {
        "camera": (MOD_CAMERA, make_block(mod_camera_apply(t))),
        "input": (MOD_INPUT, make_block(mod_input_pointer(t, action_keys))),
        "zoom": (MOD_ZOOM, make_block(mod_zoom(t))),
        "hud": (MOD_HUD, make_block(mod_hud(t))),
        "editor": (MOD_EDITOR, make_block(mod_editor_keys(t, action_keys))),
    }

    # Frame inlines modules (stream-embedded) — no const+exec nesting.
    program = [
        (t["frame_begin"], b""),
        views_op(t, 18, PROGRAM_KEY + f32(40.0) + f32(70.0)),
        *mod_camera_apply(t),
        (t["frame_clear"], u32(0xff11161b)),
        *mod_input_pointer(t, action_keys),
        *mod_zoom(t),
        *mod_camera_apply(t),
        (t["views_render"], VIEWS),
        *mod_hud(t),
        *mod_editor_keys(t, action_keys),
        (t["frame_end"], b""),
        (t["reexec"], b""),
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

    module_dir = ROOT / "atomic_module_blocks"
    module_dir.mkdir(exist_ok=True)
    for old in module_dir.glob("*.bin"):
        old.unlink()

    used = set()
    logical = {PROGRAM_KEY}
    logical.update(action_keys.values())
    # Module keys are library-only (not instruction tokens when inlined)
    module_keys = {key for key, _ in modules.values()}

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
    for name, (key, raw) in modules.items():
        (module_dir / f"{name}.bin").write_bytes(raw)
        collect(raw)

    name_by_token = {token: name for name, token in t.items()}
    manifest = {
        "program_key": PROGRAM_KEY.hex(),
        "native": {},
        "actions": {},
        "modules": {},
        "modules_inlined": True,
    }
    for tok in used:
        if tok in logical or tok in module_keys:
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
    for name, (key, raw) in modules.items():
        manifest["modules"][name] = {
            "key": key.hex(),
            "hash": hashlib.sha256(raw).hexdigest(),
        }
    manifest["first_hash"] = hashlib.sha256(first).hexdigest()
    manifest["program_hash"] = hashlib.sha256(program_raw).hexdigest()
    (ROOT / "atomic_first_boot_manifest.json").write_text(
        json.dumps(manifest, indent=2) + "\n", encoding="ascii"
    )
    print("first", manifest["first_hash"], len(first))
    print("program", manifest["program_hash"], len(program_raw), len(program), "instructions (inlined modules)")
    for name, value in manifest["actions"].items():
        print("action", name, value["hash"][:16])
    for name, value in manifest["modules"].items():
        print("module", name, value["hash"][:16], "(library)")
    print("natives", len(manifest["native"]))


if __name__ == "__main__":
    main()
