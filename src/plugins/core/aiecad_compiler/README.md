# AIECAD Compiler

A flexible, extensible, graph-based code generation system for AMD's Ryzen AI NPU AIE programming using the IRON library.

**Key Features:**
- 🎯 **100% Functional Accuracy** - Generates correct code matching reference implementations
- 🖥️ **GUI-Friendly XML** - Simplified XML format designed for GUI generation
- 🔌 **Extensible Architecture** - Add new node types without modifying core code
- 📊 **Graph-Based** - Pure semantic graph representation, no hardcoded patterns
- 🔧 **Advanced AIE Support** - Workers, external functions, split/join, tensor access patterns
- 📖 **Comprehensive Documentation** - Complete XML specification with examples
- ✅ **Validated** - Tested on simple passthrough and complex multi-worker add-activate operations

## Overview

The AIECAD Compiler converts XML representations of IRON code into executable Python code. The compiler supports **two XML input formats**:

1. **GUI XML** (`*_gui.xml`): Simplified, user-friendly format designed for GUI generation
2. **Complete XML** (`*.xml`): Full specification format with all implementation details

### Code Generation Pipeline

```
GUI XML → XMLGenerator → Complete XML → GraphDriver → GraphML → CodeGenerator → Python Code
                                ↓
                          Semantic Graph
```

**Three-Stage Process:**

1. **XMLGenerator** (Optional - GUI XML only): Expands simplified GUI XML into complete specifications
2. **GraphDriver**: Converts complete XML to semantic graph (GraphML)
3. **CodeGenerator**: Generates Python code from semantic graph

The system is designed to be **completely flexible** and **highly extensible** - no code patterns are hardcoded. All code generation is driven purely by graph structure traversal, and new node types can be added without modifying core compiler code through the extension system.

## Architecture

```
XML Input → GraphDriver → Semantic Graph → CodeGenerator → Python Code
                ↑                               ↑
         GraphExtender                  CodeGeneratorExtender
           (Extensions)                     (Extensions)
```

### Key Components

#### Core System
- **[main.py](main.py)**: End-to-end orchestration (XML → Python)
- **[GraphDriver.py](graph_builder/GraphDriver.py)**: XML parser and graph builder
- **[CodeGenerator.py](codegen/backends/CodeGenerator.py)**: Graph traversal and code generation

#### Extension System (NEW)
- **[ExtensionManager.py](extension/ExtensionManager.py)**: Coordinates graph and codegen extensions
- **[GraphExtender.py](extension/GraphExtender.py)**: Extensible XML → graph conversion
- **[CodeGeneratorExtender.py](extension/CodeGeneratorExtender.py)**: Extensible graph → code generation

#### Documentation
- **[XML_Structure_Guide.md](XML_Structure_Guide.md)**: Complete XML structure specification
- **[extension/README.md](extension/README.md)**: Extension system documentation

## Quick Start

### Installation

```bash
# Install dependencies
pip install lxml networkx
```

### Basic Usage

```bash
# Generate from GUI XML (recommended for new projects)
python main.py examples/applications/passthrough2/passthrough_gui.xml

# Generate from complete XML (for advanced users)
python main.py examples/applications/passthrough/passthrough.xml

# Generate and execute
python main.py examples/applications/passthrough2/passthrough_gui.xml --run
```

**Output Files** (created in same directory as input XML):

When processing `passthrough_gui.xml`:
- `passthrough_complete.xml` - Expanded complete XML
- `passthrough.graphml` - Semantic graph (for debugging/visualization)
- `generated_passthrough.py` - Python code (ready to run)

When processing `passthrough.xml`:
- `passthrough.graphml` - Semantic graph
- `generated_passthrough.py` - Python code

### Quick Reference

| Task | Command |
|------|---------|
| Generate code | `python main.py <xml_file>` |
| Generate and run | `python main.py <xml_file> --run` |
| View help | `python main.py` |
| Run generated code | `python generated_<name>.py` |

### Examples

**Simple Passthrough (GUI XML - Recommended):**
```bash
# Generate from GUI-friendly XML
python main.py examples/applications/passthrough2/passthrough_gui.xml

# Generate and execute
python main.py examples/applications/passthrough2/passthrough_gui.xml --run
```

**Add-Activate (GUI XML with Advanced Features):**
```bash
# Uses Worker, ExternalFunction, CoreFunction, Split, Join with GUI XML
python main.py examples/applications/add_activate2/add_activate_gui.xml
```

**Complete XML Examples (Advanced Users):**
```bash
# Passthrough with complete XML
python main.py examples/applications/passthrough/passthrough.xml

# Add-Activate with complete XML
python main.py examples/applications/add_activate/add_activate.xml
```

**Example Output:**
```
======================================================================
IRON Code Generation System
======================================================================

[1/2] Building semantic graph from XML...
      Input: passthrough.xml
      Output: passthrough.graphml
      Graph: 178 nodes, 206 edges
      Node types: 39 unique types

[2/2] Generating Python code from graph...
      Input: passthrough.graphml
      Output: generated_passthrough.py
      Generated: 85 lines of Python code

======================================================================
[SUCCESS] Code generation completed successfully!
======================================================================
```

## GUI XML Format

### Overview

GUI XML provides a simplified, user-friendly format designed for GUI generation. The XMLGenerator automatically expands GUI XML into complete XML specifications that can be processed by the GraphDriver.

### Key Features

1. **Simplified Type Definitions** - Write `N` instead of `inputA.numel()`
2. **Context-Based Naming** - Automatic semantic name generation using templates
3. **Expression Expansion** - Simple expressions like `N / 4` expand to `(inputA.numel() // 4)`
4. **ObjectFifo Operations** - Simplified split, join, and forward operations
5. **TensorAccessPattern Control** - `use_tap` flag for complex vs simple DMA operations

### Simplified Type Definitions

**GUI XML:**
```xml
<TypeAbstraction name="vector_ty">
    <ndarray>
        <shape>N</shape>
        <dtype>int32</dtype>
    </ndarray>
</TypeAbstraction>
```

**Expands to Complete XML:**
```xml
<TypeAbstraction name="vector_ty">
    <ndarray>
        <shape>
            <tuple>
                <expr><method ref="inputA" name="numel"/></expr>
            </tuple>
        </shape>
        <dtype><numpy_dtype>int32</numpy_dtype></dtype>
    </ndarray>
</TypeAbstraction>
```

### Context-Based Naming

ObjectFifos with context attributes generate semantic names automatically:

**GUI XML:**
```xml
<ObjectFifo name="of_in_a" context="L3_L2" direction="input" data="A" column="0">
    <type>chunk_ty</type>
    <depth>2</depth>
</ObjectFifo>
```

**Generated Name:** `SHIM_L3_L2_A1A2_col0`

**Naming Templates:**

| Context | Template | Example |
|---------|----------|---------|
| L3_L2 | `SHIM_L3_L2_{data}{workers}_col{column}` | `SHIM_L3_L2_A1A2_col0` |
| L2_L3 | `SHIM_L2_L3_{data}{workers}_col{column}` | `SHIM_L2_L3_D1D2_col0` |
| L2_L1 | `MEM_L2_L1_{data}{workers}_col{column}` | `MEM_L2_L1_A1A2_col0` |
| L1_L2 | `MEM_L1_L2_{data}{workers}_col{column}` | `MEM_L1_L2_D1D2_col0` |
| L1_L1 | `L1_L1_{stage}_{worker}` | `L1_L1_add_to_relu_1` |

### ObjectFifo Operations

**Split Operation:**
```xml
<ObjectFifoSplit name="split_a" context="L2_L1" data="A" column="0">
    <source>of_in_a_col0</source>
    <num_outputs>2</num_outputs>
    <output_type>worker_chunk_ty</output_type>
    <placement>Tile(0, 1)</placement>
</ObjectFifoSplit>
```

**Join Operation:**
```xml
<ObjectFifoJoin name="join_d" context="L1_L2" data="D" column="0">
    <dest>of_out_d_col0</dest>
    <num_inputs>2</num_inputs>
    <input_type>worker_chunk_ty</input_type>
    <placement>Tile(0, 1)</placement>
</ObjectFifoJoin>
```

**Forward Operation:**
```xml
<ObjectFifoForward name="of_out" source="of_in"/>
```

Expands to: `of_out = of_in.cons().forward()`

### TensorAccessPattern Control

The `use_tap` attribute controls whether complex TensorAccessPattern is generated:

**Simple Form** (`use_tap="false"` - for single column):
```xml
<Fill target="of_in" source="a_in" use_tap="false">
    <placement>Tile(0, 0)</placement>
</Fill>
```

Generates:
```python
rt.fill(of_in.prod(), a_in)
```

**Complex Form** (`use_tap="true"` - for multi-column):
```xml
<Fill target="of_in_a_col0" source="A" column="0" use_tap="true">
    <placement>Tile(0, 0)</placement>
</Fill>
```

Generates:
```python
rt.fill(placement=Tile(0, 0),
        in_fifo=SHIM_L3_L2_A1A2_col0.prod(),
        source=A,
        tap=TensorAccessPattern(
            tensor_dims=[inputA.numel()],
            offset=((inputA.numel() // 4) * 0),
            sizes=[((inputA.numel() // 4) // (inputA.numel() // 8)), (inputA.numel() // 8)],
            strides=[(inputA.numel() // 8), 1]
        ))
```

### Expression Expansion

The XMLGenerator automatically expands simplified expressions:

| GUI Expression | Expanded Expression |
|----------------|---------------------|
| `N` | `inputA.numel()` |
| `N / 4` | `(inputA.numel() // 4)` |
| `N / 8` | `(inputA.numel() // 8)` |
| `data_size` | Resolved to actual value |

### GUI XML Examples

**Example 1: Passthrough (Simple)**
- Location: `examples/applications/passthrough2/passthrough_gui.xml`
- Features: Basic ObjectFifo forward, simple fill/drain
- Use case: Single-column data passthrough

**Example 2: Add-Activate (Complex)**
- Location: `examples/applications/add_activate2/add_activate_gui.xml`
- Features: Multi-column parallelism, split/join, TensorAccessPattern
- Use case: 4-column element-wise addition + ReLU activation

## System Design

### XMLGenerator Architecture (Stage 0)

**Purpose:** Transform GUI-friendly XML into complete XML specifications

**Key Components:**
- **NamingConventions**: Template-based naming for generated elements
- **ExpressionExpander**: Expands GUI expressions to full Python expressions
- **MethodChainBuilder**: Generates method chains for ObjectFifo operations
- **XMLTransformer**: Main orchestrator for transformation

**Transformation Process:**
1. Parse GUI XML
2. Expand type definitions
3. Generate semantic names from context
4. Build method chains for operations
5. Add TensorAccessPattern based on `use_tap` flag
6. Output complete XML

### GraphDriver Architecture

**Core System (11 Major Sections):**

1. **Node and Edge Management** - Core graph construction
2. **Symbol Table and Scope Management** - Variable/function scopes
3. **Main Build Process** - XML parsing orchestration
4. **Symbols Section Processing** - Imports, constants, types
5. **DataFlow Section Processing** - IRON runtime elements
6. **Method Chain and Call Processing** - Fluent API handling
7. **Function Processing** - Function definitions
8. **Statement Processing** - Assignments, assertions, comments
9. **Expression Processing** - Recursive expression trees
10. **Control Flow Processing** - If/else with branch separation
11. **CLI Entry Point** - Command-line interface

**Key Features:**
- Pure graph structure (no hardcoded logic)
- Complete information capture
- Flexible traversal
- Type-safe nodes and edges
- **Extensible via GraphExtender** - Add new XML node types without modifying core code

### CodeGenerator Architecture

**Core System (11 Major Sections):**

1. **Graph Navigation and Code Emission** - Core utilities
2. **Main Code Generation Entry Point** - Process orchestration
3. **Symbols Section Processing** - Import generation
4. **Function Processing** - Function definitions
5. **Statement Processing** - Statement dispatch
6. **Expression Reconstruction** - Graph to Python syntax
7. **Call Reconstruction** - Method chains and function calls
8. **Statement-Specific Processors** - Assert, assign, etc.
9. **Type Definition Generation** - Numpy types
10. **DataFlow Generation** - IRON runtime code
11. **Control Flow Processing** - If/else and loops

**Key Features:**
- Pure graph traversal (no hardcoded patterns)
- Complete code generation
- Proper indentation management
- Expression reconstruction
- **Extensible via CodeGeneratorExtender** - Add new code generation patterns without modifying core code

### Extension System Architecture (NEW)

The extension system enables adding new XML node types and code generation patterns without modifying core GraphDriver and CodeGenerator classes.

**Components:**

1. **ExtensionManager**: Coordinates both graph and codegen extensions
   - Provides unified registration interface
   - Automatically wires extensions into GraphDriver and CodeGenerator

2. **GraphExtender**: Handles XML → graph conversion for new node types
   - Base class: `GraphExtension`
   - Implement `process(elem, parent_nid)` method
   - Access to all GraphBuilder APIs (`_add_node`, `_link`, `_lookup`, etc.)

3. **CodeGeneratorExtender**: Handles graph → code generation for new node types
   - Base class: `CodeGenExtension`
   - Implement `generate(node_id)` method returning code string
   - Access to all CodeGenerator APIs (`_get_node_attr`, `_get_children`, etc.)

**Built-in Extensions:**

- **WorkerExtension** / **WorkerCodeGen**: AIE worker placement and execution
- **ExternalFunctionExtension** / **ExternalFunctionCodeGen**: External C/C++ kernel declarations
- **CoreFunctionExtension** / **CoreFunctionCodeGen**: AIE core functions with acquire/release semantics
- **ListExtension** / **ListCodeGen**: List declarations

See [extension/README.md](extension/README.md) for details on creating new extensions.

## XML Schema

The XML format captures all code elements. For complete XML structure specification including all supported elements, see [XML_Structure_Guide.md](XML_Structure_Guide.md).

### Core Structure

```xml
<Module name="module_name">
  <Symbols>
    <Import name="numpy" alias="np"/>
    <Const name="N">4096</Const>
    <TypeAbstraction name="vector_ty">
      <!-- Type definition -->
    </TypeAbstraction>
  </Symbols>

  <DataFlow>
    <ObjectFifo name="of_in">
      <!-- ObjectFifo definition -->
    </ObjectFifo>
    <Runtime name="rt"/>
    <SequenceBlock>
      <!-- Operations -->
    </SequenceBlock>
  </DataFlow>

  <Function name="main" decorator="iron.device">
    <parameters>
      <param name="device_name"/>
    </parameters>
    <body>
      <!-- Function statements -->
    </body>
  </Function>

  <EntryPoint>
    <!-- if __name__ == "__main__" -->
  </EntryPoint>
</Module>
```

### Core Elements

**Symbols:**
- Import (with optional alias)
- Const (constant values)
- TypeAbstraction (numpy types)

**DataFlow:**
- ObjectFifo (data buffers)
- Runtime (runtime operations)
- SequenceBlock (sequenced operations)
- Program (compiled program)
- Placer (placement strategy)

**Statements:**
- Assign (variable assignment)
- Assert (assertions with conditions)
- Call (function/method calls)
- If/Else (conditional branches)
- For (loops)
- Print (output)
- Comment (code comments)

**Expressions:**
- Variables (VarRef)
- Constants (ConstExpr)
- Binary operations (+, -, *, /, %)
- Comparisons (==, !=, <, >, <=, >=)
- Function calls
- Method calls
- Method chains (obj.method1().method2())
- Index expressions (arr[idx])

### Extension Elements (NEW)

These elements are supported via the extension system:

**AIE-Specific:**
- **Worker** - AIE worker with core function and tile placement
- **ExternalFunction** - External C/C++ kernel declarations
- **CoreFunction** - Python wrapper functions with acquire/release semantics
- **List** - List collections

**Advanced Operations:**
- **Split** - Split ObjectFifo into multiple outputs with offsets
- **Join** - Join multiple ObjectFifos into single output
- **TensorAccessPattern** - Complex DMA patterns with multi-dimensional slicing

See [XML_Structure_Guide.md](XML_Structure_Guide.md) for complete documentation with examples.

## Graph Structure

### Node Types

The semantic graph uses 40+ node types:

**Core:**
- Module, Section, Function, EntryPoint

**Symbols:**
- Import, Const, TypeAbstraction, TypeNode

**DataFlow:**
- ObjectFifo, Runtime, SequenceBlock, Program, Placer

**Statements:**
- Assign, Assert, Call, If, For, Print, Comment

**Expressions:**
- VarRef, ConstExpr, BinaryOp, ComparisonOp
- FunctionCall, MethodCall, IndexExpr, MethodChain

**Extension Nodes (NEW):**
- Worker, ExternalFunction, CoreFunction, List
- Acquire, Release (for core function semantics)
- ConstructorCall, TensorAccessPattern

### Edge Types

Edges define relationships:

**Core Edges:**
- `contains`: Structural containment
- `calls`: Function/method invocation
- `has_arg`: Function arguments
- `has_kwarg`: Keyword arguments
- `uses_type`: Type references
- `source`: Assignment source
- `condition`: Conditional expression
- `then`/`else`: Branch statements
- `nested_call`: Method chaining

**Extension Edges (NEW):**
- `core_fn`: Worker to core function reference
- `placed_by`: Tile placement reference
- `has_param`: Function parameters
- `base`: Base object in method chain
- `has_call`: Method calls in chain
- `object`: Method call target object

## Code Generation Process

### Stage 1: XML → Graph

```python
from GraphDriver import GraphBuilder

builder = GraphBuilder("input.xml")
graph = builder.build()
```

**Process:**
1. Parse XML with lxml
2. Create Module node
3. Process Symbols section
4. Process DataFlow section
5. Process Functions
6. Process EntryPoint
7. Save to GraphML

### Stage 2: Graph → Python

```python
from CodeGenerator import CodeGenerator

generator = CodeGenerator("input.graphml")
code = generator.generate()
```

**Process:**
1. Load graph from GraphML
2. Find Module node
3. Generate imports
4. Generate type definitions
5. Generate DataFlow elements
6. Generate functions
7. Generate entry point
8. Return complete code

## Validation

The system achieves **100% functional accuracy** on multiple reference implementations:

### Passthrough Example (Basic)
- **Target**: [passthroughjit.py](examples/applications/passthrough/passthroughjit.py) (94 lines)
- **Generated**: [generated_passthrough.py](examples/applications/passthrough/generated_passthrough.py) (85 lines)
- **Accuracy**: 100% (all logic, variables, method chains, arguments, branches, and comments correct)
- **Metrics**:
  - Graph: 178 nodes, 206 edges, 39 node types
  - All imports, types, functions, method chains, and control flow correct

### Add-Activate Example (Advanced with Extensions)
- **Target**: [add_activatejit.py](examples/applications/add_activate/add_activatejit.py)
- **Generated**: [generated_add_activate.py](examples/applications/add_activate/generated_add_activate.py)
- **Accuracy**: 100% (successfully generates complex multi-worker AIE code with element-wise addition and ReLU activation)
- **Features Demonstrated**:
  - Worker placement across multiple AIE tiles
  - ExternalFunction declarations with C++ kernels
  - CoreFunction definitions with acquire/release semantics
  - Split and Join operations with offsets
  - TensorAccessPattern for complex DMA operations
  - Multi-dimensional tensor slicing
  - Element-wise operations (addition) followed by activation (ReLU)

## Advanced Features

### Method Chaining

Handles complex method chains:

```python
of_out = of_in.cons().forward()
```

XML representation:
```xml
<source>
  <method_chain>
    <call>
      <method ref="of_in" name="cons"/>
    </call>
    <call>
      <method name="forward"/>
    </call>
  </method_chain>
</source>
```

### Complex Expressions

Handles nested expressions:

```python
assert N % line_size == 0, "N must be divisible by line_size"
```

XML representation:
```xml
<Assert>
  <condition>
    <comparison op="==">
      <left>
        <binary_op op="%">
          <left><var>N</var></left>
          <right><var>line_size</var></right>
        </binary_op>
      </left>
      <right><const>0</const></right>
    </comparison>
  </condition>
  <message>"N must be divisible by line_size"</message>
</Assert>
```

### Branch Separation

Uses distinct edge types for if/else branches:

- `then` edges: Statements in then branch
- `else` edges: Statements in else branch

This enables proper branch reconstruction without confusion.

## Extensibility

The system is designed to handle any IRON code pattern through the extension system:

### Adding New Extensions (Quick Guide)

1. **Create GraphExtension** in [GraphExtender.py](extension/GraphExtender.py):
   ```python
   @register_extension
   class MyNodeExtension(GraphExtension):
       tag = "mynode"  # XML tag name

       def process(self, elem, parent_nid):
           name = elem.get("name")
           node_id = self._add_node(name, "MyNode")
           self._link(parent_nid, node_id, "contains")
           self._declare_symbol(name, node_id)
           # Process attributes and children...
           return node_id
   ```

2. **Create CodeGenExtension** in [CodeGeneratorExtender.py](extension/CodeGeneratorExtender.py):
   ```python
   @register_codegen_extension
   class MyNodeCodeGen(CodeGenExtension):
       kind = "MyNode"  # Must match graph node kind

       def generate(self, node_id):
           name = self._get_node_attr(node_id, 'label')
           # Build code string...
           return f"{name} = MyNode(...)"
   ```

3. **No changes to core files needed** - Extensions auto-register!

### Key Benefits

- **Zero Core Modification**: Add new features without touching GraphDriver or CodeGenerator
- **Automatic Registration**: Extensions wire themselves into the pipeline
- **Full API Access**: Extensions have access to all builder/generator methods
- **Type Safety**: Extensions use the same node/edge system as core code
- **Isolated Testing**: Test extensions independently

See [extension/README.md](extension/README.md) for complete guide with examples.

## Debugging

### Visualize Graph

Use yEd or similar GraphML viewer:

```bash
# Generate graph
python main.py input.xml

# Open in yEd
yed input.graphml
```

### Inspect Graph

```python
import networkx as nx

graph = nx.read_graphml("input.graphml")
print(f"Nodes: {graph.number_of_nodes()}")
print(f"Edges: {graph.number_of_edges()}")

# Show node types
for node, data in graph.nodes(data=True):
    print(f"{node}: {data.get('kind')} - {data.get('label')}")
```

### Compare Output

```bash
# Generate code
python main.py passthrough.xml

# Compare with reference
diff generated_passthrough.py passthroughjit.py
```

## File Structure

```
aiecad_compiler/
├── main.py                                    # Main entry point
├── README.md                                  # This file
├── XML_Structure_Guide.md                     # Complete XML specification
│
├── graph_builder/                             # Graph generation
│   ├── __init__.py
│   └── GraphDriver.py                         # XML → Graph converter
│
├── codegen/                                   # Code generation
│   ├── __init__.py
│   └── backends/
│       ├── __init__.py
│       └── CodeGenerator.py                   # Graph → Python converter
│
├── extension/                                 # Extension system (NEW)
│   ├── __init__.py
│   ├── README.md                              # Extension documentation
│   ├── ExtensionManager.py                    # Coordinates extensions
│   ├── GraphExtender.py                       # XML → Graph extensions
│   └── CodeGeneratorExtender.py               # Graph → Code extensions
│
├── diagnostics/                               # Error handling (NEW)
│   ├── __init__.py
│   ├── diagnostics.py                         # Diagnostic system
│   └── codes.py                               # Error codes
│
├── backends/                                  # Backend support
│   └── __init__.py
│
└── examples/                                  # Example programs
    ├── __init__.py
    └── applications/
        ├── passthrough/                       # Basic example
        │   ├── passthrough.xml                # XML input
        │   ├── passthroughjit.py             # Reference implementation
        │   ├── generated_passthrough.py       # Generated output
        │   └── passthrough.graphml            # Semantic graph
        │
        └── add_activate/                       # Advanced example (NEW)
            ├── add_activate.xml               # XML input (element-wise add + ReLU)
            ├── add_activatejit.py            # Reference implementation
            ├── generated_add_activate.py      # Generated output
            └── add_activate.graphml           # Semantic graph
```

## Recent Changes

### Version 2.0 - Extension System (November 14, 2025)

**Major Features:**
- ✨ **Extension System**: Add new XML node types without modifying core code
  - GraphExtender for XML → graph conversion
  - CodeGeneratorExtender for graph → code generation
  - ExtensionManager for coordination
- 🔧 **Built-in Extensions**: Worker, ExternalFunction, CoreFunction, List
- 📊 **Advanced Operations**: Split, Join, TensorAccessPattern
- 📖 **Comprehensive Documentation**: XML_Structure_Guide.md with complete examples
- 🔍 **Diagnostics System**: Improved error reporting
- ✅ **Add-Activate Example**: Demonstrates complex multi-worker AIE code with element-wise operations

**Breaking Changes:**
- None - fully backward compatible with existing XML files

**Migration Guide:**
- Existing XML files work without changes
- New features available via extension elements (Worker, ExternalFunction, etc.)

## Contributing

When extending the system:

### Using Extensions (Recommended)
1. Create GraphExtension in [GraphExtender.py](extension/GraphExtender.py)
2. Create CodeGenExtension in [CodeGeneratorExtender.py](extension/CodeGeneratorExtender.py)
3. Register with decorators (`@register_extension`, `@register_codegen_extension`)
4. Test with example XML files
5. Add documentation to [XML_Structure_Guide.md](XML_Structure_Guide.md)

### Modifying Core (Only if necessary)
1. Add XML elements to schema
2. Create graph nodes in GraphDriver
3. Add code generation in CodeGenerator
4. Test with reference implementation
5. Validate 100% accuracy

## What's Next?

### For Users
1. **Start Simple**: Try the passthrough example to understand basic workflow
2. **Explore Advanced**: Check out the add-activate example for complex patterns
3. **Read Documentation**: See [XML_Structure_Guide.md](XML_Structure_Guide.md) for complete XML reference
4. **Create Your Own**: Write XML for your IRON code and generate Python

### For Contributors
1. **Understand Extensions**: Read [extension/README.md](extension/README.md)
2. **Study Examples**: Look at WorkerExtension and CoreFunctionExtension
3. **Create Extension**: Add support for new XML node types
4. **Test Thoroughly**: Validate against reference implementations
5. **Document**: Add examples to XML_Structure_Guide.md

### Resources
- **Core Code**: [GraphDriver.py](graph_builder/GraphDriver.py), [CodeGenerator.py](codegen/backends/CodeGenerator.py)
- **Extensions**: [GraphExtender.py](extension/GraphExtender.py), [CodeGeneratorExtender.py](extension/CodeGeneratorExtender.py)
- **Examples**: [passthrough](examples/applications/passthrough/), [add-activate](examples/applications/add_activate/)

## License

Copyright © 2025 Brock Sorenson

## References

- IRON Library: https://github.com/Xilinx/mlir-aie
- AMD Ryzen AI NPU: https://www.amd.com/en/products/ryzen-ai
- NetworkX: https://networkx.org/
- GraphML: http://graphml.graphdrawing.org/

## Author

Brock Sorenson
