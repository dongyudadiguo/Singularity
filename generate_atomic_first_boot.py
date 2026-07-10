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


def mod_key(name: str) -> bytes:
    """Top-level module logical key (stable salt; name display is plain)."""
    return hashlib.sha256(f"#atomic.mod.{name}".encode("ascii")).digest()


def part_key(name: str) -> bytes:
    """Named firstchild part of a specialized composite."""
    return hashlib.sha256(f"#atomic.part.{name}".encode("ascii")).digest()


# Top-level frame modules (logical, non-DLL). Keep legacy mod salts.
MOD_CAMERA = mod_key("camera")
MOD_INPUT = mod_key("input")
MOD_ZOOM = mod_key("zoom")
MOD_HUD = mod_key("hud")
MOD_EDITOR = mod_key("editor")

# Specialized composites -> named firstchild parts (more integrated => more parts).
PART_STATUS = part_key("status")
PART_TYPEIN = part_key("typein")
PART_MATCH = part_key("match")
PART_NAV = part_key("nav")
PART_EDITKEYS = part_key("editkeys")
PART_SAVEKEY = part_key("savekey")
PART_CLICK_ON = part_key("click_on")
PART_RMB_ON = part_key("rmb_on")
PART_PAN_ON = part_key("pan_on")
PART_DRAG_ON = part_key("drag_on")
PART_PAN_X = part_key("pan_x")
PART_PAN_Y = part_key("pan_y")
PART_ZOOM_Z = part_key("zoom_z")
PART_ZOOM_X = part_key("zoom_x")
PART_ZOOM_Y = part_key("zoom_y")
PART_DRAG_XY = part_key("drag_xy")
PART_RMB_OPEN = part_key("rmb_open")

ACTION_CORE = ("down", "up", "delete", "insert", "backspace", "clear", "save")
ACTION_POINTER = (
    "pan", "click", "rmb", "drag", "end_drag",
    "begin_pan", "end_pan", "begin_drag_anchor",
    "zoom_apply",
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
    return result


def write_instruction_names(result):
    """token -> friendly name for views_render + registry_find match."""
    records = []
    for name, token in sorted(result.items(), key=lambda kv: kv[0]):
        encoded = name.encode("ascii")[:95]
        records.append(token + encoded + bytes(96 - len(encoded)))
    (ROOT / "instruction_names.bin").write_bytes(struct.pack("<I", len(records)) + b"".join(records))
    return len(records)


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


def bare(key):
    """Place a logical token in the stream (empty payload). Resolves to firstchild block."""
    return (key, b"")


# ---------------------------------------------------------------------------
# Leaf action bodies (short reusable units; still logical firstchild blocks)
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


# ---------------------------------------------------------------------------
# Decomposed parts: specialized math / draw / bind units
# ---------------------------------------------------------------------------

def part_pan_x(t):
    return [
        (t["var_read_payload"], GRAB_CX),
        (t["mouse_f"], b""), (t["drop_u32"], b""),
        (t["var_read_payload"], GRAB_MX), (t["f32_sub"], b""),
        (t["var_read_payload"], CAM_Z), (t["f32_div"], b""),
        (t["f32_sub"], b""),
        (t["var_write_payload"], CAM_X),
    ]


def part_pan_y(t):
    return [
        (t["var_read_payload"], GRAB_CY),
        (t["mouse_f"], b""), (t["swap_u32"], b""), (t["drop_u32"], b""),
        (t["var_read_payload"], GRAB_MY), (t["f32_sub"], b""),
        (t["var_read_payload"], CAM_Z), (t["f32_div"], b""),
        (t["f32_sub"], b""),
        (t["var_write_payload"], CAM_Y),
    ]


def part_drag_xy(t):
    return [
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


def part_zoom_z(t):
    """new_z = clamp(old_z * (1 + wheel_notches*0.08)); leave ratio=old/new on stack.
    mouse_wheel pushes f32 notches (fractional high-res wheel supported)."""
    return [
        (t["var_read_payload"], CAM_Z),
        (t["dup_u32"], b""),
        (t["mouse_wheel"], b""),  # f32 notches this frame (fractional high-res)
        # ~8.5%/notch; fractional notches still move cam (was easy to feel as "no-op")
        (t["f32_const"], f32(0.10)),
        (t["f32_mul"], b""),
        (t["f32_const"], f32(1.0)),
        (t["f32_add"], b""),
        (t["f32_mul"], b""),
        (t["f32_clamp"], f32(0.15) + f32(6.0)),
        (t["dup_u32"], b""),
        (t["var_write_payload"], CAM_Z),
        (t["f32_div"], b""),  # stack: ratio
    ]


def part_zoom_x(t):
    """cam_x' = world_x + (cam_x - world_x) * ratio; consumes one ratio, leaves ratio."""
    return [
        (t["dup_u32"], b""),
        (t["var_read_payload"], CAM_X),
        (t["world_mouse"], b""),
        (t["drop_u32"], b""),
        (t["f32_sub"], b""),
        (t["f32_mul"], b""),
        (t["world_mouse"], b""),
        (t["drop_u32"], b""),
        (t["f32_add"], b""),
        (t["var_write_payload"], CAM_X),
    ]


def part_zoom_y(t):
    """cam_y' = world_y + (cam_y - world_y) * ratio; consumes ratio."""
    return [
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


def part_rmb_open(t):
    hit_args = f32(32.0) + f32(0.0) + f32(24.0) + u32(256)
    return [
        (t["world_mouse"], b""),
        views_op(t, 30, hit_args),
        (t["drop_u32"], b""),
    ]


def part_status(t):
    return [
        (t["drawtext_screen"], static_text(20, 16, 0xff9da7b3, 16.0,
            b"multi-view | modular token blocks | mouse-zoom | Ctrl+S")),
    ]


def part_typein(t):
    return [
        (t["text_input"], b""), (t["string_append_var"], INPUT_VAR),
        (t["screen_size"], b""), (t["i32_to_f32"], b""),
        (t["f32_const"], f32(52.0)), (t["f32_sub"], b""),
        (t["swap_u32"], b""), (t["drop_u32"], b""),
        (t["f32_const"], f32(20.0)), (t["swap_u32"], b""),
        (t["drawtext_var_xy_screen"], INPUT_VAR + struct.pack("<If", 0xffffffff, 17.0)),
    ]


def part_match(t):
    return [
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


def part_nav(t, action_keys):
    return [
        (t["key_pressed"], u32(0x28)), cond_action(t, action_keys["down"]),
        (t["key_pressed"], u32(0x26)), cond_action(t, action_keys["up"]),
    ]


def part_editkeys(t, action_keys):
    return [
        (t["key_pressed"], u32(0x2e)), cond_action(t, action_keys["delete"]),
        (t["key_pressed"], u32(0x20)), cond_action(t, action_keys["insert"]),
        (t["key_pressed"], u32(0x09)), cond_action(t, action_keys["insert"]),
        (t["key_pressed"], u32(0x08)), cond_action(t, action_keys["backspace"]),
        (t["key_pressed"], u32(0x1b)), cond_action(t, action_keys["clear"]),
    ]


def part_savekey(t, action_keys):
    return [
        (t["key_down"], u32(0x11)), (t["key_pressed"], u32(ord("S"))), (t["and"], b""),
        cond_action(t, action_keys["save"]),
    ]


def part_click_on(t, action_keys):
    return [
        (t["mouse_button_pressed"], u32(1)), cond_action(t, action_keys["click"]),
    ]


def part_rmb_on(t, action_keys):
    return [
        (t["mouse_button_pressed"], u32(2)), cond_action(t, action_keys["rmb"]),
    ]


def part_pan_on(t, action_keys):
    return [
        (t["mouse_button_down"], u32(4)), cond_action(t, action_keys["pan"]),
        (t["mouse_button_down"], u32(4)), (t["not"], b""), cond_action(t, action_keys["end_pan"]),
    ]


def part_drag_on(t, action_keys):
    return [
        (t["mouse_button_down"], u32(2)), cond_action(t, action_keys["drag"]),
        (t["mouse_button_down"], u32(2)), (t["not"], b""), cond_action(t, action_keys["end_drag"]),
    ]


# ---------------------------------------------------------------------------
# Composite logical tokens: bodies are bare firstchild tokens (not native soup)
# ---------------------------------------------------------------------------

def mod_camera_apply(t):
    # Small general-purpose apply: keep as native sequence (not specialized).
    return [
        (t["var_read_payload"], CAM_X),
        (t["var_read_payload"], CAM_Y),
        (t["var_read_payload"], CAM_Z),
        (t["camera_set_stack"], b""),
    ]


def mod_zoom(t, zoom_apply_key):
    return [
        (t["mouse_wheel"], b""),
        (t["cond_payload"], zoom_apply_key),
    ]


def mod_input(t):
    return [
        bare(PART_CLICK_ON),
        bare(PART_RMB_ON),
        bare(PART_PAN_ON),
        bare(PART_DRAG_ON),
    ]


def mod_hud(t):
    return [
        bare(PART_STATUS),
        bare(PART_TYPEIN),
        bare(PART_MATCH),
    ]


def mod_editor(t):
    return [
        bare(PART_NAV),
        bare(PART_EDITKEYS),
        bare(PART_SAVEKEY),
    ]


def build_pan(t, action_keys):
    # Specialized pan: ensure anchor, then axis parts as firstchild tokens.
    return [
        (t["var_read_payload"], PAN_ACTIVE),
        (t["not"], b""),
        cond_action(t, action_keys["begin_pan"]),
        bare(PART_PAN_X),
        bare(PART_PAN_Y),
    ]


def build_drag(t, action_keys):
    return [
        (t["var_read_payload"], DRAG_ANCHORED),
        (t["not"], b""),
        cond_action(t, action_keys["begin_drag_anchor"]),
        bare(PART_DRAG_XY),
    ]


def build_rmb(t, action_keys):
    return [
        bare(PART_RMB_OPEN),
        (t["const_payload"], u32(0)), (t["var_write_payload"], DRAG_ANCHORED),
        bare(action_keys["begin_drag_anchor"]),
    ]


def build_zoom_apply(t):
    return [
        bare(PART_ZOOM_Z),
        bare(PART_ZOOM_X),
        bare(PART_ZOOM_Y),
    ]



# ---------------------------------------------------------------------------
# Specialized native surfaces: DLL still executes via find(); editor open
# resolves override -> instruction block of named firstchild facets.
# More integrated/specialized => more named facet tokens.
# ---------------------------------------------------------------------------

def sample_views(t, op, args=b""):
    """One-instruction leaf showing a views op call pattern."""
    return make_block([views_op(t, op, args)])


def build_native_surfaces(t):
    """Map surface_name -> (native_token, surface_block_bytes, facet_modules).

    facet_modules: name -> (key, raw) extra logical blocks referenced by surface.
    """
    hit_args = f32(32.0) + f32(0.0) + f32(24.0) + u32(256)
    ensure_args = PROGRAM_KEY + f32(40.0) + f32(70.0)

    # Facet keys (logical). Display names are plain: ensure, pointer_rmb, ...
    facets = {
        "ensure": (part_key("views.ensure"), sample_views(t, 18, ensure_args)),
        "active_key": (part_key("views.active_key"), sample_views(t, 22)),
        "active_cursor": (part_key("views.active_cursor"), sample_views(t, 21)),
        "cursor_add": (part_key("views.cursor_add"), sample_views(t, 23, i32(1))),
        "cursor_dec": (part_key("views.cursor_dec"), sample_views(t, 24)),
        "pointer_lmb": (part_key("views.pointer_lmb"), sample_views(t, 29, hit_args)),
        "pointer_rmb": (part_key("views.pointer_rmb"), sample_views(t, 30, hit_args)),
        "drag_end": (part_key("views.drag_end"), sample_views(t, 13)),
        "get_drag_xy": (part_key("views.get_drag_xy"), sample_views(t, 32)),
        "set_drag_xy": (part_key("views.set_drag_xy"), sample_views(t, 33)),
        "open": (part_key("views.open"), sample_views(
            t, 14, PROGRAM_KEY + f32(120.0) + f32(90.0) + i32(-1) + f32(80.0) + f32(10.0))),
        "init": (part_key("views.init"), sample_views(t, 0, PROGRAM_KEY + f32(40.0) + f32(70.0))),
        "count": (part_key("views.count"), sample_views(t, 1)),
        "active": (part_key("views.active"), sample_views(t, 2)),
        "set_active": (part_key("views.set_active"), sample_views(t, 3, u32(0))),
        "get_xy": (part_key("views.get_xy"), sample_views(t, 4, u32(0))),
        "set_xy": (part_key("views.set_xy"), sample_views(t, 5, u32(0) + f32(0.0) + f32(0.0))),
        "get_key": (part_key("views.get_key"), sample_views(t, 6, u32(0))),
        "get_cursor": (part_key("views.get_cursor"), sample_views(t, 9, u32(0))),
        "set_cursor": (part_key("views.set_cursor"), sample_views(t, 10, u32(0) + u32(0))),
        "drag_begin": (part_key("views.drag_begin"), sample_views(t, 11, u32(0))),
        "drag_step": (part_key("views.drag_step"), sample_views(t, 12, f32(0.0) + f32(0.0))),
        "hit_title": (part_key("views.hit_title"), sample_views(t, 15, f32(32.0))),
        "hit_row": (part_key("views.hit_row"), sample_views(t, 16, f32(24.0) + u32(256))),
        "move_by": (part_key("views.move_by"), sample_views(t, 17, u32(0) + f32(0.0) + f32(0.0))),
        "get_dragging": (part_key("views.get_dragging"), sample_views(t, 19)),
        "set_cursor_active": (part_key("views.set_cursor_active"), sample_views(t, 20, u32(0))),
    }

    # Surface body: only bare facet tokens (decomposable firstchild list).
    # Order = primary specialized API surface first, then secondary ops.
    surface_order = [
        "ensure", "pointer_lmb", "pointer_rmb", "get_drag_xy", "set_drag_xy",
        "drag_end", "active_key", "active_cursor", "cursor_add", "cursor_dec",
        "open", "init", "count", "active", "set_active",
        "get_xy", "set_xy", "get_key", "get_cursor", "set_cursor",
        "drag_begin", "drag_step", "hit_title", "hit_row", "move_by",
        "get_dragging", "set_cursor_active",
    ]
    views_surface = make_block([bare(facets[n][0]) for n in surface_order])

    # views_render: specialized renderer — expose as its own surface with leaf.
    render_leaf_key = part_key("views.render_draw")
    render_leaf = make_block([(t["views_render"], VIEWS)])
    render_surface = make_block([bare(render_leaf_key)])

    surfaces = {
        "views": {
            "native": "views",
            "token": t["views"],
            "block": views_surface,
            "facets": {n: facets[n] for n in surface_order},
        },
        "views_render": {
            "native": "views_render",
            "token": t["views_render"],
            "block": render_surface,
            "facets": {"render_draw": (render_leaf_key, render_leaf)},
        },
    }
    return surfaces


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
    actions["rmb"] = make_block(build_rmb(t, action_keys))
    actions["drag"] = make_block(build_drag(t, action_keys))
    actions["end_drag"] = make_block(build_end_drag(t))
    actions["zoom_apply"] = make_block(build_zoom_apply(t))

    # Parts first (leaf content), then composite modules that bare-call them.
    # Rule: the more specialized/integrated, the more firstchild token blocks.
    modules = {
        # --- leaf parts (editable units) ---
        "status": (PART_STATUS, make_block(part_status(t))),
        "typein": (PART_TYPEIN, make_block(part_typein(t))),
        "match": (PART_MATCH, make_block(part_match(t))),
        "nav": (PART_NAV, make_block(part_nav(t, action_keys))),
        "editkeys": (PART_EDITKEYS, make_block(part_editkeys(t, action_keys))),
        "savekey": (PART_SAVEKEY, make_block(part_savekey(t, action_keys))),
        "click_on": (PART_CLICK_ON, make_block(part_click_on(t, action_keys))),
        "rmb_on": (PART_RMB_ON, make_block(part_rmb_on(t, action_keys))),
        "pan_on": (PART_PAN_ON, make_block(part_pan_on(t, action_keys))),
        "drag_on": (PART_DRAG_ON, make_block(part_drag_on(t, action_keys))),
        "pan_x": (PART_PAN_X, make_block(part_pan_x(t))),
        "pan_y": (PART_PAN_Y, make_block(part_pan_y(t))),
        "zoom_z": (PART_ZOOM_Z, make_block(part_zoom_z(t))),
        "zoom_x": (PART_ZOOM_X, make_block(part_zoom_x(t))),
        "zoom_y": (PART_ZOOM_Y, make_block(part_zoom_y(t))),
        "drag_xy": (PART_DRAG_XY, make_block(part_drag_xy(t))),
        "rmb_open": (PART_RMB_OPEN, make_block(part_rmb_open(t))),
        # --- composite modules (bare firstchild parts only) ---
        "camera": (MOD_CAMERA, make_block(mod_camera_apply(t))),
        "input": (MOD_INPUT, make_block(mod_input(t))),
        "zoom": (MOD_ZOOM, make_block(mod_zoom(t, action_keys["zoom_apply"]))),
        "hud": (MOD_HUD, make_block(mod_hud(t))),
        "editor": (MOD_EDITOR, make_block(mod_editor(t))),
    }

    # Thin orchestrator: glue + bare logical module tokens (no exec).
    program = [
        (t["frame_begin"], b""),
        views_op(t, 18, PROGRAM_KEY + f32(40.0) + f32(70.0)),
        bare(MOD_CAMERA),
        (t["frame_clear"], u32(0xff11161b)),
        bare(MOD_INPUT),
        bare(MOD_ZOOM),
        bare(MOD_CAMERA),
        (t["views_render"], VIEWS),
        bare(MOD_HUD),
        bare(MOD_EDITOR),
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
    logical.update(key for key, _ in modules.values())

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

    # Specialized native surfaces (editor-open definition blocks).
    surfaces = build_native_surfaces(t)
    surface_dir = ROOT / "atomic_surface_blocks"
    surface_dir.mkdir(exist_ok=True)
    for old in surface_dir.glob("*.bin"):
        old.unlink()

    # Facets of surfaces are also logical modules (firstchild of the surface).
    for sname, sdef in surfaces.items():
        for fname, (fkey, fraw) in sdef["facets"].items():
            mod_name = f"{sname}.{fname}" if not fname.startswith(sname) else fname
            # keep short natural names for facets
            modules[fname if fname not in modules else mod_name] = (fkey, fraw)
            logical.add(fkey)
            collect(fraw)
            (module_dir / f"{fname}.bin").write_bytes(fraw)
        (surface_dir / f"{sname}.bin").write_bytes(sdef["block"])
        collect(sdef["block"])

    # Friendly names for surfaces + facets (natural; no prefix).
    for sname, sdef in surfaces.items():
        # surface itself is opened via native token; still name the native.
        for fname, (fkey, _fraw) in sdef["facets"].items():
            if fname not in t:
                t[fname] = fkey
    for name, (key, _raw) in modules.items():
        if name not in t:
            t[name] = key
    for name, key in action_keys.items():
        if name not in t:
            t[name] = key
    n_names = write_instruction_names(t)
    print("instruction_names", n_names)

    name_by_token = {token: name for name, token in t.items()}
    # Classify composite vs leaf for manifest readability.
    composite = {"camera", "input", "zoom", "hud", "editor"}
    manifest = {
        "program_key": PROGRAM_KEY.hex(),
        "native": {},
        "actions": {},
        "modules": {},
        "surfaces": {},
        "modules_inlined": False,
        "composition": (
            "bare logical tokens; specialized composites + specialized natives "
            "decompose into named firstchild part/surface blocks"
        ),
    }
    for tok in used:
        if tok in logical:
            continue
        name = name_by_token.get(tok)
        if name is None:
            raise RuntimeError("unknown native token " + tok.hex())
        manifest["native"][name] = tok.hex()
    # Ensure surface natives are listed even if only referenced via surface files.
    for sdef in surfaces.values():
        n = sdef["native"]
        manifest["native"][n] = sdef["token"].hex()
    for name, raw in actions.items():
        manifest["actions"][name] = {
            "key": action_keys[name].hex(),
            "hash": hashlib.sha256(raw).hexdigest(),
        }
    for name, (key, raw) in modules.items():
        entry = {
            "key": key.hex(),
            "hash": hashlib.sha256(raw).hexdigest(),
        }
        if name not in composite:
            entry["role"] = "part"
        else:
            entry["role"] = "module"
        manifest["modules"][name] = entry
    for sname, sdef in surfaces.items():
        manifest["surfaces"][sname] = {
            "native": sdef["native"],
            "token": sdef["token"].hex(),
            "hash": hashlib.sha256(sdef["block"]).hexdigest(),
            "facets": [fname for fname in sdef["facets"].keys()],
        }
    manifest["first_hash"] = hashlib.sha256(first).hexdigest()
    manifest["program_hash"] = hashlib.sha256(program_raw).hexdigest()
    (ROOT / "atomic_first_boot_manifest.json").write_text(
        json.dumps(manifest, indent=2) + "\n", encoding="ascii"
    )
    print("first", manifest["first_hash"], len(first))
    print("program", manifest["program_hash"], len(program_raw), len(program),
          "orchestrator (bare module tokens)")
    for name, value in manifest["actions"].items():
        print("action", name, value["hash"][:16], "size", len(actions[name]))
    for name, value in manifest["modules"].items():
        role = value.get("role", "?")
        print(role, name, value["key"][:16], "->", value["hash"][:16],
              "size", len(modules[name][1]))
    for name, value in manifest["surfaces"].items():
        print("surface", name, "native", value["native"], "->", value["hash"][:16],
              "facets", len(value["facets"]))
    print("natives", len(manifest["native"]))
    print("modules+parts", len(manifest["modules"]))


if __name__ == "__main__":
    main()
