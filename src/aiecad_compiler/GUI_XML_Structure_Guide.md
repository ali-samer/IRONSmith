# GUI XML Structure Guide

This guide explains how to construct GUI XML files for the AIECAD Compiler based on the actual `add_activate_gui.xml` implementation.

---

## What is GUI XML?

GUI XML is a simplified format that automatically expands to complete XML:
- **Simplified expressions**: Write `data_size / 4` instead of complex calculations
- **Automatic naming**: Context attributes generate semantic names
- **Configurable complexity**: `use_tap` flag controls TensorAccessPattern generation
- **GUI-friendly**: Designed for visual tool generation

**Pipeline:** `GUI XML ’ XMLGenerator ’ Complete XML ’ GraphDriver ’ Python Code`

---

## Module Structure

```xml
<?xml version='1.0' encoding='UTF-8'?>
<Module name="add_activatejit">
    <Symbols>
        <!-- Constants and types -->
    </Symbols>
    <DataFlow>
        <!-- Functions, ObjectFifos, Workers, Runtime -->
    </DataFlow>
    <Function name="jit_function" decorator="iron.jit" entry="base_aaa">
        <!-- JIT function -->
    </Function>
    <Function name="main_function" entry="main">
        <!-- Main function -->
    </Function>
    <EntryPoint>
        <!-- Entry point -->
    </EntryPoint>
</Module>
```

---

## Symbols Section

### Constants

```xml
<Const name="data_size" type="int">128</Const>
<TypeDef name="datatype">bfloat16</TypeDef>
```

### Type Abstractions

```xml
<TypeAbstraction name="data_ty" context="full_data">
    <ndarray>
        <shape>data_size</shape>
        <dtype>bfloat16</dtype>
    </ndarray>
</TypeAbstraction>

<TypeAbstraction name="chunk_ty" context="column_chunk">
    <ndarray>
        <shape>data_size / 4</shape>
        <dtype>bfloat16</dtype>
    </ndarray>
</TypeAbstraction>

<TypeAbstraction name="worker_chunk_ty" context="worker_chunk">
    <ndarray>
        <shape>data_size / 8</shape>
        <dtype>bfloat16</dtype>
    </ndarray>
</TypeAbstraction>
```

**Expression Expansion:**
- `data_size` ’ `inputA.numel()`
- `data_size / 4` ’ `(inputA.numel() // 4)`
- `data_size / 8` ’ `(inputA.numel() // 8)`

---

## DataFlow Section

### External Functions

Declare C/C++ kernel functions:

```xml
<ExternalFunction name="externalfunc1" operation="element_wise_add">
    <kernel>eltwise_add_bf16_scalar</kernel>
    <source>../../../aie_kernels/aie2/add.cc</source>
    <arg_types>
        <type>worker_chunk_ty</type>
        <type>worker_chunk_ty</type>
        <type>worker_chunk_ty</type>
    </arg_types>
</ExternalFunction>
```

### Core Functions

Python wrappers with acquire/release semantics:

```xml
<CoreFunction name="corefunc1" operation="eltwise_add">
    <parameters>
        <param name="kernel" role="external_function"/>
        <param name="inputA" role="consumer"/>
        <param name="inputB" role="consumer"/>
        <param name="outputC" role="producer"/>
    </parameters>
    <body>
        <Acquire source="inputA" count="1" name="elementA"/>
        <Acquire source="inputB" count="1" name="elementB"/>
        <Acquire source="outputC" count="1" name="elementC"/>
        <Call function="kernel" args="elementA, elementB, elementC"/>
        <Release source="inputA" count="1"/>
        <Release source="inputB" count="1"/>
        <Release source="outputC" count="1"/>
    </body>
</CoreFunction>
```

### ObjectFifos

#### Input/Output FIFOs (L3”L2)

```xml
<!-- Input from system memory -->
<ObjectFifo name="of_in_a_col0" context="L3_L2" direction="input" data="A" column="0">
    <type>chunk_ty</type>
    <depth>2</depth>
</ObjectFifo>

<!-- Output to system memory -->
<ObjectFifo name="of_out_d_col0" context="L2_L3" direction="output" data="D" column="0">
    <type>chunk_ty</type>
    <depth>2</depth>
</ObjectFifo>
```

**Generates:** `SHIM_L3_L2_A1A2_col0` and `SHIM_L2_L3_D1D2_col0`

#### Intermediate FIFOs (L1”L1)

```xml
<ObjectFifo name="of_inter_1" context="L1_L1" direction="intermediate" stage="add_to_relu" worker="1">
    <type>worker_chunk_ty</type>
    <depth>2</depth>
</ObjectFifo>
```

**Generates:** `L1_L1_add_to_relu_1`

### ObjectFifo Operations

#### Split

```xml
<ObjectFifoSplit name="split_a_col0" context="L2_L1" data="A" column="0">
    <source>of_in_a_col0</source>
    <num_outputs>2</num_outputs>
    <output_type>worker_chunk_ty</output_type>
    <placement>Tile(0, 1)</placement>
</ObjectFifoSplit>
```

**Generates:**
```python
MEM_L2_L1_A1A2_col0 = SHIM_L3_L2_A1A2_col0.cons().split(
    obj_types=[worker_chunk_ty, worker_chunk_ty],
    offsets=[0, (data_size // 8)],
    names=["MEM_L2_L1_A1A2_col0[0]", "MEM_L2_L1_A1A2_col0[1]"],
    placement=Tile(0, 1)
)
```

#### Join

```xml
<ObjectFifoJoin name="join_d_col0" context="L1_L2" data="D" column="0">
    <dest>of_out_d_col0</dest>
    <num_inputs>2</num_inputs>
    <input_type>worker_chunk_ty</input_type>
    <placement>Tile(0, 1)</placement>
</ObjectFifoJoin>
```

### Workers

```xml
<Worker name="worker_add_col0_w0" operation="add" column="0" worker_index="0">
    <core_function>corefunc1</core_function>
    <arguments>
        <arg ref="externalfunc1"/>
        <arg ref="split_a_col0" index="0" mode="consumer"/>
        <arg ref="split_b_col0" index="0" mode="consumer"/>
        <arg ref="of_inter_1" mode="producer"/>
    </arguments>
    <placement>Tile(0, 5)</placement>
</Worker>
```

**Pattern:** `worker_{operation}_col{column}_w{index}`

### Runtime

```xml
<Runtime name="runtime">
    <Sequence inputs="chunk_ty, chunk_ty, chunk_ty" as="A, B, D">
        <Start>
            <workers>
                worker_add_col0_w0, worker_add_col0_w1,
                worker_add_col1_w0, worker_add_col1_w1,
                ...
            </workers>
        </Start>

        <!-- Fill with TensorAccessPattern -->
        <Fill target="of_in_a_col0" source="A" column="0" use_tap="true">
            <placement>Tile(0, 0)</placement>
        </Fill>

        <!-- Drain with TensorAccessPattern -->
        <Drain source="of_out_d_col0" target="D" column="0" use_tap="true">
            <placement>Tile(0, 0)</placement>
            <wait>true</wait>
        </Drain>
    </Sequence>
</Runtime>
```

#### use_tap Flag

**use_tap="true"** (multi-column with TensorAccessPattern):
```python
rt.fill(
    placement=Tile(0, 0),
    in_fifo=SHIM_L3_L2_A1A2_col0.prod(),
    source=A,
    tap=TensorAccessPattern(...)
)
```

**use_tap="false"** (single-column simple):
```python
rt.fill(of_in.prod(), a_in)
```

### Program

```xml
<Program name="program">
    <device>current_device</device>
    <runtime>runtime</runtime>
    <placer>SequentialPlacer</placer>
</Program>
```

---

## Functions

### JIT Function

```xml
<Function name="jit_function" decorator="iron.jit" entry="base_aaa">
    <parameters>
        <param name="inputA" type="data_ty"/>
        <param name="inputB" type="data_ty"/>
        <param name="outputD" type="data_ty"/>
    </parameters>
    <body>
        <UseDataFlow/>
        <Return>program</Return>
    </body>
</Function>
```

### Main Function

```xml
<Function name="main_function" entry="main">
    <body>
        <Assign name="datatype" value="bfloat16"/>
        <Assign name="data_size" value="128"/>

        <Tensor name="inputA">
            <init>iron.arange(data_size, dtype=datatype, device="npu")</init>
        </Tensor>

        <Tensor name="inputB">
            <init>iron.arange(data_size, dtype=datatype, device="npu")</init>
        </Tensor>

        <Tensor name="outputD">
            <init>iron.zeros(data_size, dtype=datatype, device="npu")</init>
        </Tensor>

        <Call function="jit_function" args="inputA, inputB, outputD"/>
    </body>
</Function>
```

---

## Entry Point

```xml
<EntryPoint>
    <If condition="__name__ == '__main__'">
        <Call function="main_function"/>
    </If>
</EntryPoint>
```

---

## Multi-Column Pattern (4 columns × 2 workers)

### For Each Column (0-3):

**1. Input ObjectFifos**
```xml
<ObjectFifo name="of_in_a_colN" context="L3_L2" ...>
<ObjectFifo name="of_in_b_colN" context="L3_L2" ...>
```

**2. Split Operations**
```xml
<ObjectFifoSplit name="split_a_colN" context="L2_L1" ...>
<ObjectFifoSplit name="split_b_colN" context="L2_L1" ...>
```

**3. Intermediate FIFOs**
```xml
<ObjectFifo name="of_inter_X" context="L1_L1" ...>
<!-- X = colN*2 + workerIdx + 1 -->
```

**4. Workers (2 per column)**
```xml
<Worker name="worker_add_colN_w0" ...>
<Worker name="worker_add_colN_w1" ...>
<Worker name="worker_relu_colN_w0" ...>
<Worker name="worker_relu_colN_w1" ...>
```

**5. Join Operations**
```xml
<ObjectFifoJoin name="join_d_colN" context="L1_L2" ...>
```

**6. Output ObjectFifos**
```xml
<ObjectFifo name="of_out_d_colN" context="L2_L3" ...>
```

**7. Runtime Fill/Drain**
```xml
<Fill target="of_in_a_colN" source="A" column="N" use_tap="true">
<Fill target="of_in_b_colN" source="B" column="N" use_tap="true">
<Drain source="of_out_d_colN" target="D" column="N" use_tap="true">
```

---

## Context-Based Naming

| Context | Template | Example | Purpose |
|---------|----------|---------|---------|
| L3_L2 | `SHIM_L3_L2_{data}{workers}_col{column}` | `SHIM_L3_L2_A1A2_col0` | System’Shared input |
| L2_L3 | `SHIM_L2_L3_{data}{workers}_col{column}` | `SHIM_L2_L3_D1D2_col0` | Shared’System output |
| L2_L1 | `MEM_L2_L1_{data}{workers}_col{column}` | `MEM_L2_L1_A1A2_col0` | Split outputs |
| L1_L2 | `MEM_L1_L2_{data}{workers}_col{column}` | `MEM_L1_L2_D1D2_col0` | Join inputs |
| L1_L1 | `L1_L1_{stage}_{worker}` | `L1_L1_add_to_relu_1` | Inter-core |

---

## Worker Numbering & Tile Placement

### Worker Numbers
```
Column 0: Workers 1-2 (of_inter_1, of_inter_2)
Column 1: Workers 3-4 (of_inter_3, of_inter_4)
Column 2: Workers 5-6 (of_inter_5, of_inter_6)
Column 3: Workers 7-8 (of_inter_7, of_inter_8)
```

### Tile Layout (Column 0 example)
```
Row 0: Tile(0, 0) - Interface (Fill/Drain)
Row 1: Tile(0, 1) - Memory (Split/Join)
Row 2: Tile(0, 2) - worker_relu_col0_w1
Row 3: Tile(0, 3) - worker_add_col0_w1
Row 4: Tile(0, 4) - worker_relu_col0_w0
Row 5: Tile(0, 5) - worker_add_col0_w0
```

---

## Best Practices

### Type Definitions
 Use expressions: `data_size / 4`, not hardcoded values
 Add `context` for documentation
 Name by purpose: `chunk_ty`, `worker_chunk_ty`

### ObjectFifos
 Always provide context attributes
 Use consistent `data` identifiers (`A`, `B`, `D`)
 Match column numbers across pipeline

### Workers
 Follow naming: `worker_{op}_col{col}_w{idx}`
 Place systematically across tiles
 Use `index` attribute for split/join array access

### Runtime
 Use `use_tap="true"` for multi-column
 Use `use_tap="false"` for single-column
 Always set `column` when using TensorAccessPattern
 Place operations on interface tiles (row 0)

### Multi-Column
 Replicate pattern across all columns
 Maintain systematic worker numbering
 Match fill/drain columns
 Balance computation evenly

---

## Examples

**Simple Passthrough:**
`examples/applications/passthrough2/passthrough_gui.xml`
- Single column, no workers
- ObjectFifoForward
- use_tap="false"

**Add-Activate (4-column):**
`examples/applications/add_activate2/add_activate_gui.xml`
- 4 columns × 2 workers = 8 workers per stage
- 2 stages (add + relu) = 16 total workers
- Split/Join operations
- use_tap="true" for all Fill/Drain

---

## Resources

- **README**: [README.md](README.md) - Overview and quick start
- **Complete XML Guide**: [XML_Structure_Guide.md](XML_Structure_Guide.md) - Advanced format
- **XMLGenerator Code**: [graph_builder/XMLGenerator.py](graph_builder/XMLGenerator.py) - Implementation
