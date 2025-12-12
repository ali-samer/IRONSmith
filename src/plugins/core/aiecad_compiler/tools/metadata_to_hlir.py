#!/usr/bin/env python3
"""
metadata_to_hlir.py

Build an HLIR Program from a saved AIECAD metadata (.aiecad JSON) file.

Steps (kept explicit and loop-based for clarity):
1) Add constants/variables
2) Add type definitions
3) Add tiles (shim / memory / compute)
4) Add FIFOs (object_fifos / connections)
5) Add a minimal runtime (fill/drain) and build the program

Optional: serialize to GUI XML, expand to complete XML, emit GraphML and
generated Python code using the existing compiler pipeline.
"""

import argparse
import json
import sys
from pathlib import Path
from typing import Any, Dict, List, Optional

# Ensure local aiecad_compiler package is importable when run as a script
THIS_DIR = Path(__file__).resolve().parent
PKG_ROOT = THIS_DIR.parent
if str(PKG_ROOT) not in sys.path:
    sys.path.insert(0, str(PKG_ROOT))

try:
    from ..hlir import ProgramBuilder
    from ..hlir.gui_serializer import GUIXMLSerializer
    from ..graph_builder.XMLGenerator import XMLTransformer
    from ..graph_builder.GraphDriver import GraphBuilder
    from ..codegen.backends.CodeGenerator import CodeGenerator
    import networkx as nx
except Exception:
    try:
        from hlir import ProgramBuilder
        from hlir.gui_serializer import GUIXMLSerializer
        from graph_builder.XMLGenerator import XMLTransformer
        from graph_builder.GraphDriver import GraphBuilder
        from codegen.backends.CodeGenerator import CodeGenerator
        import networkx as nx
    except ImportError as exc:
        sys.stderr.write(f"[metadata_to_hlir] Import error: {exc}\n")
        sys.exit(1)


def _parse_int(value: str) -> Optional[int]:
    """Best-effort int parsing; returns None on failure."""
    try:
        return int(value)
    except Exception:
        return None


def _parse_dimensions(dim_str: str) -> List[Any]:
    if not dim_str:
        return []
    parts = [p.strip() for p in dim_str.split(",")]
    dims: List[Any] = []
    for p in parts:
        if not p:
            continue
        if p.startswith("$"):
            p = p[1:]
        parsed = _parse_int(p)
        dims.append(parsed if parsed is not None else p)
    return dims


def build_program_from_metadata(meta_path: Path, program_name: Optional[str] = None):
    with meta_path.open("r", encoding="utf-8") as fh:
        data = json.load(fh)

    program_name = program_name or meta_path.stem
    builder = ProgramBuilder(program_name)

    for var in data.get("variables", []):
        name = var.get("name")
        val = var.get("value", "")
        parsed = _parse_int(str(val))
        if parsed is not None:
            builder.add_constant(name, parsed, "int")
        else:
            builder.add_symbol(name, val, type_hint="str")

    for ty in data.get("types", []):
        name = ty.get("name")
        dims = _parse_dimensions(ty.get("dimensions", ""))
        dtype = ty.get("type", "int32")
        builder.add_tensor_type(name, shape=dims if dims else [1], dtype=dtype)

    node_id_to_tile: Dict[int, str] = {}
    node_id_to_model: Dict[int, str] = {}

    tile_idx = 0
    for node in data.get("nodes", []):
        model = node.get("model")
        kind = None
        if model == "ShimTile":
            kind = "shim"
        elif model == "MemoryTile":
            kind = "mem"
        elif model == "ComputeNode":
            kind = "compute"

        if not kind:
            node_id_to_model[node.get("id")] = model
            continue

        tile_idx += 1
        tile_name = node.get("custom_name") or f"{kind}{tile_idx}"
        coord = node.get("grid_coord", {})
        if coord:
            x = int(coord.get("x", tile_idx))
            y = int(coord.get("y", 0))
        else:
            pos = node.get("position", {})
            x = int(pos.get("x", tile_idx))
            y = int(pos.get("y", 0))
        builder.add_tile(tile_name, kind=kind, x=x, y=y)
        node_id_to_tile[node.get("id")] = tile_name
        node_id_to_model[node.get("id")] = model

    fifo_order: List[str] = []
    seen_fifos: Dict[str, int] = {}

    stream_nodes = {nid for nid, model in node_id_to_model.items() if model == "StreamThrough"}
    incoming_to_stream: Dict[int, Dict[str, Any]] = {}
    outgoing_from_stream: Dict[int, List[Dict[str, Any]]] = {}

    fifo_entries = data.get("object_fifos", data.get("connections", []))

    for fifo in fifo_entries:
        name = fifo.get("name")
        obj_type = fifo.get("type") or fifo.get("props", {}).get("type") or "int32"
        depth = int(fifo.get("depth", fifo.get("props", {}).get("depth", 1)))
        out_node = fifo.get("out_node") or fifo.get("outNodeId")
        in_node = fifo.get("in_node") or fifo.get("inNodeId")

        producer_tile = node_id_to_tile.get(out_node)
        consumer_tile = node_id_to_tile.get(in_node)

        if in_node in stream_nodes:
            incoming_to_stream[in_node] = {
                "name": name,
                "type": obj_type,
                "depth": depth,
                "producer_tile": producer_tile,
            }
            continue

        if out_node in stream_nodes:
            outgoing_from_stream.setdefault(out_node, []).append({
                "name": name,
                "type": obj_type,
                "depth": depth,
                "consumer_tile": consumer_tile,
            })
            continue

        if not producer_tile and not consumer_tile:
            continue

        base_name = name or f"of{len(fifo_order)+1}"
        if base_name in seen_fifos:
            seen_fifos[base_name] += 1
            base_name = f"{base_name}_{seen_fifos[base_name]}"
        else:
            seen_fifos[base_name] = 1

        consumers = [consumer_tile] if consumer_tile else []

        builder.add_fifo(
            base_name,
            obj_type=obj_type,
            depth=depth if depth > 0 else 1,
            producer=producer_tile,
            consumers=consumers
        )
        fifo_order.append(base_name)

    for stream_id in stream_nodes:
        incoming = incoming_to_stream.get(stream_id)
        outs = outgoing_from_stream.get(stream_id, [])
        if not incoming:
            continue

        base_name = incoming.get("name") or f"of{len(fifo_order)+1}"
        if base_name in seen_fifos:
            seen_fifos[base_name] += 1
            base_name = f"{base_name}_{seen_fifos[base_name]}"
        else:
            seen_fifos[base_name] = 1

        builder.add_fifo(
            base_name,
            obj_type=incoming.get("type", "int32"),
            depth=incoming.get("depth", 1),
            producer=incoming.get("producer_tile"),
            consumers=[]
        )
        fifo_order.append(base_name)

        if not outs:
            continue

        for idx, out_meta in enumerate(outs, start=1):
            out_name = out_meta.get("name") or f"{base_name}_fwd{idx}"
            if out_name in seen_fifos:
                seen_fifos[out_name] += 1
                out_name = f"{out_name}_{seen_fifos[out_name]}"
            else:
                seen_fifos[out_name] = 1

            builder.add_fifo_forward(out_name, source=base_name)
            fifo_order.append(out_name)

    rt = builder.create_runtime("runtime")
    io_type = None
    if data.get("types"):
        io_type = data["types"][0].get("name")
    elif fifo_order:
        io_type = data.get("object_fifos", data.get("connections", []))[0].get("type")
    io_type = io_type or "int32"

    rt.add_input_type(io_type)
    rt.add_output_type(io_type)
    rt.add_params(["a_in", "c_out"])

    shim_tile = next((name for _nid, name in node_id_to_tile.items() if name.lower().startswith("shim")), None)
    fill_fifo = fifo_order[0] if fifo_order else None
    drain_fifo = fifo_order[-1] if fifo_order else fill_fifo

    if fill_fifo:
        rt.add_fill("fill_0", fill_fifo, "a_in", shim_tile or "")
    if drain_fifo:
        rt.add_drain("drain_0", drain_fifo, "c_out", shim_tile or "", wait=True)
    rt.build()

    program = builder.build()
    return program


def run_pipeline(program, out_dir: Path, base_name: str,
                 emit_gui: bool, emit_complete: bool,
                 emit_graphml: bool, emit_code: bool):
    out_dir.mkdir(parents=True, exist_ok=True)
    gui_xml_path = out_dir / f"{base_name}_gui.xml"
    complete_xml_path = out_dir / f"{base_name}_complete.xml"
    graphml_path = out_dir / f"{base_name}.graphml"
    code_path = out_dir / f"generated_{base_name}.py"

    if emit_gui or emit_complete or emit_graphml or emit_code:
        GUIXMLSerializer(pretty_print=True).serialize_to_file(program, gui_xml_path)

    if emit_complete or emit_graphml or emit_code:
        XMLTransformer(gui_xml_path).save(complete_xml_path)

    if emit_graphml or emit_code:
        graph = GraphBuilder(complete_xml_path).build()
        nx.write_graphml(graph, graphml_path)

    if emit_code:
        code = CodeGenerator(graphml_path).generate()
        code_path.write_text(code, encoding="utf-8")

    return {
        "gui_xml": gui_xml_path if emit_gui or emit_complete or emit_graphml or emit_code else None,
        "complete_xml": complete_xml_path if emit_complete or emit_graphml or emit_code else None,
        "graphml": graphml_path if emit_graphml or emit_code else None,
        "generated_py": code_path if emit_code else None,
    }


def main():
    ap = argparse.ArgumentParser(description="Build HLIR Program from .aiecad metadata")
    ap.add_argument("metadata", type=Path, help="Path to .aiecad metadata JSON file")
    ap.add_argument("--name", help="Override program name (default: metadata stem)")
    ap.add_argument("--out-dir", type=Path, help="Output directory (default: metadata dir)")
    ap.add_argument("--emit-gui", action="store_true", help="Write GUI XML")
    ap.add_argument("--emit-complete", action="store_true", help="Write complete XML")
    ap.add_argument("--emit-graphml", action="store_true", help="Write GraphML")
    ap.add_argument("--emit-code", action="store_true", help="Write generated Python code")
    args = ap.parse_args()

    meta_path = args.metadata
    if not meta_path.is_file():
        sys.stderr.write(f"Metadata not found: {meta_path}\n")
        sys.exit(1)

    base_name = args.name or meta_path.stem
    out_dir = args.out_dir or meta_path.parent

    program = build_program_from_metadata(meta_path, program_name=base_name)
    results = run_pipeline(
        program,
        out_dir=out_dir,
        base_name=base_name,
        emit_gui=args.emit_gui,
        emit_complete=args.emit_complete,
        emit_graphml=args.emit_graphml,
        emit_code=args.emit_code,
    )

    sys.stdout.write(f"[metadata_to_hlir] Built program '{program.name}'\n")
    if results.get("generated_py"):
        sys.stdout.write(f"Generated code: {results['generated_py']}\n")
    if results.get("graphml"):
        sys.stdout.write(f"GraphML: {results['graphml']}\n")
    if results.get("complete_xml"):
        sys.stdout.write(f"Complete XML: {results['complete_xml']}\n")
    if results.get("gui_xml"):
        sys.stdout.write(f"GUI XML: {results['gui_xml']}\n")


if __name__ == "__main__":
    main()
