#!/usr/bin/env python3
import json
import math
import re
from collections import Counter, defaultdict
import heapq
from pathlib import Path

ROOT = Path("/home/peter/dog/dog_dev")
PCB = ROOT / "circuit" / "circuit.kicad_pcb"
OUT = ROOT / "circuit" / "review" / "analysis.json"


def parse_sexpr(text):
    tokens = re.findall(r'"(?:\\.|[^"\\])*"|[()]|[^\s()]+', text)
    stack = []
    root = []
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


def fval(node, name, default=None):
    value = sval(node, name, default)
    if value is None:
        return default
    return float(value)


def point(node):
    return (float(node[1]), float(node[2]))


def at_xy(node):
    at = first(node, "at")
    if at is None:
        return (0.0, 0.0, 0.0)
    x = float(at[1])
    y = float(at[2])
    a = float(at[3]) if len(at) > 3 else 0.0
    return x, y, a


def transform(local, origin):
    x, y = local
    ox, oy, deg = origin
    r = math.radians(deg)
    # KiCad PCB coordinates use Y-down screen coordinates, so positive
    # footprint rotation is clockwise in the file coordinate system.
    return (
        ox + x * math.cos(r) + y * math.sin(r),
        oy - x * math.sin(r) + y * math.cos(r),
    )


def distance(a, b):
    return math.hypot(a[0] - b[0], a[1] - b[1])


def rect_distance(a, b):
    # Axis-aligned bounding-box distance. Touching/overlap returns 0.
    dx = max(a[0] - b[2], b[0] - a[2], 0.0)
    dy = max(a[1] - b[3], b[1] - a[3], 0.0)
    return math.hypot(dx, dy)


def poly_area(pts):
    if len(pts) < 3:
        return 0.0
    total = 0.0
    for (x1, y1), (x2, y2) in zip(pts, pts[1:] + pts[:1]):
        total += x1 * y2 - x2 * y1
    return abs(total) / 2.0


def line_bbox(a, b, w):
    r = w / 2.0
    return (min(a[0], b[0]) - r, min(a[1], b[1]) - r, max(a[0], b[0]) + r, max(a[1], b[1]) + r)


def ccw(a, b, c):
    return (c[1] - a[1]) * (b[0] - a[0]) > (b[1] - a[1]) * (c[0] - a[0])


def lines_intersect(a, b, c, d):
    return ccw(a, c, d) != ccw(b, c, d) and ccw(a, b, c) != ccw(a, b, d)


def point_to_segment_distance(p, a, b):
    vx = b[0] - a[0]
    vy = b[1] - a[1]
    wx = p[0] - a[0]
    wy = p[1] - a[1]
    denom = vx * vx + vy * vy
    if denom == 0:
        return distance(p, a)
    t = max(0.0, min(1.0, (wx * vx + wy * vy) / denom))
    proj = (a[0] + t * vx, a[1] + t * vy)
    return distance(p, proj)


def segment_distance(a, b, c, d):
    if lines_intersect(a, b, c, d):
        return 0.0
    return min(
        point_to_segment_distance(a, c, d),
        point_to_segment_distance(b, c, d),
        point_to_segment_distance(c, a, b),
        point_to_segment_distance(d, a, b),
    )


def pad_bbox(pad):
    x, y = pad["xy"]
    sx, sy = pad["size"]
    return (x - sx / 2, y - sy / 2, x + sx / 2, y + sy / 2)


def angle_deg(a, b):
    deg = math.degrees(math.atan2(b[1] - a[1], b[0] - a[0]))
    if deg < 0:
        deg += 180.0
    if deg >= 180.0:
        deg -= 180.0
    return deg


text = PCB.read_text()
tree = parse_sexpr(text)

version = sval(tree, "version")
generator_version = sval(tree, "generator_version")

layers = []
layers_node = first(tree, "layers")
if layers_node:
    for layer in children(layers_node):
        if len(layer) >= 3 and layer[1] in {"F.Cu", "B.Cu"}:
            layers.append(layer[1])

nets = set(n[1] for n in children(tree, "net") if len(n) > 1)

footprints = []
pads = []
silk_lines = []
silk_text = []
edge_lines = []
zones = []
gr_cu_lines = []

for fp in children(tree, "footprint"):
    x, y, rot = at_xy(fp)
    ref = None
    value = None
    hidden_ref = False
    for prop in children(fp, "property"):
        if len(prop) >= 3 and prop[1] == "Reference":
            ref = prop[2]
            hidden_ref = any(isinstance(c, list) and c and c[0] == "hide" for c in prop[3:])
        if len(prop) >= 3 and prop[1] == "Value":
            value = prop[2]
    fp_pads = []
    local_silk = []
    local_crtyd = []
    for pad in children(fp, "pad"):
        number = str(pad[1])
        kind = pad[2] if len(pad) > 2 else ""
        shape = pad[3] if len(pad) > 3 else ""
        px, py, _ = at_xy(pad)
        wx, wy = transform((px, py), (x, y, rot))
        size_node = first(pad, "size")
        drill_node = first(pad, "drill")
        layers_node2 = first(pad, "layers")
        size = (float(size_node[1]), float(size_node[2])) if size_node else (0.0, 0.0)
        drill = None
        if drill_node:
            nums = [float(v) for v in drill_node[1:] if re.match(r"^-?\d+(?:\.\d+)?$", v)]
            drill = nums[0] if nums else None
        net = sval(pad, "net")
        layer_values = layers_node2[1:] if layers_node2 else []
        p = {
            "ref": ref,
            "number": number,
            "kind": kind,
            "shape": shape,
            "xy": (wx, wy),
            "size": size,
            "drill": drill,
            "net": net,
            "layers": layer_values,
        }
        fp_pads.append(p)
        pads.append(p)
    for line in children(fp, "fp_line"):
        layer = sval(line, "layer")
        start = first(line, "start")
        end = first(line, "end")
        stroke = first(line, "stroke")
        width = fval(stroke, "width", 0.0) if stroke else 0.0
        if start and end:
            a = transform(point(start), (x, y, rot))
            b = transform(point(end), (x, y, rot))
            item = {"ref": ref, "layer": layer, "a": a, "b": b, "width": width}
            if layer in {"F.SilkS", "B.SilkS"}:
                silk_lines.append(item)
                local_silk.append(item)
            if layer in {"F.CrtYd", "B.CrtYd"}:
                local_crtyd.append(item)
    for rect in children(fp, "fp_rect"):
        layer = sval(rect, "layer")
        start = first(rect, "start")
        end = first(rect, "end")
        stroke = first(rect, "stroke")
        width = fval(stroke, "width", 0.0) if stroke else 0.0
        if start and end:
            x1, y1 = point(start)
            x2, y2 = point(end)
            pts = [transform(p, (x, y, rot)) for p in [(x1, y1), (x2, y1), (x2, y2), (x1, y2)]]
            for a, b in zip(pts, pts[1:] + pts[:1]):
                item = {"ref": ref, "layer": layer, "a": a, "b": b, "width": width}
                if layer in {"F.SilkS", "B.SilkS"}:
                    silk_lines.append(item)
                    local_silk.append(item)
                if layer in {"F.CrtYd", "B.CrtYd"}:
                    local_crtyd.append(item)
    bbox_pts = [p["xy"] for p in fp_pads]
    if local_crtyd:
        bbox_pts += [pt for ln in local_crtyd for pt in [ln["a"], ln["b"]]]
    if bbox_pts:
        bbox = (
            min(p[0] for p in bbox_pts),
            min(p[1] for p in bbox_pts),
            max(p[0] for p in bbox_pts),
            max(p[1] for p in bbox_pts),
        )
    else:
        bbox = (x, y, x, y)
    footprints.append({
        "ref": ref,
        "value": value,
        "at": (x, y, rot),
        "layer": sval(fp, "layer"),
        "hidden_ref": hidden_ref,
        "pads": fp_pads,
        "bbox": bbox,
    })

for line in children(tree, "gr_line"):
    layer = sval(line, "layer")
    start = first(line, "start")
    end = first(line, "end")
    stroke = first(line, "stroke")
    width = fval(stroke, "width", 0.0) if stroke else 0.0
    if start and end:
        item = {"layer": layer, "a": point(start), "b": point(end), "width": width, "net": sval(line, "net")}
        if layer == "Edge.Cuts":
            edge_lines.append(item)
        elif layer in {"F.SilkS", "B.SilkS"}:
            silk_lines.append(item)
        elif layer in {"F.Cu", "B.Cu"}:
            gr_cu_lines.append(item)

for txt in children(tree, "gr_text"):
    content = txt[1] if len(txt) > 1 else ""
    effects = first(txt, "effects")
    font = first(effects, "font") if effects else None
    size = first(font, "size") if font else None
    thickness = fval(font, "thickness", None) if font else None
    x, y, rot = at_xy(txt)
    sx = float(size[1]) if size else None
    sy = float(size[2]) if size else None
    silk_text.append({"text": content, "layer": sval(txt, "layer"), "at": (x, y, rot), "size": (sx, sy), "thickness": thickness})

segments = []
for seg in children(tree, "segment"):
    segments.append({
        "start": point(first(seg, "start")),
        "end": point(first(seg, "end")),
        "width": fval(seg, "width", 0.0),
        "layer": sval(seg, "layer"),
        "net": sval(seg, "net"),
    })

vias = list(children(tree, "via"))

for zone in children(tree, "zone"):
    polygon = first(zone, "polygon")
    pts_node = first(polygon, "pts") if polygon else None
    pts = [point(p) for p in children(pts_node, "xy")] if pts_node else []
    zones.append({"net": sval(zone, "net"), "layer": sval(zone, "layer"), "pts": pts, "area": poly_area(pts)})

for obj in pads + segments + gr_cu_lines + zones:
    net = obj.get("net")
    if net:
        nets.add(net)
nets = sorted(nets)

edge_pts = [pt for ln in edge_lines for pt in [ln["a"], ln["b"]]]
edge_bbox = (
    min(p[0] for p in edge_pts),
    min(p[1] for p in edge_pts),
    max(p[0] for p in edge_pts),
    max(p[1] for p in edge_pts),
) if edge_pts else None
board_w = edge_bbox[2] - edge_bbox[0]
board_h = edge_bbox[3] - edge_bbox[1]
board_area = board_w * board_h

pad_outside = []
for p in pads:
    bx = pad_bbox(p)
    if bx[0] < edge_bbox[0] - 1e-6 or bx[1] < edge_bbox[1] - 1e-6 or bx[2] > edge_bbox[2] + 1e-6 or bx[3] > edge_bbox[3] + 1e-6:
        pad_outside.append({"ref": p["ref"], "pad": p["number"], "net": p["net"], "xy": p["xy"], "bbox": bx})

footprint_outside = []
for fp in footprints:
    bx = fp["bbox"]
    if bx[0] < edge_bbox[0] - 1e-6 or bx[1] < edge_bbox[1] - 1e-6 or bx[2] > edge_bbox[2] + 1e-6 or bx[3] > edge_bbox[3] + 1e-6:
        footprint_outside.append({"ref": fp["ref"], "value": fp["value"], "bbox": bx})

zone_outside = []
for z in zones:
    out_pts = [p for p in z["pts"] if p[0] < edge_bbox[0] or p[0] > edge_bbox[2] or p[1] < edge_bbox[1] or p[1] > edge_bbox[3]]
    if out_pts:
        zone_outside.append({"net": z["net"], "layer": z["layer"], "area": z["area"], "outside_points": out_pts})

refs = {fp["ref"]: fp for fp in footprints}
connector_refs = {"H1", "H2", "B1A", "B1B", "B2A", "B2B", "J1", "J2", "J3", "J4", "J5", "J7"}
mount_refs = {"MH1", "MH2", "MH3", "MH4"}
top_count = sum(1 for fp in footprints if fp["layer"] == "F.Cu")
bottom_count = sum(1 for fp in footprints if fp["layer"] == "B.Cu")

h1 = refs.get("H1")
h2 = refs.get("H2")
bp_center = None
if h1 and h2:
    xs = []
    ys = []
    for ref in ["H1", "H2"]:
        for p in refs[ref]["pads"]:
            xs.append(p["xy"][0])
            ys.append(p["xy"][1])
    bp_center = ((min(xs) + max(xs)) / 2, (min(ys) + max(ys)) / 2)

servo_map = {}
for ref in ["J1", "J2", "J3", "J4"]:
    fp = refs.get(ref)
    if fp:
        servo_map[ref] = {"value": fp["value"], "at": fp["at"], "pads": [{"pad": p["number"], "xy": p["xy"], "net": p["net"]} for p in fp["pads"]]}

mounts = {}
for ref in sorted(mount_refs):
    fp = refs.get(ref)
    if fp and fp["pads"]:
        p = fp["pads"][0]
        mounts[ref] = {"xy": p["xy"], "drill": p["drill"], "size": p["size"], "net": p["net"]}

breakout_refs = ["B1A", "B1B", "B2A", "B2B"]
bp_refs = ["H1", "H2"]
breakout_issues = []
breakout_specs = []
for ref in breakout_refs:
    fp = refs.get(ref)
    if not fp:
        breakout_issues.append(f"{ref} 缺失")
        continue
    if len(fp["pads"]) != 20:
        breakout_issues.append(f"{ref} 焊盘数 {len(fp['pads'])} != 20")
    for p in fp["pads"]:
        breakout_specs.append((p["drill"], p["size"][0], p["size"][1]))

unbroken = []
row_misaligned = []
for bp_ref, a_ref, b_ref in [("H1", "B1A", "B1B"), ("H2", "B2A", "B2B")]:
    if bp_ref not in refs or a_ref not in refs or b_ref not in refs:
        continue
    bp_pads = {p["number"]: p for p in refs[bp_ref]["pads"]}
    a_pads = {p["number"]: p for p in refs[a_ref]["pads"]}
    b_pads = {p["number"]: p for p in refs[b_ref]["pads"]}
    for num in sorted(bp_pads, key=lambda n: int(n)):
        bp = bp_pads[num]
        aa = a_pads.get(num)
        bb = b_pads.get(num)
        if not aa or not bb or aa["net"] != bp["net"] or bb["net"] != bp["net"]:
            unbroken.append({"from": f"{bp_ref}:{num}", "net": bp["net"], "targets": [
                {"ref": f"{a_ref}:{num}", "net": aa["net"] if aa else None},
                {"ref": f"{b_ref}:{num}", "net": bb["net"] if bb else None},
            ]})
        if aa and abs(aa["xy"][0] - bp["xy"][0]) > 0.08:
            row_misaligned.append({"from": f"{bp_ref}:{num}", "to": f"{a_ref}:{num}", "dx": aa["xy"][0] - bp["xy"][0]})
        if bb and abs(bb["xy"][0] - bp["xy"][0]) > 0.08:
            row_misaligned.append({"from": f"{bp_ref}:{num}", "to": f"{b_ref}:{num}", "dx": bb["xy"][0] - bp["xy"][0]})

segment_graph = defaultdict(list)
for i, s in enumerate(segments):
    key1 = (s["net"], s["layer"], round(s["start"][0], 5), round(s["start"][1], 5))
    key2 = (s["net"], s["layer"], round(s["end"][0], 5), round(s["end"][1], 5))
    segment_graph[(s["net"], s["layer"])].append((i, s))

def polylines_for_group(group_segments):
    unused = set(range(len(group_segments)))
    lines = []
    endpoint_map = defaultdict(list)
    for idx, s in enumerate(group_segments):
        for pt in [s["start"], s["end"]]:
            endpoint_map[(round(pt[0], 5), round(pt[1], 5))].append(idx)
    while unused:
        idx = unused.pop()
        s = group_segments[idx]
        pts = [s["start"], s["end"]]
        changed = True
        while changed:
            changed = False
            for end_idx in [0, -1]:
                key = (round(pts[end_idx][0], 5), round(pts[end_idx][1], 5))
                for cand in list(endpoint_map[key]):
                    if cand not in unused:
                        continue
                    cs = group_segments[cand]
                    unused.remove(cand)
                    if distance(cs["start"], pts[end_idx]) < 1e-4:
                        other = cs["end"]
                    else:
                        other = cs["start"]
                    if end_idx == 0:
                        pts.insert(0, other)
                    else:
                        pts.append(other)
                    changed = True
                    break
                if changed:
                    break
        lines.append(pts)
    return lines

polyline_items = []
for (net, layer), group in segment_graph.items():
    group_segs = [s for _, s in group]
    for pts in polylines_for_group(group_segs):
        if len(pts) < 2:
            continue
        bends = max(0, len(pts) - 2)
        length = sum(distance(a, b) for a, b in zip(pts, pts[1:]))
        angles = [angle_deg(a, b) for a, b in zip(pts, pts[1:])]
        widths = sorted({s["width"] for s in group_segs})
        polyline_items.append({"net": net, "layer": layer, "points": pts, "bends": bends, "length": length, "angles": angles, "widths": widths})

bend_hist = Counter()
for p in polyline_items:
    if p["bends"] > 2:
        bend_hist[">2"] += 1
    else:
        bend_hist[str(p["bends"])] += 1

bad_angles = []
for p in polyline_items:
    for a, b, ang in zip(p["points"], p["points"][1:], p["angles"]):
        if min(abs(ang - x) for x in [0, 45, 90, 135]) > 0.2:
            bad_angles.append({"net": p["net"], "layer": p["layer"], "start": a, "end": b, "angle": ang})

width_counter = Counter((s["net"], s["width"]) for s in segments)
width_issues = []
for s in segments:
    net = s["net"] or ""
    expected = 0.25
    klass = "信号"
    if net in {"5V_SERVO", "5V_MCU", "3V3", "3V3_H1", "GND"}:
        expected = 0.5
        klass = "一般电源"
    if "SERVO" in net and net == "5V_SERVO":
        expected = 0.5
    if abs(s["width"] - expected) > 1e-6:
        width_issues.append({"net": net, "class": klass, "width": s["width"], "expected": expected, "start": s["start"], "end": s["end"]})

pwm_lengths = {}
for net in ["PA0_PWM_FL", "PA1_PWM_FR", "PA2_PWM_RL", "PA3_PWM_RR"]:
    total = sum(distance(s["start"], s["end"]) for s in segments if s["net"] == net)
    pwm_lengths[net] = total

def shortest_trace(net, start, end):
    def key(pt):
        return (round(pt[0], 5), round(pt[1], 5))
    graph = defaultdict(list)
    for s in segments:
        if s["net"] != net:
            continue
        a = key(s["start"])
        b = key(s["end"])
        w = distance(s["start"], s["end"])
        graph[a].append((b, w, s["end"]))
        graph[b].append((a, w, s["start"]))
    start_k = key(start)
    end_k = key(end)
    pq = [(0.0, start_k)]
    prev = {}
    dist = {start_k: 0.0}
    while pq:
        d, node = heapq.heappop(pq)
        if node == end_k:
            break
        if d != dist[node]:
            continue
        for nb, w, _pt in graph[node]:
            nd = d + w
            if nd < dist.get(nb, 1e100):
                dist[nb] = nd
                prev[nb] = node
                heapq.heappush(pq, (nd, nb))
    if end_k not in dist:
        return None
    nodes = []
    cur = end_k
    while cur != start_k:
        nodes.append(cur)
        cur = prev[cur]
    nodes.append(start_k)
    nodes.reverse()
    pts = [(x, y) for x, y in nodes]
    angles = [angle_deg(a, b) for a, b in zip(pts, pts[1:])]
    return {"length": dist[end_k], "points": pts, "bends": max(0, len(pts) - 2), "angles": angles}

pwm_pad_routes = {}
pwm_specs = [
    ("PA0_PWM_FL", "H1", "5", "J1", "3"),
    ("PA1_PWM_FR", "H1", "6", "J2", "3"),
    ("PA2_PWM_RL", "H1", "7", "J3", "3"),
    ("PA3_PWM_RR", "H1", "8", "J4", "3"),
]
for net, href, hpad, jref, jpad in pwm_specs:
    hfp = refs.get(href)
    jfp = refs.get(jref)
    if not hfp or not jfp:
        continue
    hp = next((p for p in hfp["pads"] if p["number"] == hpad), None)
    jp = next((p for p in jfp["pads"] if p["number"] == jpad), None)
    if hp and jp:
        route = shortest_trace(net, hp["xy"], jp["xy"])
        if route:
            route.update({"from": f"{href}:{hpad}", "to": f"{jref}:{jpad}"})
            pwm_pad_routes[net] = route

min_pad_clearances = []
for i, a in enumerate(pads):
    if a["ref"] in mount_refs:
        continue
    for b in pads[i + 1:]:
        if b["ref"] in mount_refs:
            continue
        if a["ref"] == b["ref"]:
            continue
        d = rect_distance(pad_bbox(a), pad_bbox(b))
        if d < 1.5:
            min_pad_clearances.append({"a": f"{a['ref']}:{a['number']}", "b": f"{b['ref']}:{b['number']}", "distance": d, "nets": [a["net"], b["net"]]})

component_clearances = []
min_component_clearance = None
for i, a in enumerate(footprints):
    for b in footprints[i + 1:]:
        d = rect_distance(a["bbox"], b["bbox"])
        item = {"a": a["ref"], "b": b["ref"], "distance": d, "a_bbox": a["bbox"], "b_bbox": b["bbox"]}
        if min_component_clearance is None or d < min_component_clearance["distance"]:
            min_component_clearance = item
        if d < 1.5:
            component_clearances.append(item)

drilled = [p for p in pads if p["drill"]]
min_hole_edge = None
hole_close = []
for i, a in enumerate(drilled):
    for b in drilled[i + 1:]:
        center = distance(a["xy"], b["xy"])
        edge = center - a["drill"] / 2.0 - b["drill"] / 2.0
        item = {"a": f"{a['ref']}:{a['number']}", "b": f"{b['ref']}:{b['number']}", "edge_distance": edge, "center_distance": center}
        if min_hole_edge is None or edge < min_hole_edge["edge_distance"]:
            min_hole_edge = item
        if edge < 0.5:
            hole_close.append(item)

annular = []
annular_violations = []
for p in drilled:
    ring = min(p["size"]) / 2.0 - p["drill"] / 2.0
    item = {"pad": f"{p['ref']}:{p['number']}", "ring": ring, "size": p["size"], "drill": p["drill"]}
    annular.append(item)
    if ring < 0.15:
        annular_violations.append(item)

silk_to_pad = []
silk_to_via = []
for ln in silk_lines:
    lb = line_bbox(ln["a"], ln["b"], ln["width"])
    for p in pads:
        pb = pad_bbox(p)
        d = rect_distance(lb, pb)
        if d < 0.3:
            silk_to_pad.append({"silk_layer": ln["layer"], "silk_ref": ln.get("ref"), "pad": f"{p['ref']}:{p['number']}", "distance": d})

silk_to_silk = []
for i, a in enumerate(silk_lines):
    for b in silk_lines[i + 1:]:
        if a["layer"] != b["layer"]:
            continue
        if a.get("ref") == b.get("ref") and a.get("ref") is not None:
            continue
        raw = segment_distance(a["a"], a["b"], b["a"], b["b"])
        d = raw - a["width"] / 2.0 - b["width"] / 2.0
        if d < 0.0:
            silk_to_silk.append({
                "layer": a["layer"],
                "distance": max(d, 0.0),
                "a_ref": a.get("ref"),
                "b_ref": b.get("ref"),
                "a_start": a["a"],
                "a_end": a["b"],
                "b_start": b["a"],
                "b_end": b["b"],
            })

track_silk = []
copper_items = []
for s in segments:
    copper_items.append({"kind": "segment", "net": s["net"], "layer": s["layer"], "a": s["start"], "b": s["end"], "width": s["width"]})
for ln in gr_cu_lines:
    copper_items.append({"kind": "gr_line", "net": ln["net"], "layer": ln["layer"], "a": ln["a"], "b": ln["b"], "width": ln["width"]})
for cu in copper_items:
    for sl in silk_lines:
        if cu["layer"] == "F.Cu" and sl["layer"] != "F.SilkS":
            continue
        if cu["layer"] == "B.Cu" and sl["layer"] != "B.SilkS":
            continue
        d = segment_distance(cu["a"], cu["b"], sl["a"], sl["b"]) - cu["width"] / 2.0 - sl["width"] / 2.0
        if d < 0.3:
            track_silk.append({
                "net": cu["net"],
                "copper_layer": cu["layer"],
                "silk_layer": sl["layer"],
                "distance": max(d, 0.0),
                "copper_start": cu["a"],
                "copper_end": cu["b"],
                "silk_ref": sl.get("ref"),
            })

text_size_issues = []
for t in silk_text:
    if t["size"][0] != 1.0 or t["size"][1] != 1.0 or t["thickness"] != 0.15:
        text_size_issues.append(t)

silk_required = {
    "USB 方向箭头": any("USB ->" in t["text"] for t in silk_text),
    "Pin1 三角": sum(1 for t in silk_text if "▲" in t["text"]) >= 1,
    "SG90 标签 FL/FR/RL/RR": all(any(t["text"] == lab for t in silk_text) for lab in ["FL", "FR", "RL", "RR"]),
    "G-V-S 顺序": sum(1 for t in silk_text if t["text"] == "G-V-S") >= 4,
    "5V_SERVO": any("5V_SERVO" in t["text"] for t in silk_text),
    "STAR GND": any("STAR" in t["text"] and "GND" in t["text"] for t in silk_text),
    "MPU6050 标签": any("MPU6050" in t["text"] for t in silk_text),
    "标题栏": any("Blue Pill SG90 Carrier" in t["text"] for t in silk_text),
}

gpio_redundant = [t for t in silk_text if re.search(r"\bP[ABC]\d+\b", t["text"])]

zone_area_total = sum(z["area"] for z in zones if z["layer"] == "B.Cu" and z["net"] == "GND")
zone_ratio_vs_edge = (zone_area_total / board_area * 100.0) if board_area else None
zone_ratio_vs_zone_polygon = None
if zone_area_total:
    zone_ratio_vs_zone_polygon = 100.0

report = {
    "version": version,
    "generator_version": generator_version,
    "layers": layers,
    "edge_bbox": edge_bbox,
    "board_size": [board_w, board_h],
    "board_area": board_area,
    "net_count": len(nets),
    "nets": nets,
    "footprint_count": len(footprints),
    "top_footprint_count": top_count,
    "bottom_footprint_count": bottom_count,
    "via_count": len(vias),
    "segment_count": len(segments),
    "gr_cu_line_count": len(gr_cu_lines),
    "blue_pill_center": bp_center,
    "board_center": ((edge_bbox[0] + edge_bbox[2]) / 2, (edge_bbox[1] + edge_bbox[3]) / 2),
    "blue_pill_offset": (bp_center[0] - (edge_bbox[0] + edge_bbox[2]) / 2, bp_center[1] - (edge_bbox[1] + edge_bbox[3]) / 2) if bp_center else None,
    "servos": servo_map,
    "mounts": mounts,
    "breakout_issues": breakout_issues,
    "breakout_specs_unique": sorted({tuple(round(x, 4) if x is not None else None for x in spec) for spec in breakout_specs}),
    "unbroken_pins": unbroken,
    "row_misaligned": row_misaligned,
    "pad_outside": pad_outside,
    "footprint_outside": footprint_outside,
    "zone_outside": zone_outside,
    "zones": zones,
    "zone_area_total": zone_area_total,
    "zone_ratio_vs_edge_percent": zone_ratio_vs_edge,
    "bend_histogram": dict(bend_hist),
    "traces_gt2_bends": [p for p in polyline_items if p["bends"] > 2],
    "bad_angles": bad_angles,
    "width_counter": {f"{k[0]}@{k[1]}": v for k, v in sorted(width_counter.items())},
    "width_issues": width_issues,
    "pwm_lengths": pwm_lengths,
    "pwm_pad_routes": pwm_pad_routes,
    "min_pad_clearance_violations": sorted(min_pad_clearances, key=lambda x: x["distance"])[:80],
    "min_component_clearance": min_component_clearance,
    "component_clearance_lt_1_5": sorted(component_clearances, key=lambda x: x["distance"])[:80],
    "min_hole_edge_distance": min_hole_edge,
    "hole_to_hole_edge_lt_0_5": sorted(hole_close, key=lambda x: x["edge_distance"])[:80],
    "min_annular_ring": min(annular, key=lambda x: x["ring"]) if annular else None,
    "annular_ring_lt_0_15": annular_violations,
    "silk_text": silk_text,
    "silk_required": silk_required,
    "gpio_redundant": gpio_redundant,
    "text_size_issues": text_size_issues,
    "silk_to_pad_lt_0_3": sorted(silk_to_pad, key=lambda x: x["distance"])[:80],
    "silk_to_silk_overlap": sorted(silk_to_silk, key=lambda x: x["distance"])[:80],
    "silk_to_via_lt_0_3": silk_to_via,
    "track_to_silk_lt_0_3": sorted(track_silk, key=lambda x: x["distance"])[:80],
}

OUT.write_text(json.dumps(report, ensure_ascii=False, indent=2))
print(json.dumps({
    "board_size": report["board_size"],
    "footprints": report["footprint_count"],
    "nets": report["net_count"],
    "vias": report["via_count"],
    "segments": report["segment_count"],
    "pad_outside": len(pad_outside),
    "footprint_outside": len(footprint_outside),
    "zone_outside": len(zone_outside),
    "silk_to_pad_lt_0_3": len(silk_to_pad),
}, ensure_ascii=False, indent=2))
