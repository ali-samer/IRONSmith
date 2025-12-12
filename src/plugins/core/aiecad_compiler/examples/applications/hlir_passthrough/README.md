# HLIR Passthrough Example - Complete End-to-End Workflow

This example demonstrates the complete HLIR workflow from design construction to code generation.

## Overview

This example shows how to:
1. **Build** a design using the HLIR ProgramBuilder (clean Python API)
2. **Serialize** to GUI XML format using GUIXMLSerializer
3. **Expand** GUI XML to Complete XML via XMLGenerator
4. **Process** with GraphDriver (Complete XML → Semantic Graph)
5. **Generate** Python code with CodeGenerator

## Architecture

**Simple Passthrough Design:**
```
Host Memory (a_in)
      ↓
   [FILL DMA]
      ↓
  ObjectFifo (of_in)
      ↓
  [.cons().forward()]
      ↓
  ObjectFifo (of_out)
      ↓
   [DRAIN DMA]
      ↓
Host Memory (c_out)
```

**Tiles:**
- 1× SHIM tile at (0, 0) for DMA operations

**Data Flow:**
- Input vector (4096 int32 elements) from host
- Pass through AIE array without processing
- Output vector (4096 int32 elements) back to host

## Files

```
hlir_passthrough/
├── README.md                          # This file
├── build_design.py                    # Build design using HLIR and serialize to GUI XML
├── passthrough_gui.xml                # Generated GUI XML (from build_design.py)
├── passthrough_complete.xml           # Generated Complete XML (from XMLGenerator)
├── passthrough.graphml                # Semantic graph (from GraphDriver)
└── generated_passthrough.py           # Generated Python code (from CodeGenerator)
```

## Usage

### Step 1: Build Design with HLIR

Build the design and generate GUI XML:

```bash
cd examples/applications/hlir_passthrough
python build_design.py
```

This will:
- Build the passthrough design using HLIR ProgramBuilder
- Serialize to `passthrough_gui.xml` using GUIXMLSerializer
- Print the generated GUI XML

### Step 2: Generate Code with Compiler Pipeline

Process the GUI XML through the complete pipeline:

```bash
# From aiecad_compiler directory
cd ../../..
python main.py examples/applications/hlir_passthrough/passthrough_gui.xml
```

This will:
1. **Expand** GUI XML to Complete XML via XMLGenerator
2. **Build** semantic graph with GraphDriver
3. **Generate** Python code with CodeGenerator
4. **Save** to `generated_passthrough.py`

### Complete Pipeline Flow

```
build_design.py (HLIR API)
         ↓
  passthrough_gui.xml (GUI XML)
         ↓
  XMLGenerator (expansion)
         ↓
  passthrough_complete.xml (Complete XML)
         ↓
  GraphDriver (graph building)
         ↓
  passthrough.graphml (semantic graph)
         ↓
  CodeGenerator (code generation)
         ↓
  generated_passthrough.py (executable Python code)
```

## Code Walkthrough

### Step 1: Build Design with HLIR

```python
from hlir import ProgramBuilder

# Create builder
builder = ProgramBuilder("passthrough_hlir")

# Add constants
builder.add_constant("N", 4096, "int")
builder.add_constant("line_size", 1024, "int")

# Add type definitions
builder.add_tensor_type("line_ty", shape=["line_size"], dtype="int32")
builder.add_tensor_type("vector_ty", shape=["N"], dtype="int32")

# Add tiles
builder.add_tile("shim0", kind="shim", x=0, y=0)

# Add FIFOs
builder.add_fifo("of_in", obj_type="line_ty", depth=2)
builder.add_fifo_forward("of_out", source="of_in")

# Create runtime
rt = builder.create_runtime("runtime")
rt.add_input_type("vector_ty")
rt.add_output_type("vector_ty")
rt.add_params(["a_in", "c_out"])
rt.add_fill("fill_0", "of_in", "a_in", "shim0")
rt.add_drain("drain_0", "of_out", "c_out", "shim0", wait=True)
rt.build()

# Build program
program = builder.build()
```

### Step 2: Serialize to XML

```python
from hlir import XMLSerializer

serializer = XMLSerializer(pretty_print=True)
serializer.serialize_to_file(program, "passthrough_hlir_complete.xml")
```

This generates complete XML in the format expected by GraphDriver.

### Step 3: Process with Compiler

The generated XML can be processed by the existing AIECAD compiler pipeline:

```python
from graph_builder.graph_driver import GraphDriver
from graph_builder.code_generator import CodeGenerator

# Parse XML and build graph
driver = GraphDriver("passthrough_hlir_complete.xml")
graph = driver.build_graph()

# Generate Python code
codegen = CodeGenerator(graph)
python_code = codegen.generate()
```

## Expected Output

### Generated XML Structure

```xml
<?xml version="1.0" ?>
<Module name="passthrough_hlir">
  <Symbols>
    <Const name="N" type="int">4096</Const>
    <Const name="line_size" type="int">1024</Const>
    <TypeAbstraction name="line_ty">
      <ndarray>
        <shape>
          <tuple>
            <expr>
              <var ref="line_size"/>
            </expr>
          </tuple>
        </shape>
        <dtype>
          <numpy_dtype>int32</numpy_dtype>
        </dtype>
      </ndarray>
    </TypeAbstraction>
    <!-- ... more types ... -->
  </Symbols>

  <DataFlow>
    <object_fifo name="of_in">
      <type>
        <var ref="line_ty"/>
      </type>
      <kwargs>
        <kwarg name="depth">
          <const>2</const>
        </kwarg>
        <kwarg name="name">
          <string>"of_in"</string>
        </kwarg>
      </kwargs>
    </object_fifo>

    <object_fifo name="of_out">
      <method_chain>
        <base>
          <var ref="of_in"/>
        </base>
        <call>
          <method name="cons"/>
        </call>
        <call>
          <method name="forward"/>
        </call>
      </method_chain>
    </object_fifo>

    <Runtime name="runtime">
      <!-- Runtime sequence with fill/drain operations -->
    </Runtime>
  </DataFlow>
</Module>
```

### Generated Python Code

```python
from aie.extras.context import mlir_mod_ctx
from aie.dialects.aie import *
from aie.dialects.aiex import *

# Type definitions
N = 4096
line_size = 1024
line_ty = np.dtype([("data", np.int32, (line_size,))])
vector_ty = np.dtype([("data", np.int32, (N,))])

# ObjectFifo declarations
of_in = ObjectFifo(obj_type=line_ty, depth=2, name="of_in")
of_out = of_in.cons().forward()

# Runtime sequence
rt = Runtime()
with rt.sequence(vector_ty, vector_ty) as (a_in, c_out):
    rt.fill(of_in.prod(), a_in)
    rt.drain(of_out.cons(), c_out, wait=True)
```

## Key Benefits Demonstrated

1. **Clean Python API**: No XML manipulation required
2. **Type Safety**: Tensor shapes and dtypes validated at build time
3. **Fluent Builder**: Method chaining for ergonomic construction
4. **Automatic Validation**: Catches errors before code generation
5. **XML Compatibility**: Seamless integration with existing pipeline
6. **Graph-Based**: Explicit dataflow representation

## Comparison: HLIR vs Manual XML

### HLIR Approach (This Example)
```python
builder = ProgramBuilder("passthrough")
builder.add_fifo("of_in", obj_type="line_ty", depth=2)
builder.add_fifo_forward("of_out", source="of_in")
program = builder.build()  # Validated!
```

### Manual XML Approach (Old Way)
```xml
<object_fifo name="of_in">
  <type>
    <var ref="line_ty"/>
  </type>
  <kwargs>
    <kwarg name="depth">
      <const>2</const>
    </kwarg>
    <kwarg name="name">
      <string>"of_in"</string>
    </kwarg>
  </kwargs>
</object_fifo>
<!-- No validation until runtime! -->
```

## Troubleshooting

### Import Errors

If you get import errors, make sure you're running from the correct directory:

```bash
cd examples/applications/hlir_passthrough
python build_design.py
```

### GraphDriver Not Found

If GraphDriver is not yet implemented, you can still:
1. Build the design with HLIR
2. Generate the XML
3. Inspect the XML structure manually

### Generated Code Not Working

The generated code requires the IRON runtime. Make sure:
- IRON is installed and configured
- NPU drivers are available
- You're running on compatible hardware

## Next Steps

After running this example, you can:

1. **Modify the design**: Edit `build_design.py` to add workers, kernels, etc.
2. **Try other examples**: See `hlir/examples.py` for add, multi-worker designs
3. **Build your own**: Use the HLIR API to create custom designs
4. **Integrate with GUI**: Use HLIR as the backend for visual design tools

## Related Documentation

- [HLIR README](../../../hlir/README.md) - Complete HLIR documentation
- [HLIR Examples](../../../hlir/examples.py) - More complex examples
- [XML Structure Guide](../../XML_Structure_Guide.md) - XML format reference
- [GUI XML Guide](../../GUI_XML_Structure_Guide.md) - GUI XML format

## License

Copyright (c) 2025 AIECAD Project. All rights reserved.
