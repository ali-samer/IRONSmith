# AIECAD XML Structure Guide

This document provides the complete XML structure specification for generating IRON/AIE code. Each section shows the XML format, explains the parameters, and provides examples.

---

## Table of Contents

1. [ObjectFifo](#objectfifo)
2. [Split Operation](#split-operation)
3. [Join Operation](#join-operation)
4. [ExternalFunction](#externalfunction)
5. [Core Functions (core_fn)](#core-functions-core_fn)
6. [Worker](#worker)
7. [Runtime Fill Operation](#runtime-fill-operation)
8. [Runtime Drain Operation](#runtime-drain-operation)
9. [Method Chains](#method-chains)
10. [Complete Examples](#complete-examples)

---

## ObjectFifo

**Purpose**: Defines data movement channels between memory hierarchies in the AIE array.

**Generated Code Format**:
```python
of_name = ObjectFifo(obj_type=type_expr, depth=N, name="fifo_name")
```

**XML Structure**:
```xml
<object_fifo name="of_name">
    <type>
        <var ref="type_expr"/>
    </type>
    <kwargs>
        <kwarg name="depth">
            <const>2</const>
        </kwarg>
        <kwarg name="name">
            <string>"fifo_name"</string>
        </kwarg>
    </kwargs>
</object_fifo>
```

**Parameters**:
- `name` (attribute): Variable name for the ObjectFifo
- `<type>`: Reference to a type variable (e.g., `chunk_a`, `line_ty`)
- `<kwarg name="depth">`: Number of buffers (typically 2)
- `<kwarg name="name">`: String identifier for the FIFO

**Example**:
```xml
<object_fifo name="SHIM_L3_L2_A1A2_col0">
    <type>
        <var ref="chunk_a"/>
    </type>
    <kwargs>
        <kwarg name="depth">
            <const>2</const>
        </kwarg>
        <kwarg name="name">
            <string>"SHIM_L3_L2_A1A2_col0"</string>
        </kwarg>
    </kwargs>
</object_fifo>
```

**Generates**:
```python
SHIM_L3_L2_A1A2_col0 = ObjectFifo(obj_type=chunk_a, depth=2, name="SHIM_L3_L2_A1A2_col0")
```

---

## Split Operation

**Purpose**: Splits an ObjectFifo consumer into multiple output FIFOs with different offsets.

**Generated Code Format**:
```python
result = base_fifo.cons().split(
    obj_types=[type1, type2],
    offsets=[offset1, offset2],
    names=["name1", "name2"],
    placement=Tile(x, y)
)
```

**XML Structure**:
```xml
<object_fifo name="result_name">
    <method_chain>
        <base>
            <var ref="base_fifo"/>
        </base>
        <call>
            <method name="cons"/>
        </call>
        <call>
            <method name="split">
                <kwargs>
                    <kwarg name="obj_types">
                        <list>
                            <var ref="type1"/>
                            <var ref="type2"/>
                        </list>
                    </kwarg>
                    <kwarg name="offsets">
                        <list>
                            <binary_op op="*">
                                <lhs>
                                    <binary_op op="//">
                                        <lhs>
                                            <method_chain>
                                                <base><var ref="inputA"/></base>
                                                <call><method name="numel"/></call>
                                            </method_chain>
                                        </lhs>
                                        <rhs><const>8</const></rhs>
                                    </binary_op>
                                </lhs>
                                <rhs><const>0</const></rhs>
                            </binary_op>
                            <binary_op op="*">
                                <lhs>
                                    <binary_op op="//">
                                        <lhs>
                                            <method_chain>
                                                <base><var ref="inputA"/></base>
                                                <call><method name="numel"/></call>
                                            </method_chain>
                                        </lhs>
                                        <rhs><const>8</const></rhs>
                                    </binary_op>
                                </lhs>
                                <rhs><const>1</const></rhs>
                            </binary_op>
                        </list>
                    </kwarg>
                    <kwarg name="names">
                        <list>
                            <string>"MEM_L2_L1_A1_col0"</string>
                            <string>"MEM_L2_L1_A2_col0"</string>
                        </list>
                    </kwarg>
                    <kwarg name="placement">
                        <constructor name="Tile">
                            <args>
                                <const>0</const>
                                <const>1</const>
                            </args>
                        </constructor>
                    </kwarg>
                </kwargs>
            </method>
        </call>
    </method_chain>
</object_fifo>
```

**Parameters**:
- `obj_types`: List of type variables for each split output
- `offsets`: List of expressions calculating byte offsets for each output
- `names`: List of string names for each split FIFO
- `placement`: Tile constructor with (x, y) coordinates

**Example**:
```xml
<object_fifo name="MEM_L2_L1_A1A2_col0">
    <method_chain>
        <base>
            <var ref="SHIM_L3_L2_A1A2_col0"/>
        </base>
        <call>
            <method name="cons"/>
        </call>
        <call>
            <method name="split">
                <kwargs>
                    <kwarg name="obj_types">
                        <list>
                            <var ref="chunk_a_worker"/>
                            <var ref="chunk_a_worker"/>
                        </list>
                    </kwarg>
                    <kwarg name="offsets">
                        <list>
                            <binary_op op="*">
                                <lhs>
                                    <binary_op op="//">
                                        <lhs>
                                            <method_chain>
                                                <base><var ref="inputA"/></base>
                                                <call><method name="numel"/></call>
                                            </method_chain>
                                        </lhs>
                                        <rhs><const>8</const></rhs>
                                    </binary_op>
                                </lhs>
                                <rhs><const>0</const></rhs>
                            </binary_op>
                            <binary_op op="*">
                                <lhs>
                                    <binary_op op="//">
                                        <lhs>
                                            <method_chain>
                                                <base><var ref="inputA"/></base>
                                                <call><method name="numel"/></call>
                                            </method_chain>
                                        </lhs>
                                        <rhs><const>8</const></rhs>
                                    </binary_op>
                                </lhs>
                                <rhs><const>1</const></rhs>
                            </binary_op>
                        </list>
                    </kwarg>
                    <kwarg name="names">
                        <list>
                            <string>"MEM_L2_L1_A1_col0"</string>
                            <string>"MEM_L2_L1_A2_col0"</string>
                        </list>
                    </kwarg>
                    <kwarg name="placement">
                        <constructor name="Tile">
                            <args>
                                <const>0</const>
                                <const>1</const>
                            </args>
                        </constructor>
                    </kwarg>
                </kwargs>
            </method>
        </call>
    </method_chain>
</object_fifo>
```

**Generates**:
```python
MEM_L2_L1_A1A2_col0 = SHIM_L3_L2_A1A2_col0.cons().split(
    obj_types=[chunk_a_worker, chunk_a_worker],
    offsets=[((inputA.numel() // 8) * 0), ((inputA.numel() // 8) * 1)],
    names=["MEM_L2_L1_A1_col0", "MEM_L2_L1_A2_col0"],
    placement=Tile(0, 1)
)
```

---

## Join Operation

**Purpose**: Joins multiple input FIFOs into a single ObjectFifo producer with different offsets.

**Generated Code Format**:
```python
result = base_fifo.prod().join(
    obj_types=[type1, type2],
    names=["name1", "name2"],
    placement=Tile(x, y),
    offsets=[offset1, offset2]
)
```

**XML Structure**:
```xml
<object_fifo name="result_name">
    <method_chain>
        <base>
            <var ref="base_fifo"/>
        </base>
        <call>
            <method name="prod"/>
        </call>
        <call>
            <method name="join">
                <kwargs>
                    <kwarg name="obj_types">
                        <list>
                            <var ref="type1"/>
                            <var ref="type2"/>
                        </list>
                    </kwarg>
                    <kwarg name="names">
                        <list>
                            <string>"name1"</string>
                            <string>"name2"</string>
                        </list>
                    </kwarg>
                    <kwarg name="placement">
                        <constructor name="Tile">
                            <args>
                                <const>0</const>
                                <const>1</const>
                            </args>
                        </constructor>
                    </kwarg>
                    <kwarg name="offsets">
                        <list>
                            <binary_op op="*">
                                <lhs>
                                    <binary_op op="//">
                                        <lhs>
                                            <method_chain>
                                                <base><var ref="outputD"/></base>
                                                <call><method name="numel"/></call>
                                            </method_chain>
                                        </lhs>
                                        <rhs><const>8</const></rhs>
                                    </binary_op>
                                </lhs>
                                <rhs><const>0</const></rhs>
                            </binary_op>
                            <binary_op op="*">
                                <lhs>
                                    <binary_op op="//">
                                        <lhs>
                                            <method_chain>
                                                <base><var ref="outputD"/></base>
                                                <call><method name="numel"/></call>
                                            </method_chain>
                                        </lhs>
                                        <rhs><const>8</const></rhs>
                                    </binary_op>
                                </lhs>
                                <rhs><const>1</const></rhs>
                            </binary_op>
                        </list>
                    </kwarg>
                </kwargs>
            </method>
        </call>
    </method_chain>
</object_fifo>
```

**Parameters**:
- `obj_types`: List of type variables for each join input
- `names`: List of string names for each join FIFO
- `placement`: Tile constructor with (x, y) coordinates
- `offsets`: List of expressions calculating byte offsets for each input

**Example**:
```xml
<object_fifo name="MEM_L1_L2_D1D2_col0">
    <method_chain>
        <base>
            <var ref="SHIM_L2_L3_D1D2_col0"/>
        </base>
        <call>
            <method name="prod"/>
        </call>
        <call>
            <method name="join">
                <kwargs>
                    <kwarg name="obj_types">
                        <list>
                            <var ref="chunk_d_worker"/>
                            <var ref="chunk_d_worker"/>
                        </list>
                    </kwarg>
                    <kwarg name="names">
                        <list>
                            <string>"MEM_L1_L2_D1_col0"</string>
                            <string>"MEM_L1_L2_D2_col0"</string>
                        </list>
                    </kwarg>
                    <kwarg name="placement">
                        <constructor name="Tile">
                            <args>
                                <const>0</const>
                                <const>1</const>
                            </args>
                        </constructor>
                    </kwarg>
                    <kwarg name="offsets">
                        <list>
                            <binary_op op="*">
                                <lhs>
                                    <binary_op op="//">
                                        <lhs>
                                            <method_chain>
                                                <base><var ref="outputD"/></base>
                                                <call><method name="numel"/></call>
                                            </method_chain>
                                        </lhs>
                                        <rhs><const>8</const></rhs>
                                    </binary_op>
                                </lhs>
                                <rhs><const>0</const></rhs>
                            </binary_op>
                            <binary_op op="*">
                                <lhs>
                                    <binary_op op="//">
                                        <lhs>
                                            <method_chain>
                                                <base><var ref="outputD"/></base>
                                                <call><method name="numel"/></call>
                                            </method_chain>
                                        </lhs>
                                        <rhs><const>8</const></rhs>
                                    </binary_op>
                                </lhs>
                                <rhs><const>1</const></rhs>
                            </binary_op>
                        </list>
                    </kwarg>
                </kwargs>
            </method>
        </call>
    </method_chain>
</object_fifo>
```

**Generates**:
```python
MEM_L1_L2_D1D2_col0 = SHIM_L2_L3_D1D2_col0.prod().join(
    obj_types=[chunk_d_worker, chunk_d_worker],
    names=["MEM_L1_L2_D1_col0", "MEM_L1_L2_D2_col0"],
    placement=Tile(0, 1),
    offsets=[((outputD.numel() // 8) * 0), ((outputD.numel() // 8) * 1)]
)
```

---

## ExternalFunction

**Purpose**: Declares an external C/C++ kernel function to be called from AIE workers.

**Generated Code Format**:
```python
func_name = ExternalFunction(
    name="kernel_name",
    source_file="path/to/kernel.cc",
    arg_types=[type1, type2, type3],
    include_dirs=["path/to/includes"]
)
```

**XML Structure**:
```xml
<external_function name="func_name">
    <kwargs>
        <kwarg name="name">
            <string>"kernel_name"</string>
        </kwarg>
        <kwarg name="source_file">
            <string>"../../../aie_kernels/aie2/add.cc"</string>
        </kwarg>
        <kwarg name="arg_types">
            <list>
                <var ref="chunk_a_worker"/>
                <var ref="chunk_b_worker"/>
                <var ref="chunk_d_worker"/>
            </list>
        </kwarg>
        <kwarg name="include_dirs">
            <list>
                <string>"/scratch/andrewa/mlir-aie/aie_kernels/"</string>
            </list>
        </kwarg>
    </kwargs>
</external_function>
```

**Parameters**:
- `name` (attribute): Variable name for the ExternalFunction
- `<kwarg name="name">`: String name of the kernel function in C/C++ code
- `<kwarg name="source_file">`: Relative or absolute path to kernel source file
- `<kwarg name="arg_types">`: List of type variables matching kernel signature
- `<kwarg name="include_dirs">`: List of include directory paths (optional)

**Example**:
```xml
<external_function name="element_wise_add">
    <kwargs>
        <kwarg name="name">
            <string>"eltwise_add_bf16_scalar"</string>
        </kwarg>
        <kwarg name="source_file">
            <string>"../../../aie_kernels/aie2/add.cc"</string>
        </kwarg>
        <kwarg name="arg_types">
            <list>
                <var ref="chunk_a_worker"/>
                <var ref="chunk_b_worker"/>
                <var ref="chunk_d_worker"/>
            </list>
        </kwarg>
        <kwarg name="include_dirs">
            <list>
                <string>"/scratch/andrewa/mlir-aie/aie_kernels/"</string>
            </list>
        </kwarg>
    </kwargs>
</external_function>
```

**Generates**:
```python
element_wise_add = ExternalFunction(
    name="eltwise_add_bf16_scalar",
    source_file="../../../aie_kernels/aie2/add.cc",
    arg_types=[chunk_a_worker, chunk_b_worker, chunk_d_worker],
    include_dirs=["/scratch/andrewa/mlir-aie/aie_kernels/"]
)
```

---

## Core Functions (core_fn)

**Purpose**: Defines Python functions that specify AIE kernel behavior with FIFO acquire/release semantics.

**Generated Code Format**:
```python
def func_name(external_func, input1, input2, output):
    elem1 = input1.acquire(1)
    elem2 = input2.acquire(1)
    elem_out = output.acquire(1)
    external_func(elem1, elem2, elem_out)
    input1.release(1)
    input2.release(1)
    output.release(1)
```

**XML Structure**:
```xml
<CoreFunction name="func_name">
    <parameters>
        <param name="external_func"/>
        <param name="input1"/>
        <param name="input2"/>
        <param name="output"/>
    </parameters>
    <body>
        <Acquire name="elem1">
            <call>
                <method ref="input1" name="acquire">
                    <arg><const>1</const></arg>
                </method>
            </call>
        </Acquire>
        <Acquire name="elem2">
            <call>
                <method ref="input2" name="acquire">
                    <arg><const>1</const></arg>
                </method>
            </call>
        </Acquire>
        <Acquire name="elem_out">
            <call>
                <method ref="output" name="acquire">
                    <arg><const>1</const></arg>
                </method>
            </call>
        </Acquire>
        <Call>
            <function ref="external_func">
                <arg><var ref="elem1"/></arg>
                <arg><var ref="elem2"/></arg>
                <arg><var ref="elem_out"/></arg>
            </function>
        </Call>
        <Release>
            <call>
                <method ref="input1" name="release">
                    <arg><const>1</const></arg>
                </method>
            </call>
        </Release>
        <Release>
            <call>
                <method ref="input2" name="release">
                    <arg><const>1</const></arg>
                </method>
            </call>
        </Release>
        <Release>
            <call>
                <method ref="output" name="release">
                    <arg><const>1</const></arg>
                </method>
            </call>
        </Release>
    </body>
</CoreFunction>
```

**Parameters**:
- `name` (attribute): Function name
- `<parameters>`: List of parameter names (first is typically the ExternalFunction reference, rest are FIFOs)
- `<body>`: Function body containing:
  - `<Acquire>` elements: Call `.acquire(N)` on input/output FIFOs and bind to local variables
  - `<Call>` element: Invoke the external function with acquired elements
  - `<Release>` elements: Call `.release(N)` on input/output FIFOs

**Acquire Element Structure**:
```xml
<Acquire name="local_var_name">
    <call>
        <method ref="fifo_param" name="acquire">
            <arg><const>N</const></arg>
        </method>
    </call>
</Acquire>
```

**Release Element Structure**:
```xml
<Release>
    <call>
        <method ref="fifo_param" name="release">
            <arg><const>N</const></arg>
        </method>
    </call>
</Release>
```

**Call Element Structure**:
```xml
<Call>
    <function ref="external_func_ref">
        <arg><var ref="acquired_var1"/></arg>
        <arg><var ref="acquired_var2"/></arg>
    </function>
</Call>
```

**Example 1: Element-wise Add**
```xml
<CoreFunction name="eltwise_add">
    <parameters>
        <param name="element_wise_add"/>
        <param name="inputA"/>
        <param name="inputB"/>
        <param name="outputC"/>
    </parameters>
    <body>
        <Acquire name="elementA">
            <call>
                <method ref="inputA" name="acquire">
                    <arg><const>1</const></arg>
                </method>
            </call>
        </Acquire>
        <Acquire name="elementB">
            <call>
                <method ref="inputB" name="acquire">
                    <arg><const>1</const></arg>
                </method>
            </call>
        </Acquire>
        <Acquire name="elementC">
            <call>
                <method ref="outputC" name="acquire">
                    <arg><const>1</const></arg>
                </method>
            </call>
        </Acquire>
        <Call>
            <function ref="element_wise_add">
                <arg><var ref="elementA"/></arg>
                <arg><var ref="elementB"/></arg>
                <arg><var ref="elementC"/></arg>
            </function>
        </Call>
        <Release>
            <call>
                <method ref="inputA" name="release">
                    <arg><const>1</const></arg>
                </method>
            </call>
        </Release>
        <Release>
            <call>
                <method ref="inputB" name="release">
                    <arg><const>1</const></arg>
                </method>
            </call>
        </Release>
        <Release>
            <call>
                <method ref="outputC" name="release">
                    <arg><const>1</const></arg>
                </method>
            </call>
        </Release>
    </body>
</CoreFunction>
```

**Generates**:
```python
def eltwise_add(element_wise_add, inputA, inputB, outputC):
        elementA = inputA.acquire(1)
        elementB = inputB.acquire(1)
        elementC = outputC.acquire(1)
        element_wise_add(elementA, elementB, elementC)
        inputA.release(1)
        inputB.release(1)
        outputC.release(1)
```

**Example 2: ReLU Activation**
```xml
<CoreFunction name="relu">
    <parameters>
        <param name="relu_activation"/>
        <param name="inputC"/>
        <param name="outputD"/>
    </parameters>
    <body>
        <Acquire name="elementC">
            <call>
                <method ref="inputC" name="acquire">
                    <arg><const>1</const></arg>
                </method>
            </call>
        </Acquire>
        <Acquire name="elementD">
            <call>
                <method ref="outputD" name="acquire">
                    <arg><const>1</const></arg>
                </method>
            </call>
        </Acquire>
        <Call>
            <function ref="relu_activation">
                <arg><var ref="elementC"/></arg>
                <arg><var ref="elementD"/></arg>
            </function>
        </Call>
        <Release>
            <call>
                <method ref="inputC" name="release">
                    <arg><const>1</const></arg>
                </method>
            </call>
        </Release>
        <Release>
            <call>
                <method ref="outputD" name="release">
                    <arg><const>1</const></arg>
                </method>
            </call>
        </Release>
    </body>
</CoreFunction>
```

**Generates**:
```python
def relu(relu_activation, inputC, outputD):
        elementC = inputC.acquire(1)
        elementD = outputD.acquire(1)
        relu_activation(elementC, elementD)
        inputC.release(1)
        outputD.release(1)
```

**Important Notes**:
- The first parameter is typically the ExternalFunction reference
- Remaining parameters are FIFO references (from Worker fn_args)
- Each FIFO input/output must be acquired before use and released after
- The acquire count (typically 1) must match the release count
- Acquires come before the kernel call, releases come after
- The acquired elements (local variables) are passed to the external function, not the FIFOs themselves

---

## Worker

**Purpose**: Assigns a core function to execute on a specific AIE tile with specific FIFOs.

**Generated Code Format**:
```python
worker_name = Worker(
    core_fn=func_name,
    fn_args=[fifo1.cons(), fifo2.cons(), fifo3.prod()],
    placement=Tile(x, y)
)
```

**XML Structure**:
```xml
<worker name="worker_name">
    <kwargs>
        <kwarg name="core_fn">
            <var ref="func_name"/>
        </kwarg>
        <kwarg name="fn_args">
            <list>
                <method_chain>
                    <base><var ref="fifo1"/></base>
                    <call><method name="cons"/></call>
                </method_chain>
                <method_chain>
                    <base><var ref="fifo2"/></base>
                    <call><method name="cons"/></call>
                </method_chain>
                <method_chain>
                    <base><var ref="fifo3"/></base>
                    <call><method name="prod"/></call>
                </method_chain>
            </list>
        </kwarg>
        <kwarg name="placement">
            <constructor name="Tile">
                <args>
                    <const>0</const>
                    <const>2</const>
                </args>
            </constructor>
        </kwarg>
    </kwargs>
</worker>
```

**Parameters**:
- `name` (attribute): Variable name for the Worker
- `<kwarg name="core_fn">`: Reference to the core function to execute
- `<kwarg name="fn_args">`: List of FIFO method chains (cons/prod) to pass as arguments
- `<kwarg name="placement">`: Tile constructor with (x, y) coordinates

**Example**:
```xml
<worker name="A1_B1_worker">
    <kwargs>
        <kwarg name="core_fn">
            <var ref="add"/>
        </kwarg>
        <kwarg name="fn_args">
            <list>
                <method_chain>
                    <base>
                        <subscript>
                            <base><var ref="MEM_L2_L1_A1A2_col0"/></base>
                            <index><const>0</const></index>
                        </subscript>
                    </base>
                    <call><method name="cons"/></call>
                </method_chain>
                <method_chain>
                    <base>
                        <subscript>
                            <base><var ref="MEM_L2_L1_B1B2_col0"/></base>
                            <index><const>0</const></index>
                        </subscript>
                    </base>
                    <call><method name="cons"/></call>
                </method_chain>
                <method_chain>
                    <base><var ref="L1_L1_elwiseadd_relu_1"/></base>
                    <call><method name="prod"/></call>
                </method_chain>
            </list>
        </kwarg>
        <kwarg name="placement">
            <constructor name="Tile">
                <args>
                    <const>0</const>
                    <const>2</const>
                </args>
            </constructor>
        </kwarg>
    </kwargs>
</worker>
```

**Generates**:
```python
A1_B1_worker = Worker(
    core_fn=add,
    fn_args=[MEM_L2_L1_A1A2_col0[0].cons(), MEM_L2_L1_B1B2_col0[0].cons(), L1_L1_elwiseadd_relu_1.prod()],
    placement=Tile(0, 2)
)
```

**Note on Subscripting**:
For split/join results that create multiple FIFOs, use subscript notation:
```xml
<subscript>
    <base><var ref="MEM_L2_L1_A1A2_col0"/></base>
    <index><const>0</const></index>
</subscript>
```

---

## Runtime Fill Operation

**Purpose**: DMA operation that fills data from host memory into an AIE ObjectFifo.

**Generated Code Format**:
```python
rt.fill(
    placement=Tile(x, y),
    in_fifo=fifo.prod(),
    source=var,
    tap=TensorAccessPattern(
        tensor_dims=[dim],
        offset=offset_expr,
        sizes=[size1, size2],
        strides=[stride1, stride2]
    )
)
```

**XML Structure**:
```xml
<operation type="fill">
    <kwargs>
        <kwarg name="placement">
            <constructor name="Tile">
                <args>
                    <const>0</const>
                    <const>0</const>
                </args>
            </constructor>
        </kwarg>
        <kwarg name="in_fifo">
            <var ref="SHIM_L3_L2_A1A2_col0"/>
            <method name="prod"/>
        </kwarg>
        <kwarg name="source">
            <var ref="A"/>
        </kwarg>
        <kwarg name="tap">
            <constructor name="TensorAccessPattern">
                <kwargs>
                    <kwarg name="tensor_dims">
                        <list>
                            <method_chain>
                                <base><var ref="inputA"/></base>
                                <call><method name="numel"/></call>
                            </method_chain>
                        </list>
                    </kwarg>
                    <kwarg name="offset">
                        <binary_op op="*">
                            <lhs>
                                <binary_op op="//">
                                    <lhs>
                                        <method_chain>
                                            <base><var ref="inputA"/></base>
                                            <call><method name="numel"/></call>
                                        </method_chain>
                                    </lhs>
                                    <rhs><const>4</const></rhs>
                                </binary_op>
                            </lhs>
                            <rhs><const>0</const></rhs>
                        </binary_op>
                    </kwarg>
                    <kwarg name="sizes">
                        <list>
                            <binary_op op="//">
                                <lhs>
                                    <binary_op op="//">
                                        <lhs>
                                            <method_chain>
                                                <base><var ref="inputA"/></base>
                                                <call><method name="numel"/></call>
                                            </method_chain>
                                        </lhs>
                                        <rhs><const>4</const></rhs>
                                    </binary_op>
                                </lhs>
                                <rhs>
                                    <binary_op op="//">
                                        <lhs>
                                            <method_chain>
                                                <base><var ref="inputA"/></base>
                                                <call><method name="numel"/></call>
                                            </method_chain>
                                        </lhs>
                                        <rhs><const>8</const></rhs>
                                    </binary_op>
                                </rhs>
                            </binary_op>
                            <binary_op op="//">
                                <lhs>
                                    <method_chain>
                                        <base><var ref="inputA"/></base>
                                        <call><method name="numel"/></call>
                                    </method_chain>
                                </lhs>
                                <rhs><const>8</const></rhs>
                            </binary_op>
                        </list>
                    </kwarg>
                    <kwarg name="strides">
                        <list>
                            <binary_op op="//">
                                <lhs>
                                    <method_chain>
                                        <base><var ref="inputA"/></base>
                                        <call><method name="numel"/></call>
                                    </method_chain>
                                </lhs>
                                <rhs><const>8</const></rhs>
                            </binary_op>
                            <const>1</const>
                        </list>
                    </kwarg>
                </kwargs>
            </constructor>
        </kwarg>
    </kwargs>
    <target>
        <method ref="rt" name="fill"/>
    </target>
</operation>
```

**Parameters**:
- `<kwarg name="placement">`: Tile constructor specifying SHIM tile (typically y=0)
- `<kwarg name="in_fifo">`: Target FIFO with `.prod()` method (use `<var>` + `<method>` pattern)
- `<kwarg name="source">`: Runtime sequence parameter variable
- `<kwarg name="tap">`: TensorAccessPattern constructor with:
  - `tensor_dims`: Total tensor dimensions as list
  - `offset`: Starting offset expression
  - `sizes`: Multi-dimensional access sizes
  - `strides`: Multi-dimensional access strides
- `<target>`: Method call `rt.fill`

**Example**:
```xml
<operation type="fill">
    <kwargs>
        <kwarg name="placement">
            <constructor name="Tile">
                <args>
                    <const>0</const>
                    <const>0</const>
                </args>
            </constructor>
        </kwarg>
        <kwarg name="in_fifo">
            <var ref="SHIM_L3_L2_A1A2_col0"/>
            <method name="prod"/>
        </kwarg>
        <kwarg name="source">
            <var ref="A"/>
        </kwarg>
        <kwarg name="tap">
            <constructor name="TensorAccessPattern">
                <kwargs>
                    <kwarg name="tensor_dims">
                        <list>
                            <method_chain>
                                <base><var ref="inputA"/></base>
                                <call><method name="numel"/></call>
                            </method_chain>
                        </list>
                    </kwarg>
                    <kwarg name="offset">
                        <binary_op op="*">
                            <lhs>
                                <binary_op op="//">
                                    <lhs>
                                        <method_chain>
                                            <base><var ref="inputA"/></base>
                                            <call><method name="numel"/></call>
                                        </method_chain>
                                    </lhs>
                                    <rhs><const>4</const></rhs>
                                </binary_op>
                            </lhs>
                            <rhs><const>0</const></rhs>
                        </binary_op>
                    </kwarg>
                    <kwarg name="sizes">
                        <list>
                            <binary_op op="//">
                                <lhs>
                                    <binary_op op="//">
                                        <lhs>
                                            <method_chain>
                                                <base><var ref="inputA"/></base>
                                                <call><method name="numel"/></call>
                                            </method_chain>
                                        </lhs>
                                        <rhs><const>4</const></rhs>
                                    </binary_op>
                                </lhs>
                                <rhs>
                                    <binary_op op="//">
                                        <lhs>
                                            <method_chain>
                                                <base><var ref="inputA"/></base>
                                                <call><method name="numel"/></call>
                                            </method_chain>
                                        </lhs>
                                        <rhs><const>8</const></rhs>
                                    </binary_op>
                                </rhs>
                            </binary_op>
                            <binary_op op="//">
                                <lhs>
                                    <method_chain>
                                        <base><var ref="inputA"/></base>
                                        <call><method name="numel"/></call>
                                    </method_chain>
                                </lhs>
                                <rhs><const>8</const></rhs>
                            </binary_op>
                        </list>
                    </kwarg>
                    <kwarg name="strides">
                        <list>
                            <binary_op op="//">
                                <lhs>
                                    <method_chain>
                                        <base><var ref="inputA"/></base>
                                        <call><method name="numel"/></call>
                                    </method_chain>
                                </lhs>
                                <rhs><const>8</const></rhs>
                            </binary_op>
                            <const>1</const>
                        </list>
                    </kwarg>
                </kwargs>
            </constructor>
        </kwarg>
    </kwargs>
    <target>
        <method ref="rt" name="fill"/>
    </target>
</operation>
```

**Generates**:
```python
rt.fill(
    placement=Tile(0, 0),
    in_fifo=SHIM_L3_L2_A1A2_col0.prod(),
    source=A,
    tap=TensorAccessPattern(
        tensor_dims=[inputA.numel()],
        offset=((inputA.numel() // 4) * 0),
        sizes=[((inputA.numel() // 4) // (inputA.numel() // 8)), (inputA.numel() // 8)],
        strides=[(inputA.numel() // 8), 1]
    )
)
```

**Important Notes**:
- The `in_fifo` kwarg uses a special pattern: `<var>` followed by `<method>` (not a method_chain)
- This represents the pattern `object.method()` where the method is called on the variable

---

## Runtime Drain Operation

**Purpose**: DMA operation that drains data from an AIE ObjectFifo back to host memory.

**Generated Code Format**:
```python
rt.drain(
    placement=Tile(x, y),
    out_fifo=fifo.cons(),
    dest=var,
    wait=True,
    tap=TensorAccessPattern(
        tensor_dims=[dim],
        offset=offset_expr,
        sizes=[size1, size2],
        strides=[stride1, stride2]
    )
)
```

**XML Structure**:
```xml
<operation type="drain">
    <kwargs>
        <kwarg name="placement">
            <constructor name="Tile">
                <args>
                    <const>0</const>
                    <const>0</const>
                </args>
            </constructor>
        </kwarg>
        <kwarg name="out_fifo">
            <var ref="SHIM_L2_L3_D1D2_col0"/>
            <method name="cons"/>
        </kwarg>
        <kwarg name="dest">
            <var ref="D"/>
        </kwarg>
        <kwarg name="wait">
            <const>True</const>
        </kwarg>
        <kwarg name="tap">
            <constructor name="TensorAccessPattern">
                <kwargs>
                    <kwarg name="tensor_dims">
                        <list>
                            <method_chain>
                                <base><var ref="outputD"/></base>
                                <call><method name="numel"/></call>
                            </method_chain>
                        </list>
                    </kwarg>
                    <kwarg name="offset">
                        <binary_op op="*">
                            <lhs>
                                <binary_op op="//">
                                    <lhs>
                                        <method_chain>
                                            <base><var ref="outputD"/></base>
                                            <call><method name="numel"/></call>
                                        </method_chain>
                                    </lhs>
                                    <rhs><const>4</const></rhs>
                                </binary_op>
                            </lhs>
                            <rhs><const>0</const></rhs>
                        </binary_op>
                    </kwarg>
                    <kwarg name="sizes">
                        <list>
                            <binary_op op="//">
                                <lhs>
                                    <binary_op op="//">
                                        <lhs>
                                            <method_chain>
                                                <base><var ref="outputD"/></base>
                                                <call><method name="numel"/></call>
                                            </method_chain>
                                        </lhs>
                                        <rhs><const>4</const></rhs>
                                    </binary_op>
                                </lhs>
                                <rhs>
                                    <binary_op op="//">
                                        <lhs>
                                            <method_chain>
                                                <base><var ref="outputD"/></base>
                                                <call><method name="numel"/></call>
                                            </method_chain>
                                        </lhs>
                                        <rhs><const>8</const></rhs>
                                    </binary_op>
                                </rhs>
                            </binary_op>
                            <binary_op op="//">
                                <lhs>
                                    <method_chain>
                                        <base><var ref="outputD"/></base>
                                        <call><method name="numel"/></call>
                                    </method_chain>
                                </lhs>
                                <rhs><const>8</const></rhs>
                            </binary_op>
                        </list>
                    </kwarg>
                    <kwarg name="strides">
                        <list>
                            <binary_op op="//">
                                <lhs>
                                    <method_chain>
                                        <base><var ref="outputD"/></base>
                                        <call><method name="numel"/></call>
                                    </method_chain>
                                </lhs>
                                <rhs><const>8</const></rhs>
                            </binary_op>
                            <const>1</const>
                        </list>
                    </kwarg>
                </kwargs>
            </constructor>
        </kwarg>
    </kwargs>
    <target>
        <method ref="rt" name="drain"/>
    </target>
</operation>
```

**Parameters**:
- `<kwarg name="placement">`: Tile constructor specifying SHIM tile (typically y=0)
- `<kwarg name="out_fifo">`: Source FIFO with `.cons()` method (use `<var>` + `<method>` pattern)
- `<kwarg name="dest">`: Runtime sequence parameter variable
- `<kwarg name="wait">`: Boolean constant (True or False)
- `<kwarg name="tap">`: TensorAccessPattern constructor (same structure as fill)
- `<target>`: Method call `rt.drain`

**Example**:
```xml
<operation type="drain">
    <kwargs>
        <kwarg name="placement">
            <constructor name="Tile">
                <args>
                    <const>0</const>
                    <const>0</const>
                </args>
            </constructor>
        </kwarg>
        <kwarg name="out_fifo">
            <var ref="SHIM_L2_L3_D1D2_col0"/>
            <method name="cons"/>
        </kwarg>
        <kwarg name="dest">
            <var ref="D"/>
        </kwarg>
        <kwarg name="wait">
            <const>True</const>
        </kwarg>
        <kwarg name="tap">
            <constructor name="TensorAccessPattern">
                <kwargs>
                    <kwarg name="tensor_dims">
                        <list>
                            <method_chain>
                                <base><var ref="outputD"/></base>
                                <call><method name="numel"/></call>
                            </method_chain>
                        </list>
                    </kwarg>
                    <kwarg name="offset">
                        <binary_op op="*">
                            <lhs>
                                <binary_op op="//">
                                    <lhs>
                                        <method_chain>
                                            <base><var ref="outputD"/></base>
                                            <call><method name="numel"/></call>
                                        </method_chain>
                                    </lhs>
                                    <rhs><const>4</const></rhs>
                                </binary_op>
                            </lhs>
                            <rhs><const>0</const></rhs>
                        </binary_op>
                    </kwarg>
                    <kwarg name="sizes">
                        <list>
                            <binary_op op="//">
                                <lhs>
                                    <binary_op op="//">
                                        <lhs>
                                            <method_chain>
                                                <base><var ref="outputD"/></base>
                                                <call><method name="numel"/></call>
                                            </method_chain>
                                        </lhs>
                                        <rhs><const>4</const></rhs>
                                    </binary_op>
                                </lhs>
                                <rhs>
                                    <binary_op op="//">
                                        <lhs>
                                            <method_chain>
                                                <base><var ref="outputD"/></base>
                                                <call><method name="numel"/></call>
                                            </method_chain>
                                        </lhs>
                                        <rhs><const>8</const></rhs>
                                    </binary_op>
                                </rhs>
                            </binary_op>
                            <binary_op op="//">
                                <lhs>
                                    <method_chain>
                                        <base><var ref="outputD"/></base>
                                        <call><method name="numel"/></call>
                                    </method_chain>
                                </lhs>
                                <rhs><const>8</const></rhs>
                            </binary_op>
                        </list>
                    </kwarg>
                    <kwarg name="strides">
                        <list>
                            <binary_op op="//">
                                <lhs>
                                    <method_chain>
                                        <base><var ref="outputD"/></base>
                                        <call><method name="numel"/></call>
                                    </method_chain>
                                </lhs>
                                <rhs><const>8</const></rhs>
                            </binary_op>
                            <const>1</const>
                        </list>
                    </kwarg>
                </kwargs>
            </constructor>
        </kwarg>
    </kwargs>
    <target>
        <method ref="rt" name="drain"/>
    </target>
</operation>
```

**Generates**:
```python
rt.drain(
    placement=Tile(0, 0),
    out_fifo=SHIM_L2_L3_D1D2_col0.cons(),
    dest=D,
    wait=True,
    tap=TensorAccessPattern(
        tensor_dims=[outputD.numel()],
        offset=((outputD.numel() // 4) * 0),
        sizes=[((outputD.numel() // 4) // (outputD.numel() // 8)), (outputD.numel() // 8)],
        strides=[(outputD.numel() // 8), 1]
    )
)
```

---

## Method Chains

**Purpose**: Represents fluent API calls like `obj.method1().method2().method3()`.

**Generated Code Format**:
```python
result = base.method1().method2(arg).method3()
```

**XML Structure**:
```xml
<method_chain>
    <base>
        <var ref="base_object"/>
    </base>
    <call>
        <method name="method1"/>
    </call>
    <call>
        <method name="method2">
            <args>
                <var ref="arg"/>
            </args>
        </method>
    </call>
    <call>
        <method name="method3"/>
    </call>
</method_chain>
```

**Components**:
- `<base>`: The starting object (can be var, subscript, or another expression)
- `<call>`: Each method call in the chain
- `<method name="...">`: Method name
- `<args>`: Optional arguments (positional)
- `<kwargs>`: Optional keyword arguments

**Simple Example**:
```xml
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
```

**Generates**:
```python
of_in.cons().forward()
```

**Complex Example with Arguments**:
```xml
<method_chain>
    <base>
        <var ref="inputA"/>
    </base>
    <call>
        <method name="reshape">
            <args>
                <const>128</const>
                <const>128</const>
            </args>
        </method>
    </call>
    <call>
        <method name="transpose"/>
    </call>
</method_chain>
```

**Generates**:
```python
inputA.reshape(128, 128).transpose()
```

---

## Complete Examples

### Example 1: Simple Passthrough Application

**XML**:
```xml
<?xml version="1.0" encoding="UTF-8"?>
<program>
    <function name="passthrough_dmas_jit">
        <params>
            <param name="input_tensor"/>
            <param name="output_tensor"/>
        </params>
        <body>
            <!-- Type definitions -->
            <assign name="N">
                <method_chain>
                    <base><var ref="input_tensor"/></base>
                    <call><method name="numel"/></call>
                </method_chain>
            </assign>
            <assign name="line_size">
                <const>1024</const>
            </assign>

            <!-- ObjectFifos -->
            <object_fifo name="of_in">
                <type>
                    <var ref="line_ty"/>
                </type>
                <kwargs>
                    <kwarg name="name">
                        <string>"in"</string>
                    </kwarg>
                </kwargs>
            </object_fifo>

            <object_fifo name="of_out">
                <method_chain>
                    <base><var ref="of_in"/></base>
                    <call><method name="cons"/></call>
                    <call><method name="forward"/></call>
                </method_chain>
            </object_fifo>

            <!-- Runtime operations -->
            <operation type="fill">
                <kwargs>
                    <kwarg name="in_fifo">
                        <var ref="of_in"/>
                        <method name="prod"/>
                    </kwarg>
                    <kwarg name="source">
                        <var ref="a_in"/>
                    </kwarg>
                </kwargs>
                <target>
                    <method ref="rt" name="fill"/>
                </target>
            </operation>

            <operation type="drain">
                <kwargs>
                    <kwarg name="out_fifo">
                        <var ref="of_out"/>
                        <method name="cons"/>
                    </kwarg>
                    <kwarg name="dest">
                        <var ref="c_out"/>
                    </kwarg>
                    <kwarg name="wait">
                        <const>True</const>
                    </kwarg>
                </kwargs>
                <target>
                    <method ref="rt" name="drain"/>
                </target>
            </operation>
        </body>
    </function>
</program>
```

**Generates**:
```python
def passthrough_dmas_jit(input_tensor, output_tensor):
    N = input_tensor.numel()
    line_size = 1024

    of_in = ObjectFifo(obj_type=line_ty, name="in")
    of_out = of_in.cons().forward()

    rt = Runtime()
    with rt.sequence(vector_ty, vector_ty) as (a_in, c_out):
        rt.fill(of_in.prod(), a_in)
        rt.drain(of_out.cons(), c_out, wait=True)
```

---

### Example 2: Element-wise Add with ReLU Activation (Multi-Worker)

**XML Snippet**:
```xml
<!-- External kernel function -->
<external_function name="element_wise_add">
    <kwargs>
        <kwarg name="name">
            <string>"eltwise_add_bf16_scalar"</string>
        </kwarg>
        <kwarg name="source_file">
            <string>"../../../aie_kernels/aie2/add.cc"</string>
        </kwarg>
        <kwarg name="arg_types">
            <list>
                <var ref="chunk_a_worker"/>
                <var ref="chunk_b_worker"/>
                <var ref="chunk_d_worker"/>
            </list>
        </kwarg>
    </kwargs>
</external_function>

<!-- Core function -->
<CoreFunction name="eltwise_add">
    <parameters>
        <param name="element_wise_add"/>
        <param name="inputA"/>
        <param name="inputB"/>
        <param name="outputC"/>
    </parameters>
    <body>
        <Acquire name="elementA">
            <call>
                <method ref="inputA" name="acquire">
                    <arg><const>1</const></arg>
                </method>
            </call>
        </Acquire>
        <Acquire name="elementB">
            <call>
                <method ref="inputB" name="acquire">
                    <arg><const>1</const></arg>
                </method>
            </call>
        </Acquire>
        <Acquire name="elementC">
            <call>
                <method ref="outputC" name="acquire">
                    <arg><const>1</const></arg>
                </method>
            </call>
        </Acquire>
        <Call>
            <function ref="element_wise_add">
                <arg><var ref="elementA"/></arg>
                <arg><var ref="elementB"/></arg>
                <arg><var ref="elementC"/></arg>
            </function>
        </Call>
        <Release>
            <call>
                <method ref="inputA" name="release">
                    <arg><const>1</const></arg>
                </method>
            </call>
        </Release>
        <Release>
            <call>
                <method ref="inputB" name="release">
                    <arg><const>1</const></arg>
                </method>
            </call>
        </Release>
        <Release>
            <call>
                <method ref="outputC" name="release">
                    <arg><const>1</const></arg>
                </method>
            </call>
        </Release>
    </body>
</CoreFunction>

<!-- Worker -->
<worker name="A1_B1_worker">
    <kwargs>
        <kwarg name="core_fn">
            <var ref="eltwise_add"/>
        </kwarg>
        <kwarg name="fn_args">
            <list>
                <var ref="element_wise_add"/>
                <method_chain>
                    <base>
                        <subscript>
                            <base><var ref="MEM_L2_L1_A1A2_col0"/></base>
                            <index><const>0</const></index>
                        </subscript>
                    </base>
                    <call><method name="cons"/></call>
                </method_chain>
                <method_chain>
                    <base>
                        <subscript>
                            <base><var ref="MEM_L2_L1_B1B2_col0"/></base>
                            <index><const>0</const></index>
                        </subscript>
                    </base>
                    <call><method name="cons"/></call>
                </method_chain>
                <method_chain>
                    <base><var ref="L1_L1_elwiseadd_relu_1"/></base>
                    <call><method name="prod"/></call>
                </method_chain>
            </list>
        </kwarg>
        <kwarg name="placement">
            <constructor name="Tile">
                <args>
                    <const>0</const>
                    <const>2</const>
                </args>
            </constructor>
        </kwarg>
    </kwargs>
</worker>
```

**Generates**:
```python
element_wise_add = ExternalFunction(
    name="eltwise_add_bf16_scalar",
    source_file="../../../aie_kernels/aie2/add.cc",
    arg_types=[chunk_a_worker, chunk_b_worker, chunk_d_worker]
)

def eltwise_add(element_wise_add, inputA, inputB, outputC):
        elementA = inputA.acquire(1)
        elementB = inputB.acquire(1)
        elementC = outputC.acquire(1)
        element_wise_add(elementA, elementB, elementC)
        inputA.release(1)
        inputB.release(1)
        outputC.release(1)

A1_B1_worker = Worker(
    core_fn=eltwise_add,
    fn_args=[element_wise_add, MEM_L2_L1_A1A2_col0[0].cons(), MEM_L2_L1_B1B2_col0[0].cons(), L1_L1_elwiseadd_relu_1.prod()],
    placement=Tile(0, 2)
)
```

---

## Common XML Patterns

### Binary Operations
```xml
<binary_op op="*">
    <lhs>
        <binary_op op="//">
            <lhs><var ref="N"/></lhs>
            <rhs><const>8</const></rhs>
        </binary_op>
    </lhs>
    <rhs><const>0</const></rhs>
</binary_op>
```
Generates: `((N // 8) * 0)`

### Unary Operations
```xml
<unary_op op="~">
    <method_chain>
        <base><var ref="np"/></base>
        <call>
            <method name="isclose">
                <args>
                    <var ref="actual"/>
                    <var ref="expected"/>
                </args>
            </method>
        </call>
    </method_chain>
</unary_op>
```
Generates: `~np.isclose(actual, expected)`

### Subscript (Array Indexing)
```xml
<subscript>
    <base><var ref="array_name"/></base>
    <index><const>0</const></index>
</subscript>
```
Generates: `array_name[0]`

### Constructor Calls
```xml
<constructor name="Tile">
    <args>
        <const>0</const>
        <const>2</const>
    </args>
</constructor>
```
Generates: `Tile(0, 2)`

### String Literals
```xml
<string>"my_string_value"</string>
```
Generates: `"my_string_value"`

### Lists
```xml
<list>
    <var ref="item1"/>
    <const>42</const>
    <string>"text"</string>
</list>
```
Generates: `[item1, 42, "text"]`

---

## Important Notes for GUI Implementation

1. **Method Chain Base Element**: Always wrap the base object in a `<base>` element within `<method_chain>`.

2. **Fill/Drain in_fifo/out_fifo Pattern**: These kwargs use a special pattern:
   ```xml
   <kwarg name="in_fifo">
       <var ref="fifo_name"/>
       <method name="prod"/>
   </kwarg>
   ```
   This is NOT a method_chain but a var followed by a method element.

3. **Binary Operations**: Always have `<lhs>` and `<rhs>` children. Supported operators:
   - Arithmetic: `+`, `-`, `*`, `/`, `//`, `%`, `**`
   - Comparison: `==`, `!=`, `<`, `>`, `<=`, `>=`
   - Logical: `and`, `or`

4. **Unary Operations**: Have a single child representing the operand. Supported operators:
   - `~` (bitwise NOT / logical NOT)
   - `-` (negation)
   - `+` (positive)

5. **Constants**: Use `<const>` for numeric values and booleans:
   - Integers: `<const>42</const>`
   - Floats: `<const>3.14</const>`
   - Scientific notation: `<const>1e-3</const>`
   - Booleans: `<const>True</const>` or `<const>False</const>`

6. **Variable References**: Use `<var ref="name"/>` to reference previously defined variables.

7. **Nested Structures**: XML elements can be deeply nested. For example, TensorAccessPattern kwargs contain lists of binary operations containing method chains.

8. **Order Matters**: Elements should be defined in XML in the order they appear in code (top to bottom).

9. **Type References**: Types are referenced as variables, not as special constructs. Ensure type variables are assigned before ObjectFifos reference them.

10. **Subscript Access**: For split/join results that create indexed FIFOs, wrap in `<subscript>` before method chains.

11. **Core Function Pattern**: The standard pattern is:
    - First parameter: ExternalFunction reference
    - Remaining parameters: FIFO references (passed from Worker fn_args)
    - Body: Acquire all FIFOs → Call external function → Release all FIFOs
    - The first fn_arg in Worker must be the ExternalFunction reference

12. **Worker fn_args Order**: Must match CoreFunction parameters:
    - First: ExternalFunction variable reference
    - Rest: FIFO method chains matching the core function's FIFO parameters

---

## Validation Checklist

Before generating XML, verify:

- [ ] All variable references (`ref="..."`) point to previously defined variables
- [ ] Method chains have explicit `<base>` elements
- [ ] Binary operations have both `<lhs>` and `<rhs>`
- [ ] Constructors use `<args>` for positional arguments
- [ ] String literals are wrapped in `<string>` tags
- [ ] Numeric constants use `<const>` tags
- [ ] Fill/drain operations use the special var+method pattern for fifos
- [ ] Worker fn_args reference the correct FIFO methods (cons/prod)
- [ ] Worker fn_args first element is the ExternalFunction reference
- [ ] CoreFunction parameters match Worker fn_args in order
- [ ] Each Acquire has a matching Release in the CoreFunction
- [ ] Tile placements have valid (x, y) coordinates
- [ ] ExternalFunction arg_types match the data types being processed
- [ ] All kwargs have `name` attributes

---

## Support

For questions or issues with XML generation, please contact the AIECAD compiler team or refer to the example files:
- `examples/applications/passthrough/passthrough.xml`
- `examples/applications/add_activate/add_activate.xml`
