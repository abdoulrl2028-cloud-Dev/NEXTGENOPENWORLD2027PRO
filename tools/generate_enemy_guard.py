#!/usr/bin/env python3
"""Original low-poly dock guard for Vera Costa.

Body parts are smooth meshes in joint-local space. The game poses them.
This is not a scanned person and not a placeholder made of boxes.
"""

import json
import math
import struct
import sys
from pathlib import Path


def normalize(value):
    mag = math.sqrt(value[0] * value[0] + value[1] * value[1] + value[2] * value[2])
    if mag < 1e-8:
        return (0.0, 1.0, 0.0)
    return (value[0] / mag, value[1] / mag, value[2] / mag)


def sub(a, b):
    return (a[0] - b[0], a[1] - b[1], a[2] - b[2])


def cross(a, b):
    return (
        a[1] * b[2] - a[2] * b[1],
        a[2] * b[0] - a[0] * b[2],
        a[0] * b[1] - a[1] * b[0],
    )


def dot(a, b):
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]


class Part:
    def __init__(self, name, color, roughness, metallic=0.0):
        self.name = name
        self.color = color
        self.roughness = roughness
        self.metallic = metallic
        self.positions = []
        self.normals = []
        self.uvs = []
        self.indices = []

    def add_vertex(self, position, normal, uv):
        self.positions.append(position)
        self.normals.append(normalize(normal))
        self.uvs.append(uv)
        return len(self.positions) - 1

    def add_tri(self, a, b, c):
        self.indices.extend((a, b, c))

    def flip_winding(self):
        for index in range(0, len(self.indices), 3):
            self.indices[index + 1], self.indices[index + 2] = (
                self.indices[index + 2],
                self.indices[index + 1],
            )

    def signed_volume(self):
        volume = 0.0
        for index in range(0, len(self.indices), 3):
            a = self.positions[self.indices[index]]
            b = self.positions[self.indices[index + 1]]
            c = self.positions[self.indices[index + 2]]
            volume += dot(a, cross(b, c))
        return volume / 6.0

    def add_ellipsoid(self, center, radius, lat=10, lon=14):
        cx, cy, cz = center
        rx, ry, rz = radius
        grid = []
        for lat_index in range(lat + 1):
            theta = math.pi * lat_index / lat
            ring = []
            y = math.cos(theta)
            radial = math.sin(theta)
            for lon_index in range(lon):
                phi = math.tau * lon_index / lon
                normal = (radial * math.cos(phi), y, radial * math.sin(phi))
                position = (cx + normal[0] * rx, cy + normal[1] * ry, cz + normal[2] * rz)
                ring.append(self.add_vertex(position, normal, (lon_index / lon, lat_index / lat)))
            grid.append(ring)
        for lat_index in range(lat):
            for lon_index in range(lon):
                nxt = (lon_index + 1) % lon
                a = grid[lat_index][lon_index]
                b = grid[lat_index + 1][lon_index]
                c = grid[lat_index + 1][nxt]
                d = grid[lat_index][nxt]
                self.add_tri(a, d, b)
                self.add_tri(b, d, c)

    def add_limb(self, length, radius_top, radius_bottom, segments=14):
        rings = (
            (0.0, radius_top),
            (-length * 0.45, (radius_top * 0.55 + radius_bottom * 0.45)),
            (-length, radius_bottom),
        )
        grid = []
        for ring_index, (y_pos, radius) in enumerate(rings):
            ring = []
            for step in range(segments):
                phi = math.tau * step / segments
                normal = (math.cos(phi), 0.0, math.sin(phi))
                position = (normal[0] * radius, y_pos, normal[2] * radius)
                ring.append(self.add_vertex(position, normal, (step / segments, ring_index / 2.0)))
            grid.append(ring)
        for ring_index in range(len(grid) - 1):
            for step in range(segments):
                nxt = (step + 1) % segments
                a = grid[ring_index][step]
                b = grid[ring_index + 1][step]
                c = grid[ring_index + 1][nxt]
                d = grid[ring_index][nxt]
                self.add_tri(a, b, d)
                self.add_tri(b, c, d)
        top_center = self.add_vertex((0.0, 0.0, 0.0), (0.0, 1.0, 0.0), (0.5, 0.0))
        bottom_center = self.add_vertex((0.0, -length, 0.0), (0.0, -1.0, 0.0), (0.5, 1.0))
        for step in range(segments):
            nxt = (step + 1) % segments
            self.add_tri(top_center, grid[0][nxt], grid[0][step])
            self.add_tri(bottom_center, grid[-1][step], grid[-1][nxt])

    def add_boot(self):
        def vertex(x_pos, y_pos, z_pos, normal):
            return self.add_vertex((x_pos, y_pos, z_pos), normal, (0.5, 0.5))

        ankle = 0.045
        sole = -0.08
        heel = -0.04
        toe = 0.16
        side = 0.055
        top = 0.02
        points = {
            "atl": vertex(-side, top, heel, (-1.0, 0.4, -0.2)),
            "atr": vertex(side, top, heel, (1.0, 0.4, -0.2)),
            "afl": vertex(-side * 0.7, top, toe * 0.35, (-0.6, 0.6, 0.5)),
            "afr": vertex(side * 0.7, top, toe * 0.35, (0.6, 0.6, 0.5)),
            "sbl": vertex(-side, sole, heel, (-0.4, -0.8, -0.3)),
            "sbr": vertex(side, sole, heel, (0.4, -0.8, -0.3)),
            "sfl": vertex(-side * 0.55, sole, toe, (-0.2, -0.7, 0.6)),
            "sfr": vertex(side * 0.55, sole, toe, (0.2, -0.7, 0.6)),
        }
        quads = (
            ("atl", "atr", "afr", "afl", (0.0, 1.0, 0.2)),
            ("sbl", "sfl", "sfr", "sbr", (0.0, -1.0, 0.0)),
            ("atl", "afl", "sfl", "sbl", (-1.0, 0.0, 0.0)),
            ("atr", "sbr", "sfr", "afr", (1.0, 0.0, 0.0)),
            ("afl", "afr", "sfr", "sfl", (0.0, 0.2, 1.0)),
            ("atr", "atl", "sbl", "sbr", (0.0, 0.0, -1.0)),
        )
        del ankle
        for a, b, c, d, outward in quads:
            pa = self.positions[points[a]]
            pb = self.positions[points[b]]
            if dot(cross(sub(pb, pa), sub(self.positions[points[c]], pa)), outward) < 0.0:
                b, d = d, b
            self.add_tri(points[a], points[b], points[c])
            self.add_tri(points[a], points[c], points[d])

    def add_vest_panel(self):
        columns = 6
        rows = 4
        grid = []
        for row in range(rows + 1):
            y_pos = 0.08 + 0.30 * row / rows
            line = []
            for column in range(columns + 1):
                x_pos = -0.15 + 0.30 * column / columns
                bulge = math.sin(math.pi * column / columns) * 0.035
                z_pos = 0.145 + bulge
                normal = normalize((x_pos * 0.2, 0.0, 1.0))
                line.append(self.add_vertex((x_pos, y_pos, z_pos), normal, (column / columns, row / rows)))
            grid.append(line)
        for row in range(rows):
            for column in range(columns):
                a = grid[row][column]
                b = grid[row][column + 1]
                c = grid[row + 1][column + 1]
                d = grid[row + 1][column]
                self.add_tri(a, b, c)
                self.add_tri(a, c, d)


def outward(part):
    if part.signed_volume() < 0.0:
        part.flip_winding()


def build_parts():
    skin = (0.67, 0.48, 0.36, 1.0)
    navy = (0.11, 0.15, 0.22, 1.0)
    red = (0.62, 0.13, 0.11, 1.0)
    hair = (0.09, 0.07, 0.06, 1.0)
    boot_color = (0.07, 0.07, 0.08, 1.0)
    eye = (0.08, 0.08, 0.09, 1.0)

    hips = Part("hips", navy, 0.72)
    hips.add_ellipsoid((0.0, 0.0, 0.0), (0.16, 0.11, 0.12), 8, 14)
    outward(hips)

    torso = Part("torso", navy, 0.68)
    torso.add_ellipsoid((0.0, 0.27, 0.0), (0.18, 0.28, 0.12), 10, 14)
    outward(torso)

    vest = Part("torso_vest", red, 0.42, 0.04)
    vest.add_vest_panel()

    head = Part("head", skin, 0.55)
    head.add_ellipsoid((0.0, 0.13, 0.0), (0.105, 0.125, 0.11), 10, 14)
    head.add_ellipsoid((0.0, 0.12, 0.09), (0.028, 0.04, 0.03), 6, 8)
    outward(head)

    hair_cap = Part("head_hair", hair, 0.8)
    hair_cap.add_ellipsoid((0.0, 0.18, -0.01), (0.112, 0.09, 0.115), 8, 14)
    outward(hair_cap)

    eyes = Part("head_eyes", eye, 0.25, 0.02)
    eyes.add_ellipsoid((-0.038, 0.15, 0.09), (0.016, 0.016, 0.012), 6, 8)
    eyes.add_ellipsoid((0.038, 0.15, 0.09), (0.016, 0.016, 0.012), 6, 8)
    outward(eyes)

    def arm(name, color, roughness):
        part = Part(name, color, roughness)
        part.add_limb(0.30 if "upper" in name else 0.27, 0.05 if "upper" in name else 0.042, 0.038)
        if name.startswith("forearm"):
            part.add_ellipsoid((0.0, -0.29, 0.0), (0.04, 0.045, 0.028), 6, 8)
        outward(part)
        return part

    def leg(name):
        part = Part(name, navy, 0.7)
        length = 0.46 if name.startswith("thigh") else 0.44
        radius_top = 0.075 if name.startswith("thigh") else 0.055
        radius_bottom = 0.055 if name.startswith("thigh") else 0.042
        part.add_limb(length, radius_top, radius_bottom)
        outward(part)
        return part

    def boot(name):
        part = Part(name, boot_color, 0.58)
        part.add_boot()
        return part

    parts = [
        hips,
        torso,
        vest,
        head,
        hair_cap,
        eyes,
        arm("upper_arm_l", navy, 0.68),
        arm("upper_arm_r", navy, 0.68),
        arm("forearm_l", skin, 0.55),
        arm("forearm_r", skin, 0.55),
        leg("thigh_l"),
        leg("thigh_r"),
        leg("shin_l"),
        leg("shin_r"),
        boot("boot_l"),
        boot("boot_r"),
    ]
    expected = {part.name for part in parts}
    if len(expected) != len(parts):
        raise SystemExit("duplicate part name")
    for part in parts:
        if len(part.indices) < 12 or len(part.indices) % 3 != 0:
            raise SystemExit(f"{part.name} has no triangles")
        if any(length(normal) < 0.9 for normal in part.normals):
            raise SystemExit(f"{part.name} has a bad normal")
    return parts


def length(value):
    return math.sqrt(dot(value, value))


def pack_f32(values):
    return b"".join(struct.pack("<f", float(value)) for value in values)


def pack_u32(values):
    return b"".join(struct.pack("<I", int(value)) for value in values)


def build_glb(parts):
    blob = bytearray()
    buffer_views = []
    accessors = []
    materials = []
    meshes = []
    nodes = []

    def add_view(data, target):
        offset = len(blob)
        blob.extend(data)
        buffer_views.append({
            "buffer": 0,
            "byteOffset": offset,
            "byteLength": len(data),
            "target": target,
        })
        return len(buffer_views) - 1

    def add_accessor(view_index, component_type, accessor_type, count, minimum=None, maximum=None):
        accessor = {
            "bufferView": view_index,
            "componentType": component_type,
            "count": count,
            "type": accessor_type,
        }
        if minimum is not None:
            accessor["min"] = minimum
            accessor["max"] = maximum
        accessors.append(accessor)
        return len(accessors) - 1

    for part in parts:
        flat_positions = [axis for position in part.positions for axis in position]
        flat_normals = [axis for normal in part.normals for axis in normal]
        flat_uvs = [axis for uv in part.uvs for axis in uv]
        xs = flat_positions[0::3]
        ys = flat_positions[1::3]
        zs = flat_positions[2::3]
        position_view = add_view(pack_f32(flat_positions), 34962)
        normal_view = add_view(pack_f32(flat_normals), 34962)
        uv_view = add_view(pack_f32(flat_uvs), 34962)
        index_view = add_view(pack_u32(part.indices), 34963)
        position_accessor = add_accessor(
            position_view, 5126, "VEC3", len(part.positions),
            [min(xs), min(ys), min(zs)], [max(xs), max(ys), max(zs)])
        normal_accessor = add_accessor(normal_view, 5126, "VEC3", len(part.normals))
        uv_accessor = add_accessor(uv_view, 5126, "VEC2", len(part.uvs))
        index_accessor = add_accessor(index_view, 5125, "SCALAR", len(part.indices))
        materials.append({
            "name": part.name + "Material",
            "pbrMetallicRoughness": {
                "baseColorFactor": list(part.color),
                "metallicFactor": part.metallic,
                "roughnessFactor": part.roughness,
            },
        })
        meshes.append({
            "name": part.name,
            "primitives": [{
                "attributes": {
                    "POSITION": position_accessor,
                    "NORMAL": normal_accessor,
                    "TEXCOORD_0": uv_accessor,
                },
                "indices": index_accessor,
                "material": len(materials) - 1,
                "mode": 4,
            }],
        })
        nodes.append({"name": part.name, "mesh": len(meshes) - 1})

    gltf = {
        "asset": {"version": "2.0", "generator": "NEXTGENOPENWORLD2027PRO enemy-guard"},
        "scene": 0,
        "scenes": [{"name": "EnemyGuard", "nodes": list(range(len(nodes)))}],
        "nodes": nodes,
        "meshes": meshes,
        "materials": materials,
        "accessors": accessors,
        "bufferViews": buffer_views,
        "buffers": [{"byteLength": len(blob)}],
    }
    json_chunk = json.dumps(gltf, separators=(",", ":")).encode("utf-8")
    while len(json_chunk) % 4:
        json_chunk += b" "
    while len(blob) % 4:
        blob.append(0)
    total = 12 + 8 + len(json_chunk) + 8 + len(blob)
    header = struct.pack("<III", 0x46546C67, 2, total)
    json_header = struct.pack("<II", len(json_chunk), 0x4E4F534A)
    bin_header = struct.pack("<II", len(blob), 0x004E4942)
    return header + json_header + json_chunk + bin_header + bytes(blob)


def main():
    output = Path(sys.argv[1]) if len(sys.argv) > 1 else Path(
        "app/src/main/assets/models/enemy_guard.glb"
    )
    parts = build_parts()
    output.parent.mkdir(parents=True, exist_ok=True)
    payload = build_glb(parts)
    output.write_bytes(payload)
    triangles = sum(len(part.indices) // 3 for part in parts)
    print(f"wrote {output} ({len(payload)} bytes) parts={len(parts)} triangles={triangles}")


if __name__ == "__main__":
    main()
