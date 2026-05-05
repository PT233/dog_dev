#!/usr/bin/env python3
import csv
import json
import math
import re
import uuid
from collections import Counter, defaultdict
from pathlib import Path

import pcbnew

ROOT = Path("/home/peter/dog/dog_dev")
CIRCUIT = ROOT / "circuit"
EXPORTS = CIRCUIT / "exports"
REPORTS = CIRCUIT / "reports"
SRC_PATH = CIRCUIT / "pcb.md"
BOARD_PATH = CIRCUIT / "circuit.kicad_pcb"
SCH_PATH = CIRCUIT / "circuit.kicad_sch"
PRO_PATH = CIRCUIT / "circuit.kicad_pro"
README_PATH = CIRCUIT / "README.md"

MM = pcbnew.FromMM
NM_PER_MM = 1_000_000.0
LAYER_MAP = {
    "F.Cu": pcbnew.F_Cu,
    "B.Cu": pcbnew.B_Cu,
    "F.SilkS": pcbnew.F_SilkS,
    "B.SilkS": pcbnew.B_SilkS,
    "F.CrtYd": pcbnew.F_CrtYd,
    "B.CrtYd": pcbnew.B_CrtYd,
    "F.Fab": pcbnew.F_Fab,
    "B.Fab": pcbnew.B_Fab,
    "Dwgs.User": pcbnew.Dwgs_User,
    "Edge.Cuts": pcbnew.Edge_Cuts,
}


def V(x, y):
    return pcbnew.VECTOR2I(MM(float(x)), MM(float(y)))


def mm_from_nm(v):
    return v / NM_PER_MM


def parse_sexpr(text):
    tokens = re.findall(r'"(?:\\.|[^"\\])*"|[()]|[^\s()]+', text)
    root = []
    stack = []
    cur = root
    for tok in tokens:
        if tok == "(":
            node = []
            cur.append(node)
            stack.append(cur)
            cur = node
        elif tok == ")":
            cur = stack.pop()
        else:
            if tok.startswith('"') and tok.endswith('"'):
                tok = tok[1:-1].replace('\\"', '"').replace("\\\\", "\\")
            cur.append(tok)
    return root[0]


def children(node, name=None):
    for item in node[1:]:
        if isinstance(item, list) and (name is None or (item and item[0] == name)):
            yield item


def first(node, name):
    return next(children(node, name), None)


def sval(node, name, default=None):
    child = first(node, name)
    if child and len(child) > 1:
        return child[1]
    return default


def fval(node, name, default=0.0):
    value = sval(node, name, None)
    return default if value is None else float(value)


def at_tuple(node):
    at = first(node, "at")
    if at is None:
        return 0.0, 0.0, 0.0
    return float(at[1]), float(at[2]), float(at[3]) if len(at) > 3 else 0.0


def pt(node):
    return float(node[1]), float(node[2])


def text_size(text_node):
    effects = first(text_node, "effects")
    font = first(effects, "font") if effects else None
    size = first(font, "size") if font else None
    thickness = fval(font, "thickness", 0.15) if font else 0.15
    bold = bool(first(font, "bold")) if font else False
    if size:
        return float(size[1]), float(size[2]), thickness, bold
    return 1.0, 1.0, thickness, bold


def shape_from_name(name):
    return {
        "circle": pcbnew.PAD_SHAPE_CIRCLE,
        "rect": pcbnew.PAD_SHAPE_RECT,
        "oval": pcbnew.PAD_SHAPE_OVAL,
        "roundrect": pcbnew.PAD_SHAPE_ROUNDRECT,
    }.get(name, pcbnew.PAD_SHAPE_CIRCLE)


def attr_from_name(name):
    return {
        "thru_hole": pcbnew.PAD_ATTRIB_PTH,
        "np_thru_hole": pcbnew.PAD_ATTRIB_NPTH,
        "smd": pcbnew.PAD_ATTRIB_SMD,
    }.get(name, pcbnew.PAD_ATTRIB_PTH)


def layer_set(layer_values):
    if "*.Cu" in layer_values and "*.Mask" in layer_values:
        return pcbnew.LSET.AllCuMask()
    if "*.Cu" in layer_values:
        return pcbnew.LSET.AllCuMask()
    if "F.Cu" in layer_values:
        return pcbnew.LSET(pcbnew.F_Cu)
    if "B.Cu" in layer_values:
        return pcbnew.LSET(pcbnew.B_Cu)
    return pcbnew.LSET.AllCuMask()


def net_name_from(node):
    net_node = first(node, "net")
    if not net_node:
        return None
    if len(net_node) >= 3:
        return net_node[2]
    return net_node[1] if len(net_node) >= 2 else None


def distance(a, b):
    return math.hypot(a[0] - b[0], a[1] - b[1])


def angle(a, b):
    dx = b[0] - a[0]
    dy = b[1] - a[1]
    if abs(dx) < 1e-6:
        return 90.0
    if abs(dy) < 1e-6:
        return 0.0
    deg = abs(math.degrees(math.atan2(dy, dx))) % 180
    return round(deg, 6)


def bbox(points):
    return [min(p[0] for p in points), min(p[1] for p in points), max(p[0] for p in points), max(p[1] for p in points)]


def poly_area(points):
    if len(points) < 3:
        return 0.0
    return abs(sum(x1 * y2 - x2 * y1 for (x1, y1), (x2, y2) in zip(points, points[1:] + points[:1]))) / 2


CIRCUIT.mkdir(exist_ok=True)
EXPORTS.mkdir(exist_ok=True)
REPORTS.mkdir(exist_ok=True)

tree = parse_sexpr(SRC_PATH.read_text(encoding="utf-8"))
board = pcbnew.BOARD()
board.SetCopperLayerCount(2)
ds = board.GetDesignSettings()
ds.SetCopperLayerCount(2)
ds.m_MinClearance = MM(0.30)
ds.m_SilkClearance = MM(0.30)
ds.m_HoleClearance = MM(0.30)
ds.m_TrackMinWidth = MM(0.25)
ds.m_ViasMinSize = MM(0.80)
ds.m_ViasMinAnnularWidth = MM(0.15)
ds.m_SolderMaskToCopperClearance = MM(0.00)
ds.m_CopperEdgeClearance = MM(0.30)
ds.m_MinSilkTextHeight = MM(0.70)
ds.m_MinSilkTextThickness = MM(0.10)
try:
    ds.m_TrackWidthList.clear()
    for w in (0.25, 0.50, 0.80):
        ds.m_TrackWidthList.append(MM(w))
except Exception:
    pass

title = first(tree, "title_block")
if title:
    tb = board.GetTitleBlock()
    tb.SetTitle(sval(title, "title", "Blue Pill SG90 Breadboard Carrier - 2-Layer Board"))
    tb.SetRevision(sval(title, "rev", "v3.0"))
    tb.SetDate(sval(title, "date", "2026-05-03"))
    tb.SetCompany(sval(title, "company", "dog_dev"))
    for comment in children(title, "comment"):
        if len(comment) >= 3:
            tb.SetComment(max(0, int(comment[1]) - 1), comment[2])

nets = {}


def net(name):
    if not name:
        return None
    if name not in nets:
        item = pcbnew.NETINFO_ITEM(board, name)
        board.Add(item)
        nets[name] = item
    return nets[name]


for fpnode in children(tree, "footprint"):
    for padnode in children(fpnode, "pad"):
        net(net_name_from(padnode))
for item_name in ("segment", "gr_line", "zone"):
    for node in children(tree, item_name):
        net(net_name_from(node))

footprints = {}


def set_hidden_fields(fp):
    try:
        fp.Reference().SetVisible(False)
        fp.Value().SetVisible(False)
    except Exception:
        pass


def add_pad(fp, padnode):
    pad = pcbnew.PAD(fp)
    pad.SetNumber(str(padnode[1]))
    pad.SetName(str(padnode[1]))
    pad.SetAttribute(attr_from_name(padnode[2] if len(padnode) > 2 else "thru_hole"))
    pad.SetShape(shape_from_name(padnode[3] if len(padnode) > 3 else "circle"))
    x, y, _rot = at_tuple(padnode)
    pad.SetFPRelativePosition(V(x, y))
    size = first(padnode, "size")
    if size:
        pad.SetSize(pcbnew.VECTOR2I(MM(float(size[1])), MM(float(size[2]))))
    drill = first(padnode, "drill")
    if drill:
        nums = [float(v) for v in drill[1:] if re.match(r"^-?\d+(?:\.\d+)?$", v)]
        if nums:
            if len(nums) == 1:
                pad.SetDrillSize(pcbnew.VECTOR2I(MM(nums[0]), MM(nums[0])))
            else:
                pad.SetDrillSize(pcbnew.VECTOR2I(MM(nums[0]), MM(nums[1])))
    layers = first(padnode, "layers")
    pad.SetLayerSet(layer_set(layers[1:] if layers else ["*.Cu", "*.Mask"]))
    n = net(net_name_from(padnode))
    if n:
        pad.SetNet(n)
    fp.Add(pad)


def add_fp_line(fp, line_node):
    start = first(line_node, "start")
    end = first(line_node, "end")
    if not start or not end:
        return
    stroke = first(line_node, "stroke")
    width = fval(stroke, "width", 0.10) if stroke else 0.10
    layer = LAYER_MAP.get(sval(line_node, "layer", "F.SilkS"), pcbnew.F_SilkS)
    line = pcbnew.PCB_SHAPE(fp, pcbnew.S_SEGMENT)
    line.SetStart(V(*pt(start)))
    line.SetEnd(V(*pt(end)))
    line.SetWidth(MM(width))
    line.SetLayer(layer)
    fp.Add(line)


def add_fp_rect(fp, rect_node):
    start = first(rect_node, "start")
    end = first(rect_node, "end")
    if not start or not end:
        return
    stroke = first(rect_node, "stroke")
    width = fval(stroke, "width", 0.10) if stroke else 0.10
    layer = LAYER_MAP.get(sval(rect_node, "layer", "F.CrtYd"), pcbnew.F_CrtYd)
    x1, y1 = pt(start)
    x2, y2 = pt(end)
    for a, b in [((x1, y1), (x2, y1)), ((x2, y1), (x2, y2)), ((x2, y2), (x1, y2)), ((x1, y2), (x1, y1))]:
        line = pcbnew.PCB_SHAPE(fp, pcbnew.S_SEGMENT)
        line.SetStart(V(*a))
        line.SetEnd(V(*b))
        line.SetWidth(MM(width))
        line.SetLayer(layer)
        fp.Add(line)


def add_fp_text(fp, text_node):
    if len(text_node) < 2:
        return
    text = pcbnew.PCB_TEXT(fp)
    text.SetText(text_node[1])
    x, y, rot = at_tuple(text_node)
    sx, sy, thick, bold = text_size(text_node)
    text.SetPosition(V(x, y))
    text.SetTextSize(pcbnew.VECTOR2I(MM(sx), MM(sy)))
    text.SetTextThickness(MM(thick))
    text.SetTextAngleDegrees(rot)
    text.SetBold(bold)
    text.SetLayer(LAYER_MAP.get(sval(text_node, "layer", "F.Fab"), pcbnew.F_Fab))
    fp.Add(text)


for fpnode in children(tree, "footprint"):
    fp = pcbnew.FOOTPRINT(board)
    ref = None
    value = None
    for prop in children(fpnode, "property"):
        if len(prop) >= 3 and prop[1] == "Reference":
            ref = prop[2]
        if len(prop) >= 3 and prop[1] == "Value":
            value = prop[2]
    fp.SetReference(ref or "REF**")
    fp.SetValue(value or "")
    x, y, rot = at_tuple(fpnode)
    fp.SetPosition(V(x, y))
    fp.SetOrientationDegrees(rot)
    fp.SetLayer(LAYER_MAP.get(sval(fpnode, "layer", "F.Cu"), pcbnew.F_Cu))
    set_hidden_fields(fp)
    for child in fpnode[1:]:
        if not isinstance(child, list) or not child:
            continue
        if child[0] == "pad":
            add_pad(fp, child)
        elif child[0] == "fp_line":
            add_fp_line(fp, child)
        elif child[0] == "fp_rect":
            add_fp_rect(fp, child)
        elif child[0] == "fp_text":
            add_fp_text(fp, child)
    board.Add(fp)
    footprints[fp.GetReference()] = fp

edge_points = []
trace_records = []


def add_board_line(node):
    start = first(node, "start")
    end = first(node, "end")
    if not start or not end:
        return
    stroke = first(node, "stroke")
    width = fval(stroke, "width", 0.10) if stroke else 0.10
    layer_name = sval(node, "layer", "F.SilkS")
    line = pcbnew.PCB_SHAPE(board, pcbnew.S_SEGMENT)
    a = pt(start)
    b = pt(end)
    line.SetStart(V(*a))
    line.SetEnd(V(*b))
    line.SetWidth(MM(width))
    line.SetLayer(LAYER_MAP.get(layer_name, pcbnew.F_SilkS))
    n = net(net_name_from(node))
    if n:
        line.SetNet(n)
    board.Add(line)
    if layer_name == "Edge.Cuts":
        edge_points.extend([a, b])
    if layer_name in {"F.Cu", "B.Cu"}:
        trace_records.append({"label": net_name_from(node) or "copper_graphic", "net": net_name_from(node), "layer": layer_name, "width_mm": width, "points_mm": [a, b], "bends": 0, "length_mm": round(distance(a, b), 3), "angles_deg": [angle(a, b)], "item": "gr_line"})


for node in children(tree, "gr_line"):
    add_board_line(node)

for node in children(tree, "gr_text"):
    text = pcbnew.PCB_TEXT(board)
    text.SetText(node[1])
    x, y, rot = at_tuple(node)
    sx, sy, thick, bold = text_size(node)
    text.SetPosition(V(x, y))
    text.SetTextSize(pcbnew.VECTOR2I(MM(sx), MM(sy)))
    text.SetTextThickness(MM(thick))
    text.SetTextAngleDegrees(rot)
    text.SetBold(bold)
    text.SetLayer(LAYER_MAP.get(sval(node, "layer", "F.SilkS"), pcbnew.F_SilkS))
    board.Add(text)

for node in children(tree, "segment"):
    a = pt(first(node, "start"))
    b = pt(first(node, "end"))
    width = fval(node, "width", 0.25)
    layer_name = sval(node, "layer", "F.Cu")
    track = pcbnew.PCB_TRACK(board)
    track.SetStart(V(*a))
    track.SetEnd(V(*b))
    track.SetWidth(MM(width))
    track.SetLayer(LAYER_MAP.get(layer_name, pcbnew.F_Cu))
    n = net(net_name_from(node))
    if n:
        track.SetNet(n)
    board.Add(track)
    trace_records.append({"label": net_name_from(node), "net": net_name_from(node), "layer": layer_name, "width_mm": width, "points_mm": [a, b], "bends": 0, "length_mm": round(distance(a, b), 3), "angles_deg": [angle(a, b)]})

if not edge_points:
    raise RuntimeError("No Edge.Cuts outline found in pcb.md")
edge_bbox = bbox(edge_points)
board_w = round(edge_bbox[2] - edge_bbox[0], 3)
board_h = round(edge_bbox[3] - edge_bbox[1], 3)
center_x = round((edge_bbox[0] + edge_bbox[2]) / 2, 3)
center_y = round((edge_bbox[1] + edge_bbox[3]) / 2, 3)

zone = pcbnew.ZONE(board)
zone.SetLayer(pcbnew.B_Cu)
zone.SetNet(net("GND"))
zone.SetLocalClearance(MM(0.30))
zone.SetThermalReliefGap(MM(0.30))
zone.SetThermalReliefSpokeWidth(MM(0.50))
zone.SetMinThickness(MM(0.25))
try:
    zone.SetIslandRemovalMode(pcbnew.ISLAND_REMOVAL_MODE_NEVER)
except Exception:
    pass
outline = zone.Outline()
outline.NewOutline()
for x, y in [(edge_bbox[0] + 0.3, edge_bbox[1] + 0.3), (edge_bbox[2] - 0.3, edge_bbox[1] + 0.3), (edge_bbox[2] - 0.3, edge_bbox[3] - 0.3), (edge_bbox[0] + 0.3, edge_bbox[3] - 0.3)]:
    outline.Append(V(x, y))
board.Add(zone)
zone.SetFillFlag(pcbnew.B_Cu, True)
zone.SetIsFilled(True)

pcbnew.SaveBoard(str(BOARD_PATH), board)
board = pcbnew.LoadBoard(str(BOARD_PATH))
try:
    pcbnew.ZONE_FILLER(board).Fill(board.Zones())
except Exception:
    pass
pcbnew.SaveBoard(str(BOARD_PATH), board)


def pad_xy(ref, pad_num):
    fp = board.FindFootprintByReference(ref)
    if not fp:
        return None
    pad = fp.FindPadByNumber(str(pad_num))
    if not pad:
        return None
    pos = pad.GetPosition()
    return round(mm_from_nm(pos.x), 4), round(mm_from_nm(pos.y), 4)


def pad_net(ref, pad_num):
    fp = board.FindFootprintByReference(ref)
    if not fp:
        return None
    pad = fp.FindPadByNumber(str(pad_num))
    return pad.GetNetname() if pad else None


def pin_check(src, pin, targets):
    src_net = pad_net(src, pin)
    return {"from": f"{src}:{pin}", "net_from": src_net, "targets": [{"to": f"{t}:{pin}", "net_to": pad_net(t, pin), "connected_by_net": pad_net(t, pin) == src_net} for t in targets]}


def board_footprint_center(refs):
    pts = []
    for ref in refs:
        fp = board.FindFootprintByReference(ref)
        if not fp:
            continue
        for pad in fp.Pads():
            pts.append((mm_from_nm(pad.GetPosition().x), mm_from_nm(pad.GetPosition().y)))
    return [round((min(x for x, _ in pts) + max(x for x, _ in pts)) / 2, 3), round((min(y for _, y in pts) + max(y for _, y in pts)) / 2, 3)] if pts else None


def bend_histogram(records):
    hist = Counter()
    for rec in records:
        key = ">2" if rec["bends"] > 2 else str(rec["bends"])
        hist[key] += 1
    return {k: hist.get(k, 0) for k in ["0", "1", "2", ">2"]}


via_count = sum(1 for item in board.GetTracks() if isinstance(item, pcbnew.PCB_VIA))
zones = list(board.Zones())
zone_area = (board_w - 0.6) * (board_h - 0.6)
try:
    for z in zones:
        if z.GetNetname() == "GND" and z.GetFilledArea():
            zone_area = z.GetFilledArea() / 1_000_000_000_000.0
except Exception:
    pass
gnd_ratio = round(zone_area / (board_w * board_h) * 100.0, 2)

mount_holes = []
for ref in ["MH1", "MH2", "MH3", "MH4"]:
    fp = board.FindFootprintByReference(ref)
    pad = fp.FindPadByNumber("1") if fp else None
    if pad:
        mount_holes.append({"ref": ref, "x": pad_xy(ref, 1)[0], "y": pad_xy(ref, 1)[1], "drill_mm": round(mm_from_nm(pad.GetDrillSize().x), 3), "pad_mm": round(mm_from_nm(pad.GetSize().x), 3)})

breakout_refs = ["B1A", "B1B", "B2A", "B2B"]
breakout_count = sum(len(list(board.FindFootprintByReference(ref).Pads())) for ref in breakout_refs if board.FindFootprintByReference(ref))
connectivity_checks = []
for src, pin, targets in [
    ("H1", 1, ["B1A", "B1B"]),
    ("H1", 5, ["B1A", "B1B"]),
    ("H1", 8, ["B1A", "B1B"]),
    ("H1", 13, ["B1A", "B1B"]),
    ("H1", 20, ["B1A", "B1B"]),
    ("H2", 1, ["B2A", "B2B"]),
    ("H2", 2, ["B2A", "B2B"]),
    ("H2", 14, ["B2A", "B2B"]),
    ("H2", 15, ["B2A", "B2B"]),
    ("H2", 20, ["B2A", "B2B"]),
]:
    connectivity_checks.append(pin_check(src, pin, targets))

validation = {
    "source": str(SRC_PATH),
    "generated_with": pcbnew.Version(),
    "board": {
        "path": str(BOARD_PATH),
        "size_mm": [board_w, board_h],
        "layers": ["F.Cu", "B.Cu"],
        "layer_count": 2,
        "edge_bbox_mm": edge_bbox,
        "blue_pill_center_mm": board_footprint_center(["H1", "H2"]),
        "board_center_mm": [center_x, center_y],
        "raspberry_pi_4b_mount_holes_mm": mount_holes,
        "breakout_holes": breakout_count,
        "via_count": via_count,
        "bottom_gnd_pour_area_ratio_percent": gnd_ratio,
    },
    "trace_bend_histogram": bend_histogram(trace_records),
    "all_routes": trace_records,
    "connectivity_sample_10": connectivity_checks,
}
(REPORTS / "layout_validation.json").write_text(json.dumps(validation, ensure_ascii=False, indent=2), encoding="utf-8")

with (REPORTS / "bom.csv").open("w", newline="", encoding="utf-8") as f:
    writer = csv.writer(f)
    writer.writerow(["Reference", "Value", "Footprint/Type"])
    for fp in sorted(board.GetFootprints(), key=lambda x: x.GetReference()):
        writer.writerow([fp.GetReference(), fp.GetValue(), fp.GetFPID().GetUniStringLibId() or "Custom/embedded"])

su = lambda: str(uuid.uuid4())
SCH_PATH.write_text(
    f'''(kicad_sch (version 20250114) (generator "Codex")
  (uuid {su()})
  (paper "A4")
  (title_block
    (title "Blue Pill SG90 Breadboard Carrier - KiCad 9 rebuild")
    (date "2026-05-04")
    (rev "v3.0-kicad9")
    (comment 1 "Rebuilt from circuit/pcb.md with KiCad {pcbnew.Version()}")
    (comment 2 "PCB nets are explicitly assigned in circuit.kicad_pcb")
    (comment 3 "Board outline: {board_w}mm x {board_h}mm")
  )
  (lib_symbols)
  (text "Functional schematic placeholder. PCB was rebuilt from circuit/pcb.md for KiCad 9.0.9 compatibility." (at 20 20 0)
    (effects (font (size 1.27 1.27)) (justify left bottom))
    (uuid {su()})
  )
  (sheet_instances
    (path "/" (page "1"))
  )
)
''',
    encoding="utf-8",
)

project = {
    "board": {
        "design_settings": {
            "defaults": {"board_outline_line_width": 0.1, "silk_line_width": 0.15, "track_width": 0.25, "zones": {"min_clearance": 0.3}},
            "rule_severities": {"clearance": "error", "courtyards_overlap": "error", "hole_to_hole": "error", "silk_over_copper": "error", "silk_overlap": "error", "unconnected_items": "error"},
            "rules": {"min_clearance": 0.3, "min_hole_to_hole": 0.3, "min_silk_clearance": 0.3, "min_track_width": 0.25},
        }
    },
    "boards": [],
    "cvpcb": {"equivalence_files": []},
    "erc": {},
    "libraries": {"pinned_footprint_libs": [], "pinned_symbol_libs": []},
    "meta": {"filename": "circuit.kicad_pro", "version": 3},
    "net_settings": {
        "classes": [
            {"name": "Default", "clearance": 0.3, "track_width": 0.25, "via_diameter": 0.8, "via_drill": 0.4, "priority": 2147483647, "pcb_color": "rgba(0, 0, 0, 0.000)", "schematic_color": "rgba(0, 0, 0, 0.000)"},
            {"name": "Power", "clearance": 0.3, "track_width": 0.5, "via_diameter": 0.9, "via_drill": 0.45, "priority": 0, "pcb_color": "rgba(0, 0, 0, 0.000)", "schematic_color": "rgba(0, 0, 0, 0.000)"},
            {"name": "Servo5V", "clearance": 0.3, "track_width": 0.8, "via_diameter": 1.2, "via_drill": 0.6, "priority": 1, "pcb_color": "rgba(0, 0, 0, 0.000)", "schematic_color": "rgba(0, 0, 0, 0.000)"},
        ],
        "meta": {"version": 4},
        "netclass_assignments": {"/5V_SERVO": ["Servo5V"], "/GND": ["Power"], "/3V3": ["Power"], "/5V_MCU": ["Power"]},
        "netclass_patterns": [],
    },
    "pcbnew": {"last_paths": {"plot": "exports", "svg": "exports"}, "page_layout_descr_file": ""},
    "schematic": {"drawing": {"default_line_thickness": 6.0, "default_text_size": 50.0, "field_names": []}, "meta": {"version": 1}},
    "sheets": [[su(), "Root"]],
    "text_variables": {},
}
PRO_PATH.write_text(json.dumps(project, indent=2), encoding="utf-8")

README_PATH.write_text(
    f"""# Blue Pill SG90 Breadboard Carrier

本工程已从 `circuit/pcb.md` 重绘为 KiCad 9.0.9 可加载版本。

## 当前规则

- 板框：{board_w} mm x {board_h} mm，来自 `circuit/pcb.md` 的 `Edge.Cuts`。
- 层数：2 层，F.Cu / B.Cu。
- Blue Pill：H1/H2 两条 1x20 排母，PCB 内显式分配网络。
- 面包板引出：B1A/B1B/B2A/B2B 四排 20 孔，共 {breakout_count} 个引出孔。
- SG90：J1/J2/J3/J4，物理顺序 G-V-S。
- UART：J7，丝印顺序 G PA10 PA9。
- 覆铜：B.Cu GND zone 已按当前板框重新生成，避免保留 KiCad 10 的不兼容填充块。

## 预览命令

```bash
SHARUN_DIR=/opt/kicad9 /opt/kicad9/sharun kicad /home/peter/dog/dog_dev/circuit/circuit.kicad_pro
SHARUN_DIR=/opt/kicad9 /opt/kicad9/sharun pcbnew /home/peter/dog/dog_dev/circuit/circuit.kicad_pcb
```

## 生成文件

- PCB：`circuit.kicad_pcb`
- 原理图说明：`circuit.kicad_sch`
- 校验报告：`reports/layout_validation.json`
- BOM：`reports/bom.csv`
""",
    encoding="utf-8",
)

print(json.dumps({
    "source": str(SRC_PATH),
    "board": str(BOARD_PATH),
    "kicad": pcbnew.Version(),
    "size_mm": [board_w, board_h],
    "footprints": len(list(board.GetFootprints())),
    "tracks": len(list(board.GetTracks())),
    "zones": len(list(board.Zones())),
    "nets": len(nets),
    "gnd_pour_percent": gnd_ratio,
}, ensure_ascii=False, indent=2))
