import hashlib
import json
import struct
from pathlib import Path

# POLICY: mods must stay low-level. If a behavior is expressible as a bare
# composition of existing tokens/facets, do NOT add a specialized native or
# alias action — place the composition (module/part/action recipe) instead.
# Example: string_*_var is forbidden; use stack string_append/string_backspace
# + var_read_payload/var_write_payload (clear = const zeros + var_write).
# Forbidden specialized: mega views, views_render, registry_*, string_*_var,
# mouse_button_pressed, views_pointer_*, views_cursor_*. Prefer apply/pick +
# table field ops + paint_*; cond_payload/name_*/platform gfx are mid-low OK.
# Pointer map: LMB title=drag view, LMB row=select; RMB title=close node,
# RMB row=open linked view + drag-out.

ROOT = Path(__file__).resolve().parent
PROGRAM_KEY = hashlib.sha256(b"#SingularityAtomicProgram").digest()
# Variable ids are plaintext UTF-8 strings (arbitrary binary still supported by vmvar).
# Prefer readable names so the editor shows id text instead of hex digests.
INPUT_VAR = b"atomic.editor.input"
CAM_X = b"atomic.cam.x"
CAM_Y = b"atomic.cam.y"
CAM_Z = b"atomic.cam.z"
GRAB_MX = b"atomic.cam.grab_mx"
GRAB_MY = b"atomic.cam.grab_my"
GRAB_CX = b"atomic.cam.grab_cx"
GRAB_CY = b"atomic.cam.grab_cy"
GRAB_VX = b"atomic.cam.grab_vx"
GRAB_VY = b"atomic.cam.grab_vy"
PAN_ACTIVE = b"atomic.cam.pan_active"
DRAG_ANCHORED = b"atomic.cam.drag_anchored"
PREV_LMB = b"atomic.input.prev_lmb"
PREV_RMB = b"atomic.input.prev_rmb"
VIEWS = b"atomic.views.table"


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
PART_CURSOR_ADD = part_key("views.cursor_add")
PART_CURSOR_DEC = part_key("views.cursor_dec")
PART_POINTER_LMB = part_key("views.apply_lmb")
PART_POINTER_RMB = part_key("views.apply_rmb")
PART_DRAG_END = part_key("views.drag_end")
PART_ENSURE = part_key("views.ensure")
PART_RENDER_DRAW = part_key("views.render_draw")
PART_GET_DRAG_XY = part_key("views.get_drag_xy")
PART_SET_DRAG_XY = part_key("views.set_drag_xy")
PART_ACTIVE_KEY = part_key("views.active_key")
PART_ACTIVE_CURSOR = part_key("views.active_cursor")
PART_EDITKEYS = part_key("editkeys")
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

ACTION_CORE = ("delete", "insert", "backspace", "clear")
ACTION_POINTER = (
    "pan", "click", "rmb", "drag", "end_drag",
    "begin_pan", "end_pan", "begin_drag_anchor", "begin_title_drag",
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
    """token -> friendly name for paint + name_prefix_find match."""
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
    """id may be any binary blob; encode as id_len[u32] + id + size[u32]."""
    return (t["var_set_payload"], u32(len(var_id)) + var_id + u32(size))


def views_var_payload(args=b""):
    """Encode views table var id as id_len[u32] + id + op args."""
    return u32(len(VIEWS)) + VIEWS + args


def views_call(t, op_name, args=b""):
    """Call a split views_* native. Payload is id_len+id + op args (no op code)."""
    return (t[op_name], views_var_payload(args))


def cond_action(t, key):
    return (t["cond_payload"], key)


def bare(key):
    """Place a logical token in the stream (empty payload). Resolves to firstchild block."""
    return (key, b"")


def mouse_edge(t, mask, prev_var):
    """Rising-edge of mouse_button_down without specialized mouse_button_pressed.
    Stack ends with u32 edge (1 if down&&!prev). Also writes prev=down.
    Sequence: down; dup; read prev; not; and; swap; write prev  -> edge
    prev_var is a plaintext id blob; write must use id_len+id (not legacy 32).
    """
    return [
        (t["mouse_button_down"], u32(mask)),
        (t["dup_u32"], b""),
        (t["var_read_payload"], prev_var),
        (t["not"], b""),
        (t["and"], b""),
        (t["swap_u32"], b""),
        (t["var_write_payload"], u32(len(prev_var)) + prev_var),
    ]


# ---------------------------------------------------------------------------
# Leaf action bodies (short reusable units; still logical firstchild blocks)
# ---------------------------------------------------------------------------

def build_begin_pan(t):
    return [
        (t["mouse_f"], b""),
        (t["var_write_payload"], u32(len(GRAB_MY)) + GRAB_MY),
        (t["var_write_payload"], u32(len(GRAB_MX)) + GRAB_MX),
        (t["var_read_payload"], CAM_X), (t["var_write_payload"], u32(len(GRAB_CX)) + GRAB_CX),
        (t["var_read_payload"], CAM_Y), (t["var_write_payload"], u32(len(GRAB_CY)) + GRAB_CY),
        (t["const_payload"], u32(1)), (t["var_write_payload"], u32(len(PAN_ACTIVE)) + PAN_ACTIVE),
    ]


def build_end_pan(t):
    return [
        (t["const_payload"], u32(0)), (t["var_write_payload"], u32(len(PAN_ACTIVE)) + PAN_ACTIVE),
    ]


def build_begin_drag_anchor(t):
    return [
        (t["mouse_f"], b""),
        (t["var_write_payload"], u32(len(GRAB_MY)) + GRAB_MY),
        (t["var_write_payload"], u32(len(GRAB_MX)) + GRAB_MX),
        bare(PART_GET_DRAG_XY),
        (t["var_write_payload"], u32(len(GRAB_VY)) + GRAB_VY),
        (t["var_write_payload"], u32(len(GRAB_VX)) + GRAB_VX),
        (t["const_payload"], u32(1)), (t["var_write_payload"], u32(len(DRAG_ANCHORED)) + DRAG_ANCHORED),
    ]


def build_begin_title_drag(t, action_keys):
    # Reset anchor flag then sample grab origin for the active dragging view.
    return [
        (t["const_payload"], u32(0)),
        (t["var_write_payload"], u32(len(DRAG_ANCHORED)) + DRAG_ANCHORED),
        bare(action_keys["begin_drag_anchor"]),
    ]


def build_end_drag(t):
    return [
        bare(PART_DRAG_END),
        (t["const_payload"], u32(0)), (t["var_write_payload"], u32(len(DRAG_ANCHORED)) + DRAG_ANCHORED),
    ]


def build_click(t, action_keys):
    # LMB edge: apply_lmb (title -> active+dragging; row -> select, no drag).
    # If a title drag started (dragging>=0), reset anchor and begin_drag_anchor.
    return [
        (t["world_mouse"], b""),
        bare(PART_POINTER_LMB),
        (t["drop_u32"], b""),
        views_call(t, "views_get_dragging"),
        (t["const_payload"], i32(-1)),
        (t["neq"], b""),
        (t["cond_payload"], action_keys["begin_title_drag"]),
    ]


def sel(t):
    # active_key facet + block_select_stack (no raw views-op)
    return [bare(PART_ACTIVE_KEY), (t["block_select_stack"], b"")]


def cur(t):
    return [bare(PART_ACTIVE_CURSOR), (t["block_offset_at_index"], b"")]


def build_editor_actions(t):
    # Only actions that bind payload/state — pure aliases of existing facets
    # (cursor_add/cursor_dec) are NOT re-wrapped as actions.
    # string edits: stack string ops + var_read/var_write (no string_*_var natives).
    return {
        "delete": make_block(sel(t) + cur(t) + [(t["block_delete"], b""), (t["jump_payload"], PROGRAM_KEY)]),
        "insert": make_block(sel(t) + cur(t) + [
            (t["var_read_payload"], INPUT_VAR), (t["name_prefix_find"], b""),
            (t["block_insert_stack"], b""),
            # clear input: write 256 zero bytes
            (t["const_payload"], bytes(256)), (t["var_write_payload"], u32(len(INPUT_VAR)) + INPUT_VAR),
            (t["jump_payload"], PROGRAM_KEY),
        ]),
        "backspace": make_block([
            (t["var_read_payload"], INPUT_VAR),
            (t["string_backspace"], u32(256)),
            (t["var_write_payload"], u32(len(INPUT_VAR)) + INPUT_VAR),
        ]),
        "clear": make_block([
            (t["const_payload"], bytes(256)), (t["var_write_payload"], u32(len(INPUT_VAR)) + INPUT_VAR),
        ]),
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
        (t["var_write_payload"], u32(len(CAM_X)) + CAM_X),
    ]


def part_pan_y(t):
    return [
        (t["var_read_payload"], GRAB_CY),
        (t["mouse_f"], b""), (t["swap_u32"], b""), (t["drop_u32"], b""),
        (t["var_read_payload"], GRAB_MY), (t["f32_sub"], b""),
        (t["var_read_payload"], CAM_Z), (t["f32_div"], b""),
        (t["f32_sub"], b""),
        (t["var_write_payload"], u32(len(CAM_Y)) + CAM_Y),
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
        bare(PART_SET_DRAG_XY),
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
        (t["var_write_payload"], u32(len(CAM_Z)) + CAM_Z),
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
        (t["var_write_payload"], u32(len(CAM_X)) + CAM_X),
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
        (t["var_write_payload"], u32(len(CAM_Y)) + CAM_Y),
    ]


def part_rmb_open(t):
    # compose: world_mouse + views.pointer_rmb facet + drop
    return [
        (t["world_mouse"], b""),
        bare(PART_POINTER_RMB),
        (t["drop_u32"], b""),
    ]


def part_status(t):
    return [
        (t["drawtext_screen"], static_text(20, 16, 0xff9da7b3, 16.0,
            b"modular | compose>specialized | cond opens payload | edge=down+prev")),
    ]


def part_typein(t):
    # typein = text_input + (var_read + string_append + var_write) + draw
    # no specialized string_append_var / drawtext_var_* wrappers
    return [
        (t["var_read_payload"], INPUT_VAR),
        (t["text_input"], b""),
        (t["string_append"], u32(256) + u32(256)),
        (t["var_write_payload"], u32(len(INPUT_VAR)) + INPUT_VAR),
        (t["screen_size"], b""), (t["i32_to_f32"], b""),
        (t["f32_const"], f32(52.0)), (t["f32_sub"], b""),
        (t["swap_u32"], b""), (t["drop_u32"], b""),
        (t["f32_const"], f32(20.0)), (t["swap_u32"], b""),
        (t["var_read_payload"], INPUT_VAR),
        (t["drawtext_xy_stack_screen"], struct.pack("<IfI", 0xffffffff, 17.0, 256)),
    ]


def part_match(t):
    # match label x = measure(input)+pad; show tag-graph path (name_lookup -> path[160])
    return [
        (t["screen_size"], b""), (t["i32_to_f32"], b""),
        (t["f32_const"], f32(52.0)), (t["f32_sub"], b""),
        (t["swap_u32"], b""), (t["drop_u32"], b""),
        (t["var_read_payload"], INPUT_VAR),
        (t["measure_text"], f32(17.0) + u32(256)),
        (t["f32_const"], f32(32.0)), (t["f32_add"], b""),
        (t["swap_u32"], b""),
        (t["var_read_payload"], INPUT_VAR), (t["name_prefix_find"], b""),
        (t["name_lookup"], b""),
        (t["drawtext_xy_stack_screen"], struct.pack("<IfI", 0xff73808c, 17.0, 160)),
    ]


def part_nav(t, action_keys):
    # Arrow keys exec existing views surface facets (no alias actions).
    return [
        (t["key_pressed"], u32(0x28)), cond_action(t, PART_CURSOR_ADD),
        (t["key_pressed"], u32(0x26)), cond_action(t, PART_CURSOR_DEC),
    ]


def part_editkeys(t, action_keys):
    return [
        (t["key_pressed"], u32(0x2e)), cond_action(t, action_keys["delete"]),
        (t["key_pressed"], u32(0x20)), cond_action(t, action_keys["insert"]),
        (t["key_pressed"], u32(0x09)), cond_action(t, action_keys["insert"]),
        (t["key_pressed"], u32(0x08)), cond_action(t, action_keys["backspace"]),
        (t["key_pressed"], u32(0x1b)), cond_action(t, action_keys["clear"]),
    ]


def part_click_on(t, action_keys):
    # edge = down && !prev; no specialized mouse_button_pressed native
    return mouse_edge(t, 1, PREV_LMB) + [cond_action(t, action_keys["click"])]


def part_rmb_on(t, action_keys):
    return mouse_edge(t, 2, PREV_RMB) + [cond_action(t, action_keys["rmb"])]


def part_pan_on(t, action_keys):
    return [
        (t["mouse_button_down"], u32(4)), cond_action(t, action_keys["pan"]),
        (t["mouse_button_down"], u32(4)), (t["not"], b""), cond_action(t, action_keys["end_pan"]),
    ]


def part_drag_on(t, action_keys):
    # Drag while LMB (title move) or RMB (row open drag-out) is held.
    return [
        (t["mouse_button_down"], u32(1)),
        (t["mouse_button_down"], u32(2)),
        (t["or"], b""),
        cond_action(t, action_keys["drag"]),
        (t["mouse_button_down"], u32(1)),
        (t["mouse_button_down"], u32(2)),
        (t["or"], b""),
        (t["not"], b""),
        cond_action(t, action_keys["end_drag"]),
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
    # RMB edge: apply_rmb (title -> close node; row -> open linked + dragging).
    # Only start drag anchor when a view is actually being dragged (row open).
    return [
        bare(PART_RMB_OPEN),
        views_call(t, "views_get_dragging"),
        (t["const_payload"], i32(-1)),
        (t["neq"], b""),
        (t["cond_payload"], action_keys["begin_title_drag"]),
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

def sample_views(t, op_name, args=b""):
    """One-instruction leaf: split views_* native + views var id + args."""
    return make_block([views_call(t, op_name, args)])


def build_native_surfaces(t):
    """Thin table + paint surfaces. Integrated pointer/cursor ops are recipes."""
    hit_args = f32(32.0) + f32(24.0) + u32(256)
    ensure_args = PROGRAM_KEY + f32(40.0) + f32(70.0)

    facets = {
        "ensure": (part_key("views.ensure"), sample_views(t, "views_ensure", ensure_args)),
        "pick": (part_key("views.pick"), sample_views(t, "views_pick", hit_args)),
        "apply_lmb": (part_key("views.apply_lmb"), sample_views(t, "views_apply_lmb", hit_args)),
        "apply_rmb": (part_key("views.apply_rmb"), sample_views(t, "views_apply_rmb", hit_args)),
        "open_key": (part_key("views.open_key"), sample_views(t, "views_open_key")),
        "select_row": (part_key("views.select_row"), sample_views(t, "views_select_row")),
        "set_active_drag": (part_key("views.set_active_drag"), sample_views(t, "views_set_active_drag")),
        "active_key": (part_key("views.active_key"), sample_views(t, "views_active_key")),
        "active_cursor": (part_key("views.active_cursor"), sample_views(t, "views_active_cursor")),
        "set_cursor_active": (part_key("views.set_cursor_active"), sample_views(t, "views_set_cursor_active")),
        "drag_end": (part_key("views.drag_end"), sample_views(t, "views_drag_end")),
        "get_drag_xy": (part_key("views.get_drag_xy"), sample_views(t, "views_get_drag_xy")),
        "set_drag_xy": (part_key("views.set_drag_xy"), sample_views(t, "views_set_drag_xy")),
    }
    surface_order = [
        "ensure", "pick", "apply_lmb", "apply_rmb", "open_key", "select_row",
        "set_active_drag", "active_key", "active_cursor", "set_cursor_active",
        "drag_end", "get_drag_xy", "set_drag_xy",
    ]
    views_surface = make_block([bare(facets[n][0]) for n in surface_order])

    render_leaf_key = part_key("views.render_draw")
    render_leaf = make_block([
        views_call(t, "views_paint_links"),
        views_call(t, "views_paint_titles"),
        views_call(t, "views_paint_rows"),
    ])
    paint_facets = {
        "paint_links": (part_key("views.paint_links"), sample_views(t, "views_paint_links")),
        "paint_titles": (part_key("views.paint_titles"), sample_views(t, "views_paint_titles")),
        "paint_rows": (part_key("views.paint_rows"), sample_views(t, "views_paint_rows")),
        "render_draw": (render_leaf_key, render_leaf),
    }
    paint_order = ["paint_links", "paint_titles", "paint_rows", "render_draw"]
    paint_surface = make_block([bare(paint_facets[n][0]) for n in paint_order])

    surfaces = {
        "views": {
            "native": "views_ensure",
            "token": t["views_ensure"],
            "block": views_surface,
            "facets": {n: facets[n] for n in surface_order},
        },
        "views_paint": {
            "native": "views_paint_rows",
            "token": t["views_paint_rows"],
            "block": paint_surface,
            "facets": paint_facets,
        },
    }
    return surfaces


def main():
    t = load_tokens()
    required = {
        "frame_begin", "frame_clear", "frame_end", "reexec", "camera_set_stack",
        "drawtext_screen", "drawtext_xy_stack_screen",
        "const_payload", "var_set_payload", "var_read_payload", "var_write_payload",
        "key_pressed", "cond_payload", "name_prefix_find", "name_lookup",
        "text_input", "string_append", "string_backspace",
        "block_insert_stack", "block_delete",
        "jump_payload", "mouse_f", "mouse_wheel", "mouse_button_down",
        "i32_to_f32", "i32_add", "i32_min", "i32_max", "eq",
        "f32_add", "f32_sub", "f32_mul", "f32_div", "f32_const", "f32_clamp",
        "world_mouse", "drop_u32", "swap_u32", "dup_u32",
        "views_paint_links", "views_paint_titles", "views_paint_rows",
        "views_ensure", "views_active_key", "views_active_cursor",
        "views_set_cursor_active", "views_drag_end",
        "views_get_drag_xy", "views_set_drag_xy",
        "views_apply_lmb", "views_apply_rmb", "views_pick", "views_open_key",
        "views_select_row", "views_set_active_drag", "views_get_dragging",
        "block_resolve", "block_instr_count", "instr_open_at",
        "block_select_stack", "block_offset_at_index", "measure_text", "not", "and", "or", "neq",
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
    actions["begin_title_drag"] = make_block(build_begin_title_drag(t, action_keys))
    actions["pan"] = make_block(build_pan(t, action_keys))
    actions["click"] = make_block(build_click(t, action_keys))
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
        # cursor_add/dec: composition (no specialized views_cursor_* natives)
        "cursor_add": (PART_CURSOR_ADD, make_block([
            views_call(t, "views_active_cursor"),
            (t["const_payload"], i32(1)), (t["i32_add"], b""),
            (t["const_payload"], i32(0)), (t["i32_max"], b""),
            # clamp high: resolve active key block, count instrs, min(cur, count)
            views_call(t, "views_active_key"),
            (t["block_resolve"], b""),
            (t["block_instr_count"], b""),
            (t["i32_min"], b""),
            views_call(t, "views_set_cursor_active"),
        ])),
        "cursor_dec": (PART_CURSOR_DEC, make_block([
            views_call(t, "views_active_cursor"),
            (t["const_payload"], i32(-1)), (t["i32_add"], b""),
            (t["const_payload"], i32(0)), (t["i32_max"], b""),
            views_call(t, "views_set_cursor_active"),
        ])),
    }

    # Thin orchestrator: glue + bare logical module tokens (no exec).
    program = [
        # Thin orchestrator: only frame glue + bare logical tokens (no specialized embeds).
        (t["frame_begin"], b""),
        bare(PART_ENSURE),          # views.ensure facet (not raw views-op soup)
        bare(MOD_CAMERA),
        (t["frame_clear"], u32(0xff11161b)),
        bare(MOD_INPUT),
        bare(MOD_ZOOM),
        bare(MOD_CAMERA),
        bare(PART_RENDER_DRAW),     # views.render_draw facet
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
        alloc_var(t, PREV_LMB, 4), alloc_var(t, PREV_RMB, 4),
        alloc_var(t, VIEWS, 2320),
        (t["const_payload"], f32(640.0)), (t["var_write_payload"], u32(len(CAM_X)) + CAM_X),
        (t["const_payload"], f32(360.0)), (t["var_write_payload"], u32(len(CAM_Y)) + CAM_Y),
        (t["const_payload"], f32(1.0)), (t["var_write_payload"], u32(len(CAM_Z)) + CAM_Z),
        (t["const_payload"], u32(0)), (t["var_write_payload"], u32(len(PAN_ACTIVE)) + PAN_ACTIVE),
        (t["const_payload"], u32(0)), (t["var_write_payload"], u32(len(DRAG_ANCHORED)) + DRAG_ANCHORED),
        (t["const_payload"], u32(0)), (t["var_write_payload"], u32(len(PREV_LMB)) + PREV_LMB),
        (t["const_payload"], u32(0)), (t["var_write_payload"], u32(len(PREV_RMB)) + PREV_RMB),
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
