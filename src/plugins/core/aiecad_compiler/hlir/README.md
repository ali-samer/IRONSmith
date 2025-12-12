# AIECAD High-Level Intermediate Representation (HLIR)

## Overview

The AIECAD HLIR is a clean, XML-free Python intermediate representation for AIE programs. It provides a semantic abstraction layer between GUI/frontend tools and the XML-based code generation pipeline. The HLIR is designed to be:

- **XML-independent**: No XML tags, attributes, or parsing concerns
- **Type-safe**: Rich type system with tensor shapes and data types
- **Graph-based**: Explicit representation of tiles (nodes) and ObjectFifos (edges)
- **Fluent**: Builder API with method chaining and invariant enforcement
- **Serializable**: Can export to XML, JSON, or other formats

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                         Frontend                            │
│                    (GUI, Python API, etc.)                  │
└─────────────────────────┬───────────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────────┐
│                    HLIR Builder API                         │
│            (Fluent, user-friendly construction)             │
└─────────────────────────┬───────────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────────┐
│                    HLIR Core Classes                        │
│   Program, Tile, ObjectFifo, Worker, ExternalKernel, etc.  │
└─────────────────────────┬───────────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────────┐
│                  GUIXMLSerializer                           │
│          (Converts HLIR to GUI XML format)                  │
└─────────────────────────┬───────────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────────┐
│                    XMLGenerator                             │
│         (Expands GUI XML to Complete XML)                   │
└─────────────────────────┬───────────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────────┐
│                 Existing Code Pipeline                      │
│        GraphDriver → GraphML → CodeGenerator → Python       │
└─────────────────────────────────────────────────────────────┘
```

## Package Structure

```
hlir/
├── __init__.py           # Package exports
├── types.py              # Type system (DataType, TensorType, ScalarType)
├── core.py               # Core semantic classes (Program, Tile, ObjectFifo, etc.)
├── operations.py         # Operations (Split, Join, Forward, Fill, Drain, TAP)
├── builder.py            # Fluent builder API (ProgramBuilder, RuntimeBuilder)
├── serializer.py         # Complete XML serialization (legacy)
├── gui_serializer.py     # GUI XML serialization (recommended)
├── test_hlir.py          # Comprehensive tests
├── examples.py           # Usage examples
└── README.md             # This file
```

## Core Concepts

### 1. Type System (`types.py`)

The HLIR provides a rich type system for representing tensor and scalar types:

**DataType**: Enum of supported scalar types
```python
from hlir import DataType

DataType.INT32      # 32-bit integer
DataType.FLOAT32    # 32-bit float
DataType.BFLOAT16   # Brain float 16
# ... and more
```

**TensorType**: Represents multi-dimensional tensors with shape and dtype
```python
from hlir import TensorType, DataType

# Concrete shape
tensor = TensorType(shape=(128, 256), dtype=DataType.INT32)

# Symbolic shape (useful for runtime-sized tensors)
tensor = TensorType(shape=("N", "N // 4"), dtype=DataType.FLOAT32)
```

### 2. Core Classes (`core.py`)

**Program**: Top-level container for an AIECAD design
```python
from hlir import Program

prog = Program(name="my_design")
# Contains: tiles, fifos, workers, kernels, functions, runtime
```

**Tile**: Represents a physical tile in the AIE array
```python
from hlir import Tile, TileKind

shim = Tile(name="shim0", kind=TileKind.SHIM, x=0, y=0)
mem = Tile(name="mem0", kind=TileKind.MEM, x=0, y=1)
compute = Tile(name="compute0", kind=TileKind.COMPUTE, x=0, y=2)
```

**ObjectFifo**: Data movement channel (edge in the dataflow graph)
```python
from hlir import ObjectFifo

fifo = ObjectFifo(
    name="fifo_in",
    obj_type="chunk_ty",  # Type reference
    depth=2,               # Number of buffers
    producer=shim_tile,
    consumers=[compute_tile]
)
```

**ExternalKernel**: External C/C++ kernel declaration
```python
from hlir import ExternalKernel

kernel = ExternalKernel(
    name="add_kernel",
    kernel_name="eltwise_add_bf16",
    source_file="kernels/add.cc",
    arg_types=["chunk_a", "chunk_b", "chunk_c"],
    include_dirs=["/path/to/includes"]
)
```

**CoreFunction**: Python wrapper function for kernel invocations
```python
from hlir import CoreFunction, Acquire, Release, KernelCall

func = CoreFunction(
    name="add_fn",
    parameters=["kernel", "fifoA", "fifoB", "fifoC"],
    acquires=[
        Acquire("fifoA", 1, "elemA"),
        Acquire("fifoB", 1, "elemB"),
        Acquire("fifoC", 1, "elemC")
    ],
    kernel_call=KernelCall("kernel", ["elemA", "elemB", "elemC"]),
    releases=[
        Release("fifoA", 1),
        Release("fifoB", 1),
        Release("fifoC", 1)
    ]
)
```

**Worker**: Binds a core function to a specific tile with specific FIFOs
```python
from hlir import Worker, FifoBinding, FifoAccessMode

worker = Worker(
    name="worker_0",
    core_fn=add_fn,
    fn_args=[
        "add_kernel",  # External kernel reference
        FifoBinding(fifo_a, FifoAccessMode.CONSUMER, index=0),
        FifoBinding(fifo_b, FifoAccessMode.CONSUMER, index=1),
        FifoBinding(fifo_c, FifoAccessMode.PRODUCER, index=None)
    ],
    placement=compute_tile
)
```

**RuntimeSequence**: Host-side control flow (fill/drain operations)
```python
from hlir import RuntimeSequence, RuntimeFill, RuntimeDrain

runtime = RuntimeSequence(
    name="runtime",
    input_types=["vector_ty", "vector_ty"],
    output_types=["vector_ty"],
    param_names=["a_in", "b_in", "c_out"],
    workers=[worker_0, worker_1],
    operations=[
        RuntimeFill(placement=shim, fifo=fifo_a, source_param="a_in"),
        RuntimeFill(placement=shim, fifo=fifo_b, source_param="b_in"),
        RuntimeDrain(placement=shim, fifo=fifo_c, dest_param="c_out", wait=True)
    ]
)
```

### 3. Operations (`operations.py`)

**SplitOperation**: Splits a FIFO consumer into multiple outputs
```python
from hlir import SplitOperation

split = SplitOperation(
    name="split_a",
    source=fifo_in,
    num_outputs=2,
    output_type="worker_chunk_ty",
    output_names=["out_0", "out_1"],
    offsets=[0, 1024],  # Can be symbolic: ["0", "worker_chunk_size"]
    placement=mem_tile
)
```

**JoinOperation**: Combines multiple inputs into a single FIFO producer
```python
from hlir import JoinOperation

join = JoinOperation(
    name="join_c",
    dest=fifo_out,
    num_inputs=2,
    input_type="worker_chunk_ty",
    input_names=["in_0", "in_1"],
    offsets=[0, 1024],
    placement=mem_tile
)
```

**ForwardOperation**: Simple passthrough (cons -> prod)
```python
from hlir import ForwardOperation

forward = ForwardOperation(
    name="fifo_out",
    source=fifo_in
)
```

**TensorAccessPattern**: Multi-dimensional DMA access pattern
```python
from hlir import TensorAccessPattern

tap = TensorAccessPattern(
    tensor_dims=["N"],
    offset="(N // 4) * column",
    sizes=["N // 4", "chunk_size"],
    strides=["chunk_size", "1"]
)
```

## Builder API

The fluent builder API provides a user-friendly way to construct programs without manually wiring fields.

### Basic Usage

```python
from hlir import ProgramBuilder

# Create builder
builder = ProgramBuilder("my_program")

# Add constants
builder.add_constant("N", 4096, "int")
builder.add_constant("chunk_size", 1024, "int")

# Add type definitions
builder.add_tensor_type("chunk_ty", shape=["chunk_size"], dtype="int32")
builder.add_tensor_type("vector_ty", shape=["N"], dtype="int32")

# Add tiles
shim = builder.add_tile("shim0", kind="shim", x=0, y=0)
compute = builder.add_tile("compute0", kind="compute", x=0, y=2)

# Add FIFOs
fifo_in = builder.add_fifo("fifo_in", obj_type="chunk_ty", depth=2)

# Add forward operation
fifo_out = builder.add_fifo_forward("fifo_out", source="fifo_in")

# Build program
program = builder.build()
```

### Complete Passthrough Example

```python
from hlir import ProgramBuilder

def build_passthrough():
    builder = ProgramBuilder("passthrough")

    # Constants & types
    builder.add_constant("N", 4096, "int")
    builder.add_tensor_type("line_ty", shape=[1024], dtype="int32")
    builder.add_tensor_type("vector_ty", shape=["N"], dtype="int32")

    # Tiles
    builder.add_tile("shim0", kind="shim", x=0, y=0)

    # FIFOs
    builder.add_fifo("of_in", obj_type="line_ty", depth=2)
    builder.add_fifo_forward("of_out", source="of_in")

    # Runtime
    rt = builder.create_runtime("runtime")
    rt.add_input_type("vector_ty")
    rt.add_output_type("vector_ty")
    rt.add_params(["a_in", "c_out"])
    rt.add_fill("fill_0", "of_in", "a_in", "shim0")
    rt.add_drain("drain_0", "of_out", "c_out", "shim0", wait=True)
    rt.build()

    return builder.build()
```

### Element-wise Add Example

```python
from hlir import ProgramBuilder

def build_element_wise_add():
    builder = ProgramBuilder("element_wise_add")

    # Constants & types
    builder.add_constant("N", 4096, "int")
    builder.add_tensor_type("chunk_ty", shape=[1024], dtype="bfloat16")
    builder.add_tensor_type("vector_ty", shape=["N"], dtype="bfloat16")

    # Tiles
    builder.add_tile("shim0", kind="shim", x=0, y=0)
    builder.add_tile("compute0", kind="compute", x=0, y=2)

    # External kernel
    builder.add_external_kernel(
        "add_kernel",
        kernel_name="eltwise_add_bf16_scalar",
        source_file="kernels/add.cc",
        arg_types=["chunk_ty", "chunk_ty", "chunk_ty"]
    )

    # Core function
    builder.add_core_function(
        "add_fn",
        parameters=["kernel", "fifoA", "fifoB", "fifoC"],
        acquires=[
            ("fifoA", 1, "elemA"),
            ("fifoB", 1, "elemB"),
            ("fifoC", 1, "elemC")
        ],
        kernel_call=("kernel", ["elemA", "elemB", "elemC"]),
        releases=[("fifoA", 1), ("fifoB", 1), ("fifoC", 1)]
    )

    # FIFOs
    builder.add_fifo("of_in_a", obj_type="chunk_ty", depth=2)
    builder.add_fifo("of_in_b", obj_type="chunk_ty", depth=2)
    builder.add_fifo("of_out_c", obj_type="chunk_ty", depth=2)

    # Worker
    builder.add_worker(
        "worker_add",
        core_fn="add_fn",
        fn_args=[
            "add_kernel",
            ("of_in_a", "cons", None),
            ("of_in_b", "cons", None),
            ("of_out_c", "prod", None)
        ],
        placement="compute0"
    )

    # Runtime
    rt = builder.create_runtime("runtime")
    rt.add_input_type("vector_ty")
    rt.add_input_type("vector_ty")
    rt.add_output_type("vector_ty")
    rt.add_params(["a_in", "b_in", "c_out"])
    rt.add_worker("worker_add")
    rt.add_fill("fill_a", "of_in_a", "a_in", "shim0")
    rt.add_fill("fill_b", "of_in_b", "b_in", "shim0")
    rt.add_drain("drain_c", "of_out_c", "c_out", "shim0", wait=True)
    rt.build()

    return builder.build()
```

### Multi-Worker with Split/Join Example

```python
from hlir import ProgramBuilder

def build_multi_worker():
    builder = ProgramBuilder("multi_worker")

    # Constants & types
    builder.add_constant("N", 8192, "int")
    builder.add_constant("chunk_size", 4096, "int")
    builder.add_constant("worker_chunk_size", 2048, "int")
    builder.add_tensor_type("chunk_ty", shape=["chunk_size"], dtype="bfloat16")
    builder.add_tensor_type("worker_chunk_ty", shape=["worker_chunk_size"], dtype="bfloat16")
    builder.add_tensor_type("vector_ty", shape=["N"], dtype="bfloat16")

    # Tiles
    builder.add_tile("shim0", kind="shim", x=0, y=0)
    builder.add_tile("mem0", kind="mem", x=0, y=1)
    builder.add_tile("compute0", kind="compute", x=0, y=2)
    builder.add_tile("compute1", kind="compute", x=0, y=3)

    # External kernel
    builder.add_external_kernel(
        "add_kernel",
        kernel_name="eltwise_add_bf16_scalar",
        source_file="kernels/add.cc",
        arg_types=["worker_chunk_ty", "worker_chunk_ty", "worker_chunk_ty"]
    )

    # Core function
    builder.add_core_function(
        "add_fn",
        parameters=["kernel", "fifoA", "fifoB", "fifoC"],
        acquires=[("fifoA", 1, "elemA"), ("fifoB", 1, "elemB"), ("fifoC", 1, "elemC")],
        kernel_call=("kernel", ["elemA", "elemB", "elemC"]),
        releases=[("fifoA", 1), ("fifoB", 1), ("fifoC", 1)]
    )

    # L3 <-> L2 FIFOs
    builder.add_fifo("of_in_a_l3l2", obj_type="chunk_ty", depth=2)
    builder.add_fifo("of_in_b_l3l2", obj_type="chunk_ty", depth=2)
    builder.add_fifo("of_out_c_l2l3", obj_type="chunk_ty", depth=2)

    # Split operations
    builder.add_fifo_split(
        "split_a",
        source="of_in_a_l3l2",
        num_outputs=2,
        output_type="worker_chunk_ty",
        output_names=["of_in_a_w0", "of_in_a_w1"],
        offsets=[0, "worker_chunk_size"],
        placement="mem0"
    )

    builder.add_fifo_split(
        "split_b",
        source="of_in_b_l3l2",
        num_outputs=2,
        output_type="worker_chunk_ty",
        output_names=["of_in_b_w0", "of_in_b_w1"],
        offsets=[0, "worker_chunk_size"],
        placement="mem0"
    )

    # Join operation
    builder.add_fifo_join(
        "join_c",
        dest="of_out_c_l2l3",
        num_inputs=2,
        input_type="worker_chunk_ty",
        input_names=["of_out_c_w0", "of_out_c_w1"],
        offsets=[0, "worker_chunk_size"],
        placement="mem0"
    )

    # Workers
    builder.add_worker(
        "worker_0",
        core_fn="add_fn",
        fn_args=[
            "add_kernel",
            ("split_a", "cons", 0),
            ("split_b", "cons", 0),
            ("join_c", "prod", 0)
        ],
        placement="compute0"
    )

    builder.add_worker(
        "worker_1",
        core_fn="add_fn",
        fn_args=[
            "add_kernel",
            ("split_a", "cons", 1),
            ("split_b", "cons", 1),
            ("join_c", "prod", 1)
        ],
        placement="compute1"
    )

    # Runtime
    rt = builder.create_runtime("runtime")
    rt.add_input_type("vector_ty")
    rt.add_input_type("vector_ty")
    rt.add_output_type("vector_ty")
    rt.add_params(["a_in", "b_in", "c_out"])
    rt.add_worker("worker_0")
    rt.add_worker("worker_1")
    rt.add_fill("fill_a", "of_in_a_l3l2", "a_in", "shim0")
    rt.add_fill("fill_b", "of_in_b_l3l2", "b_in", "shim0")
    rt.add_drain("drain_c", "of_out_c_l2l3", "c_out", "shim0", wait=True)
    rt.build()

    return builder.build()
```

## Serialization

The HLIR can be serialized to XML format for use with the existing code generation pipeline.

```python
from hlir import XMLSerializer

# Build program
program = build_passthrough()

# Create serializer
serializer = XMLSerializer(pretty_print=True)

# Serialize to string
xml_str = serializer.serialize(program)

# Serialize to file
serializer.serialize_to_file(program, "output.xml")
```

The generated XML is in the complete format expected by GraphDriver, matching the structure documented in `XML_Structure_Guide.md`.

## Validation

Programs are automatically validated when built:

```python
builder = ProgramBuilder("test")

# ... construct program ...

# This will validate and raise ValueError if invalid
program = builder.build()

# Manual validation
errors = program.validate()
if errors:
    for error in errors:
        print(f"Error: {error}")
```

Validation checks include:
- No duplicate names across namespaces
- Workers reference valid core functions
- FIFOs have valid producer/consumer tiles
- All references resolve correctly

## Graph Representation

The HLIR explicitly models the AIE design as a directed graph:

- **Nodes**: Tiles (physical placement points)
- **Edges**: ObjectFifos (data movement channels)
- **Compute Units**: Workers (executing on tiles)
- **Operations**: Split/Join/Forward (dataflow transformations)

```
     ┌──────────┐
     │  Shim    │ (Tile)
     │  (0, 0)  │
     └────┬─────┘
          │ ObjectFifo (edge)
          │
     ┌────▼─────┐
     │   Mem    │ (Tile)
     │  (0, 1)  │
     └────┬─────┘
          │ Split (operation)
      ┌───┴───┐
      │       │
  ┌───▼──┐ ┌──▼───┐
  │Worker│ │Worker│ (on Compute tiles)
  │  0   │ │  1   │
  └──────┘ └──────┘
```

## Design Principles

1. **XML Independence**: No XML concerns in the core model. XML is just one possible serialization format.

2. **Type Safety**: Rich type system with tensor shapes and dtypes. Symbolic expressions for runtime dimensions.

3. **Fluent API**: Builder pattern with method chaining for ergonomic construction.

4. **Invariant Enforcement**: Builders validate invariants (unique names, valid references, etc.).

5. **Graph Semantics**: Explicit graph structure makes dataflow clear and analyzable.

6. **Extensibility**: Easy to add new operations, tile types, or serialization formats.

7. **Testability**: Pure Python objects are easy to test without XML parsing.

## Testing

Run the comprehensive test suite:

```bash
# Run all tests
python -m aiecad.aiecad_compiler.hlir.test_hlir

# Run specific test class
python -m aiecad.aiecad_compiler.hlir.test_hlir TestBuilder

# Run examples
python -m aiecad.aiecad_compiler.hlir.examples
```

Test coverage includes:
- Type system (DataType, TensorType, symbolic shapes)
- Core classes (Program, Tile, ObjectFifo, Worker, etc.)
- Builder API (all construction methods)
- Operations (Split, Join, Forward, TAP)
- Serialization (XML output validation)
- Complete examples (passthrough, add, multi-worker)

## Integration

### From GUI/Frontend

```python
from aiecad_compiler.hlir import ProgramBuilder, XMLSerializer

def gui_to_hlir(gui_design):
    """Convert GUI design to HLIR."""
    builder = ProgramBuilder(gui_design.name)

    # Add tiles from GUI
    for gui_tile in gui_design.tiles:
        builder.add_tile(
            name=gui_tile.id,
            kind=gui_tile.type,
            x=gui_tile.x,
            y=gui_tile.y
        )

    # Add FIFOs from connections
    for connection in gui_design.connections:
        builder.add_fifo(
            name=connection.id,
            obj_type=connection.data_type,
            depth=2
        )

    # ... etc

    return builder.build()

def hlir_to_xml(program):
    """Serialize HLIR to XML."""
    serializer = XMLSerializer()
    return serializer.serialize(program)

# Usage
gui_design = load_from_gui()
program = gui_to_hlir(gui_design)
xml = hlir_to_xml(program)
save_xml(xml, "output.xml")
```

### To Code Generation

```python
from aiecad_compiler.hlir import XMLSerializer
from aiecad_compiler.graph_builder.graph_driver import GraphDriver
from aiecad_compiler.graph_builder.code_generator import CodeGenerator

def hlir_to_python(program):
    """Generate Python code from HLIR."""
    # Serialize to XML
    serializer = XMLSerializer()
    xml_str = serializer.serialize(program)

    # Save to temp file
    import tempfile
    with tempfile.NamedTemporaryFile(mode='w', suffix='.xml', delete=False) as f:
        f.write(xml_str)
        xml_path = f.name

    # Use existing pipeline
    driver = GraphDriver(xml_path)
    graph = driver.build_graph()

    codegen = CodeGenerator(graph)
    python_code = codegen.generate()

    return python_code
```

## API Reference

See individual module docstrings for detailed API documentation:

- `types.py`: Type system classes
- `core.py`: Core semantic model classes
- `operations.py`: Operation classes
- `builder.py`: Fluent builder API
- `serializer.py`: XML serialization

## Future Extensions

Potential future enhancements:

1. **JSON Serialization**: Alternative to XML for web/REST APIs
2. **Protobuf Serialization**: For efficient binary representation
3. **GraphML Export**: Direct graph export without XML intermediate
4. **Visualization**: Graph visualization using networkx/graphviz
5. **Optimization Passes**: HLIR-level optimizations before code generation
6. **Type Inference**: Automatic type inference for reduced boilerplate
7. **Validation Rules**: More sophisticated validation (resource usage, etc.)
8. **Template Library**: Pre-built templates for common patterns

## License

Copyright (c) 2025 AIECAD Project. All rights reserved.
