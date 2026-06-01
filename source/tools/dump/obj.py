#!/usr/bin/env python3

"""
Parse geometry.txt from ray-bench capture to Wavefront OBJ format.

Usage:
    python obj.py [geometry.txt] [output.obj]
"""

import sys
import re
from dataclasses import dataclass, field
from typing import Dict, List, Optional
from pathlib import Path


@dataclass
class Geometry:
    """Represents a geometry within a BLAS."""
    vertices: List[tuple] = field(default_factory=list)  # List of (x, y, z)
    indices: List[int] = field(default_factory=list)
    transform: Optional[List[List[float]]] = None  # 3x4 transformation matrix


@dataclass
class BLAS:
    """Bottom-Level Acceleration Structure."""
    address: int
    geometries: List[Geometry] = field(default_factory=list)


@dataclass
class Instance:
    """TLAS instance referencing a BLAS."""
    blas_address: int
    transform: List[List[float]]  # 3x4 transformation matrix


@dataclass
class TLAS:
    """Top-Level Acceleration Structure."""
    address: int
    instances: List[Instance] = field(default_factory=list)


def identity_3x4():
    """Create a 3x4 identity matrix."""
    return [
        [1.0, 0.0, 0.0, 0.0],
        [0.0, 1.0, 0.0, 0.0],
        [0.0, 0.0, 1.0, 0.0]
    ]


def parse_geometry_file(filename: str) -> tuple[Dict[int, BLAS], List[TLAS]]:
    """Parse geometry.txt and return BLAS and TLAS data."""
    blas_map: Dict[int, BLAS] = {}
    tlas_list: List[TLAS] = []
    
    with open(filename, 'r') as f:
        lines = f.readlines()
    
    i = 0
    while i < len(lines):
        line = lines[i].rstrip()
        
        # Parse acceleration structure header
        if line.startswith("Acceleration Structure:"):
            match = re.search(r'0x([0-9a-fA-F]+)', line)
            if match:
                addr = int(match.group(1), 16)
                i += 1
                
                # Check type
                if i < len(lines):
                    type_line = lines[i].rstrip()
                    if type_line == "Type: TLAS":
                        tlas = TLAS(address=addr, instances=[])
                        i += 1
                        
                        # Parse instances count
                        if i < len(lines) and lines[i].startswith("Instances:"):
                            match = re.search(r'Instances: (\d+)', lines[i])
                            if match:
                                num_instances = int(match.group(1))
                                i += 1
                                
                                # Parse each instance
                                for _ in range(num_instances):
                                    # Find "Instance X:"
                                    while i < len(lines) and not re.match(r'\s+Instance \d+:', lines[i]):
                                        i += 1
                                    i += 1  # Skip "Instance X:" line
                                    
                                    # Parse instance fields
                                    blas_addr = 0
                                    transform = identity_3x4()
                                    
                                    while i < len(lines):
                                        inst_line = lines[i].rstrip()
                                        
                                        if re.match(r'\s+AccelerationStructure:', inst_line):
                                            match = re.search(r'0x([0-9a-fA-F]+)', inst_line)
                                            if match:
                                                blas_addr = int(match.group(1), 16)
                                            i += 1
                                        elif re.match(r'\s+Transform:', inst_line):
                                            i += 1
                                            # Parse 3 rows of transform
                                            for row in range(3):
                                                if i < len(lines):
                                                    match = re.search(r'\[\s*([^\]]+)\]', lines[i])
                                                    if match:
                                                        values = [float(v) for v in match.group(1).split()]
                                                        transform[row][:len(values)] = values[:4]
                                                i += 1
                                            break
                                        else:
                                            i += 1
                                    
                                    tlas.instances.append(Instance(blas_address=blas_addr, transform=transform))
                                
                        tlas_list.append(tlas)
                        
                    elif type_line == "Type: BLAS":
                        blas = BLAS(address=addr, geometries=[])
                        i += 1
                        
                        # Parse geometries
                        while i < len(lines):
                            geom_line = lines[i].rstrip()
                            
                            if geom_line.startswith("Acceleration Structure:") or not lines[i].strip():
                                break
                            
                            if re.match(r'\s+Geometry \d+:', geom_line):
                                geom = Geometry()
                                i += 1
                                
                                # Parse geometry fields
                                while i < len(lines):
                                    field_line = lines[i].rstrip()
                                    
                                    if re.match(r'\s+Geometry \d+:', field_line) or \
                                       field_line.startswith("Acceleration Structure:") or \
                                       not field_line.strip():
                                        break
                                    
                                    if re.match(r'\s+Transform:', field_line):
                                        i += 1
                                        transform = identity_3x4()
                                        # Parse 3 rows of transform
                                        for row in range(3):
                                            if i < len(lines):
                                                match = re.search(r'\[\s*([^\]]+)\]', lines[i])
                                                if match:
                                                    values = [float(v) for v in match.group(1).split()]
                                                    transform[row][:len(values)] = values[:4]
                                            i += 1
                                        geom.transform = transform
                                    
                                    elif re.match(r'\s+Indices', field_line):
                                        match = re.search(r'Indices \((\d+)\):', field_line)
                                        if match:
                                            num_indices = int(match.group(1))
                                            i += 1
                                            
                                            # Parse indices (may span multiple lines)
                                            indices_collected = 0
                                            while indices_collected < num_indices and i < len(lines):
                                                idx_line = lines[i].strip()
                                                if idx_line and not idx_line.startswith('Vertices'):
                                                    try:
                                                        indices = [int(x) for x in idx_line.split()]
                                                        geom.indices.extend(indices)
                                                        indices_collected += len(indices)
                                                    except ValueError:
                                                        break
                                                i += 1
                                    
                                    elif re.match(r'\s+Vertices', field_line):
                                        match = re.search(r'Vertices \((\d+)\):', field_line)
                                        if match:
                                            num_vertices = int(match.group(1))
                                            i += 1
                                            
                                            # Skip VertexFormat and Raw bytes lines
                                            while i < len(lines):
                                                vline = lines[i].rstrip()
                                                if vline.strip().startswith('v'):
                                                    break
                                                i += 1
                                            
                                            # Parse vertices
                                            for _ in range(num_vertices):
                                                if i < len(lines):
                                                    vline = lines[i].rstrip()
                                                    match = re.search(r'v\d+:\s*\(([^)]+)\)', vline)
                                                    if match:
                                                        coords = [float(v) for v in match.group(1).split(',')]
                                                        if len(coords) >= 3:
                                                            geom.vertices.append((coords[0], coords[1], coords[2]))
                                                i += 1
                                    
                                    else:
                                        i += 1
                                
                                blas.geometries.append(geom)
                            else:
                                i += 1
                        
                        blas_map[addr] = blas
                    else:
                        i += 1
                else:
                    i += 1
        else:
            i += 1
    
    return blas_map, tlas_list


def apply_transform(vertex: tuple, transform: List[List[float]]) -> tuple:
    """Apply 3x4 transformation matrix to a vertex."""
    x, y, z = vertex
    # Transform is 3x4 matrix: [[m00, m01, m02, m03], [m10, m11, m12, m13], [m20, m21, m22, m23]]
    # Result = M * [x, y, z, 1]
    tx = transform[0][0] * x + transform[0][1] * y + transform[0][2] * z + transform[0][3]
    ty = transform[1][0] * x + transform[1][1] * y + transform[1][2] * z + transform[1][3]
    tz = transform[2][0] * x + transform[2][1] * y + transform[2][2] * z + transform[2][3]
    return (tx, ty, tz)


def write_obj(filename: str, blas_map: Dict[int, BLAS], tlas_list: List[TLAS]):
    """Write geometry to OBJ file."""
    with open(filename, 'w') as f:
        f.write("# Generated from RayBench geometry.txt\n")
        f.write(f"# BLAS count: {len(blas_map)}\n")
        f.write(f"# TLAS count: {len(tlas_list)}\n\n")
        
        vertex_offset = 1  # OBJ uses 1-based indexing
        
        # If we have TLAS, use instances; otherwise dump all BLAS directly
        if tlas_list:
            for tlas_idx, tlas in enumerate(tlas_list):
                f.write(f"# TLAS {tlas_idx} (0x{tlas.address:X})\n")
                
                for inst_idx, instance in enumerate(tlas.instances):
                    f.write(f"o instance_{tlas_idx}_{inst_idx}_blas_{instance.blas_address:X}\n")
                    
                    if instance.blas_address in blas_map:
                        blas = blas_map[instance.blas_address]
                        
                        # Write vertices and faces for each geometry
                        for geom in blas.geometries:
                            # Write vertices with transform applied
                            for vertex in geom.vertices:
                                # Apply geometry transform first if present
                                if geom.transform is not None:
                                    v = apply_transform(vertex, geom.transform)
                                else:
                                    v = vertex
                                
                                # Then apply instance transform
                                v = apply_transform(v, instance.transform)
                                f.write(f"v {v[0]:.6f} {v[1]:.6f} {v[2]:.6f}\n")
                            
                            # Write faces for this geometry
                            for i in range(0, len(geom.indices), 3):
                                if i + 2 < len(geom.indices):
                                    i0 = geom.indices[i] + vertex_offset
                                    i1 = geom.indices[i + 1] + vertex_offset
                                    i2 = geom.indices[i + 2] + vertex_offset
                                    f.write(f"f {i0} {i1} {i2}\n")
                            
                            # Update vertex offset for next geometry
                            vertex_offset += len(geom.vertices)
        else:
            # No TLAS, dump all BLAS directly
            for blas_addr, blas in blas_map.items():
                f.write(f"o blas_{blas_addr:X}\n")
                
                # Write vertices and faces for each geometry
                for geom in blas.geometries:
                    # Write vertices with transform applied
                    for vertex in geom.vertices:
                        if geom.transform is not None:
                            v = apply_transform(vertex, geom.transform)
                        else:
                            v = vertex
                        f.write(f"v {v[0]:.6f} {v[1]:.6f} {v[2]:.6f}\n")
                    
                    # Write faces for this geometry
                    for i in range(0, len(geom.indices), 3):
                        if i + 2 < len(geom.indices):
                            i0 = geom.indices[i] + vertex_offset
                            i1 = geom.indices[i + 1] + vertex_offset
                            i2 = geom.indices[i + 2] + vertex_offset
                            f.write(f"f {i0} {i1} {i2}\n")
                    
                    # Update vertex offset for next geometry
                    vertex_offset += len(geom.vertices)
        
        print(f"Written {vertex_offset - 1} vertices to {filename}")


def main():
    input_file = sys.argv[1] if len(sys.argv) > 1 else "geometry.txt"
    output_file = sys.argv[2] if len(sys.argv) > 2 else "output.obj"
    
    if not Path(input_file).exists():
        print(f"Error: {input_file} not found")
        sys.exit(1)
    
    print(f"Parsing {input_file}...")
    blas_map, tlas_list = parse_geometry_file(input_file)
    
    print(f"Found {len(blas_map)} BLAS and {len(tlas_list)} TLAS")
    
    if not blas_map:
        print("No geometry found in file")
        sys.exit(1)
    
    print(f"Writing {output_file}...")
    write_obj(output_file, blas_map, tlas_list)


if __name__ == "__main__":
    main()
