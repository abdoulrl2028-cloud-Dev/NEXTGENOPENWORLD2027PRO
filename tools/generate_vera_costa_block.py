#!/usr/bin/env python3
"""Build a small original glTF blockout for the Phase 1 renderer.

The meshes are architectural volumes used to validate camera, lighting and
glTF import. They are not final characters, vehicles or city art.
"""

import json
import struct
import sys
from pathlib import Path


def sub(a, b):
    return (a[0] - b[0], a[1] - b[1], a[2] - b[2])


def add(a, b):
    return (a[0] + b[0], a[1] + b[1], a[2] + b[2])


def dot(a, b):
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]


def cross(a, b):
    return (
        a[1] * b[2] - a[2] * b[1],
        a[2] * b[0] - a[0] * b[2],
        a[0] * b[1] - a[1] * b[0],
    )


def length(a):
    return dot(a, a) ** 0.5


def normalize(a):
    mag = length(a)
    if mag < 1e-8:
        return (0.0, 1.0, 0.0)
    return (a[0] / mag, a[1] / mag, a[2] / mag)


class Primitive:
    def __init__(self, name, color, metallic, roughness, translation):
        self.name = name
        self.color = color
        self.metallic = metallic
        self.roughness = roughness
        self.translation = translation
        self.positions = []
        self.normals = []
        self.uvs = []
        self.indices = []

    def add_tri(self, a, b, c):
        normal = normalize(cross(sub(b, a), sub(c, a)))
        base = len(self.positions)
        for index, position in enumerate((a, b, c)):
            self.positions.append(position)
            self.normals.append(normal)
            self.uvs.append((1.0 if index else 0.0, 1.0 if index == 2 else 0.0))
        self.indices.extend((base, base + 1, base + 2))

    def add_quad(self, corners, outward):
        a, b, c, d = corners
        if dot(cross(sub(b, a), sub(c, a)), outward) < 0.0:
            b, d = d, b
        self.add_tri(a, b, c)
        self.add_tri(a, c, d)

    def add_box(self, x0, y0, z0, x1, y1, z1):
        def p(x, y, z):
            return (x, y, z)

        self.add_quad(
            [p(x0, y1, z0), p(x0, y1, z1), p(x1, y1, z1), p(x1, y1, z0)],
            (0.0, 1.0, 0.0),
        )
        self.add_quad(
            [p(x0, y0, z1), p(x0, y0, z0), p(x1, y0, z0), p(x1, y0, z1)],
            (0.0, -1.0, 0.0),
        )
        self.add_quad(
            [p(x1, y0, z0), p(x1, y1, z0), p(x1, y1, z1), p(x1, y0, z1)],
            (1.0, 0.0, 0.0),
        )
        self.add_quad(
            [p(x0, y0, z1), p(x0, y1, z1), p(x0, y1, z0), p(x0, y0, z0)],
            (-1.0, 0.0, 0.0),
        )
        self.add_quad(
            [p(x0, y0, z1), p(x1, y0, z1), p(x1, y1, z1), p(x0, y1, z1)],
            (0.0, 0.0, 1.0),
        )
        self.add_quad(
            [p(x1, y0, z0), p(x0, y0, z0), p(x0, y1, z0), p(x1, y1, z0)],
            (0.0, 0.0, -1.0),
        )

    def add_roof(self, width, depth, eave_y, rise):
        x0, x1 = -width * 0.5, width * 0.5
        z0, z1 = -depth * 0.5, depth * 0.5
        apex_y = eave_y + rise
        left_front = (x0, eave_y, z0)
        right_front = (x1, eave_y, z0)
        right_back = (x1, eave_y, z1)
        left_back = (x0, eave_y, z1)
        apex_front = (0.0, apex_y, z0)
        apex_back = (0.0, apex_y, z1)
        self.add_tri(left_front, apex_front, right_front)
        self.add_tri(right_back, apex_back, left_back)
        self.add_quad(
            [right_front, apex_front, apex_back, right_back],
            (1.0, 0.4, 0.0),
        )
        self.add_quad(
            [left_back, apex_back, apex_front, left_front],
            (-1.0, 0.4, 0.0),
        )


def centered_box(name, translation, size, y0, color, roughness, metallic=0.0):
    sx, sy, sz = size
    primitive = Primitive(name, color, metallic, roughness, translation)
    primitive.add_box(-sx * 0.5, y0, -sz * 0.5, sx * 0.5, y0 + sy, sz * 0.5)
    return primitive


def roof(name, translation, width, depth, eave_y, rise, color):
    primitive = Primitive(name, color, 0.0, 0.55, translation)
    primitive.add_roof(width, depth, eave_y, rise)
    return primitive


def build_scene():
    sand = (0.72, 0.64, 0.46, 1.0)
    grass = (0.34, 0.46, 0.30, 1.0)
    water = (0.12, 0.34, 0.46, 1.0)
    asphalt = (0.16, 0.17, 0.18, 1.0)
    walk = (0.62, 0.62, 0.58, 1.0)
    plaza = (0.70, 0.67, 0.58, 1.0)
    stucco = (0.78, 0.70, 0.58, 1.0)
    market = (0.66, 0.42, 0.30, 1.0)
    apartment = (0.55, 0.58, 0.62, 1.0)
    clinic = (0.82, 0.84, 0.80, 1.0)
    warehouse = (0.40, 0.43, 0.40, 1.0)
    tile = (0.48, 0.24, 0.20, 1.0)
    pier = (0.42, 0.32, 0.22, 1.0)

    primitives = [
        centered_box("VeraCostaWater", (-30.0, -0.55, 0.0), (28.0, 0.4, 84.0), 0.0, water, 0.18, 0.04),
        centered_box("VeraCostaBeach", (-12.0, -0.08, 0.0), (14.0, 0.16, 84.0), 0.0, sand, 0.92),
        centered_box("VeraCostaGround", (18.0, -0.12, 0.0), (52.0, 0.24, 84.0), 0.0, grass, 0.95),
        centered_box("VeraCostaRoad", (8.0, 0.02, 0.0), (7.2, 0.08, 84.0), 0.0, asphalt, 0.72),
        centered_box("VeraCostaSidewalkWest", (3.4, 0.04, 0.0), (1.8, 0.10, 84.0), 0.0, walk, 0.86),
        centered_box("VeraCostaSidewalkEast", (12.6, 0.04, 0.0), (1.8, 0.10, 84.0), 0.0, walk, 0.86),
        centered_box("VeraCostaPlaza", (1.2, 0.05, 8.0), (7.5, 0.12, 7.5), 0.0, plaza, 0.80),
        centered_box("VeraCostaPier", (-18.0, 0.15, 2.0), (18.0, 0.28, 3.2), 0.0, pier, 0.70),
        centered_box("VeraCostaHarborOffice", (20.0, 0.0, -22.0), (8.0, 6.0, 10.0), 0.0, stucco, 0.62),
        centered_box("VeraCostaMarket", (21.5, 0.0, -6.0), (10.0, 4.2, 8.0), 0.0, market, 0.58),
        centered_box("VeraCostaApartments", (22.0, 0.0, 12.0), (9.0, 14.0, 12.0), 0.0, apartment, 0.48),
        centered_box("VeraCostaClinic", (20.5, 0.0, 28.0), (8.5, 5.0, 9.0), 0.0, clinic, 0.50),
        centered_box("VeraCostaWarehouse", (33.0, 0.0, -16.0), (12.0, 7.0, 16.0), 0.0, warehouse, 0.66),
        roof("VeraCostaHarborOfficeRoof", (20.0, 0.0, -22.0), 8.6, 10.6, 6.0, 2.2, tile),
        roof("VeraCostaMarketRoof", (21.5, 0.0, -6.0), 10.6, 8.6, 4.2, 1.6, tile),
        roof("VeraCostaClinicRoof", (20.5, 0.0, 28.0), 9.1, 9.6, 5.0, 1.8, tile),
        roof("VeraCostaWarehouseRoof", (33.0, 0.0, -16.0), 12.6, 16.6, 7.0, 1.4, (0.30, 0.32, 0.33, 1.0)),
    ]
    return primitives


def pack_f32(values):
    blob = bytearray()
    for value in values:
        blob += struct.pack("<f", float(value))
    return blob


def pack_u32(values):
    return b"".join(struct.pack("<I", int(value)) for value in values)


def build_glb(primitives):
    blob = bytearray()
    buffer_views = []
    accessors = []
    materials = []
    meshes = []
    nodes = []

    def add_view(data, target):
        offset = len(blob)
        blob.extend(data)
        view = {
            "buffer": 0,
            "byteOffset": offset,
            "byteLength": len(data),
            "target": target,
        }
        buffer_views.append(view)
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

    for primitive in primitives:
        flat_positions = [axis for position in primitive.positions for axis in position]
        flat_normals = [axis for normal in primitive.normals for axis in normal]
        flat_uvs = [axis for uv in primitive.uvs for axis in uv]
        xs = flat_positions[0::3]
        ys = flat_positions[1::3]
        zs = flat_positions[2::3]
        position_view = add_view(pack_f32(flat_positions), 34962)
        normal_view = add_view(pack_f32(flat_normals), 34962)
        uv_view = add_view(pack_f32(flat_uvs), 34962)
        index_view = add_view(pack_u32(primitive.indices), 34963)
        position_accessor = add_accessor(
            position_view,
            5126,
            "VEC3",
            len(primitive.positions),
            [min(xs), min(ys), min(zs)],
            [max(xs), max(ys), max(zs)],
        )
        normal_accessor = add_accessor(normal_view, 5126, "VEC3", len(primitive.normals))
        uv_accessor = add_accessor(uv_view, 5126, "VEC2", len(primitive.uvs))
        index_accessor = add_accessor(index_view, 5125, "SCALAR", len(primitive.indices))
        materials.append(
            {
                "name": primitive.name + "Material",
                "pbrMetallicRoughness": {
                    "baseColorFactor": list(primitive.color),
                    "metallicFactor": primitive.metallic,
                    "roughnessFactor": primitive.roughness,
                },
            }
        )
        meshes.append(
            {
                "name": primitive.name,
                "primitives": [
                    {
                        "attributes": {
                            "POSITION": position_accessor,
                            "NORMAL": normal_accessor,
                            "TEXCOORD_0": uv_accessor,
                        },
                        "indices": index_accessor,
                        "material": len(materials) - 1,
                        "mode": 4,
                    }
                ],
            }
        )
        nodes.append(
            {
                "name": primitive.name,
                "mesh": len(meshes) - 1,
                "translation": list(primitive.translation),
            }
        )

    gltf = {
        "asset": {"version": "2.0", "generator": "NEXTGENOPENWORLD2027PRO vera-costa-blockout"},
        "scene": 0,
        "scenes": [{"name": "VeraCostaBlock", "nodes": list(range(len(nodes)))}],
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
        "app/src/main/assets/models/vera_costa_block.glb"
    )
    output.parent.mkdir(parents=True, exist_ok=True)
    payload = build_glb(build_scene())
    output.write_bytes(payload)
    print(f"wrote {output} ({len(payload)} bytes)")


if __name__ == "__main__":
    main()
