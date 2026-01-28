# Extension System for AIECAD Compiler

This directory contains the extension system that allows adding new XML node types and code generation patterns without modifying the core `GraphDriver` and `CodeGenerator` classes.

## Architecture

The extension system consists of three main components:

### 1. GraphExtender (`GraphExtender.py`)
Handles conversion of new XML node types into graph nodes.

**How it works:**
- Define a class inheriting from `GraphExtension`
- Set the `tag` attribute to the XML tag name (lowercase)
- Implement `process(elem, parent_nid)` method
- Register with `@register_extension`

**Example:**
```python
@register_extension
class WorkerExtension(GraphExtension):
    tag = "worker"
    
    def process(self, elem, parent_nid):
        name = elem.get("name")
        worker_nid = self._add_node(name, "Worker")
        self._link(parent_nid, worker_nid, "contains")
        self._declare_symbol(name, worker_nid)
        # ... process attributes, children, etc.
        return worker_nid
```

### 2. CodeGeneratorExtender (`CodeGeneratorExtender.py`)
Handles code generation for new graph node types.

**How it works:**
- Define a class inheriting from `CodeGenExtension`
- Set the `kind` attribute to the node kind
- Implement `generate(node_id)` method returning code string
- Register with `@register_codegen_extension`

**Example:**
```python
@register_codegen_extension
class WorkerCodeGen(CodeGenExtension):
    kind = "Worker"
    
    def generate(self, node_id):
        name = self._get_node_attr(node_id, 'label')
        # ... build code string
        return f"{name} = Worker(...)"
```

### 3. ExtensionManager (`ExtensionManager.py`)
Coordinates both extension types and provides unified interface.

## Currently Supported Extensions

### Graph Extensions (XML → Graph)

1. **WorkerExtension** - Handles `<Worker>` nodes
   - Processes core_fn references
   - Handles fn_args with complex expressions (indexing, method calls)
   - Processes placement (Tile constructors)

2. **ExternalFunctionExtension** - Handles `<ExternalFunction>` nodes
   - Processes kwargs with lists (arg_types, include_dirs)
   - Handles string and type_ref children

3. **CoreFunctionExtension** - Handles `<CoreFunction>` nodes
   - Processes parameters
   - Handles body statements (Acquire, Release, Call)

4. **ListExtension** - Handles `<List>` nodes
   - Processes list items (variable references)

### Code Generation Extensions (Graph → Python)

1. **WorkerCodeGen** - Generates Worker declarations
   ```python
   worker = Worker(core_fn=func, fn_args=[...], placement=Tile(x, y))
   ```

2. **ExternalFunctionCodeGen** - Generates ExternalFunction declarations
   ```python
   func = ExternalFunction(
       name="...",
       source_file="...",
       arg_types=[...],
       include_dirs=[...]
   )
   ```

3. **CoreFunctionCodeGen** - Generates function definitions
   ```python
   def func_name(param1, param2):
       element = obj.acquire(1)
       func(element)
       obj.release(1)
   ```

4. **ListCodeGen** - Generates list declarations
   ```python
   workers = [worker1, worker2, worker3]
   ```

## Adding New Extensions

### Step 1: Create Graph Extension

Add to `GraphExtender.py`:

```python
class MyNodeExtension(GraphExtension):
    tag = "mynode"  # XML tag name (lowercase)
    
    def process(self, elem, parent_nid):
        # 1. Extract attributes
        name = elem.get("name")
        
        # 2. Create graph node
        node_id = self._add_node(name, "MyNode")
        
        # 3. Link to parent
        self._link(parent_nid, node_id, "contains")
        
        # 4. Register in symbol table
        self._declare_symbol(name, node_id)
        
        # 5. Process children/attributes
        # ... your logic here ...
        
        return node_id

# Register it
register_extension(MyNodeExtension)
```

### Step 2: Create Code Generation Extension

Add to `CodeGeneratorExtender.py`:

```python
class MyNodeCodeGen(CodeGenExtension):
    kind = "MyNode"  # Must match graph node kind
    
    def generate(self, node_id):
        # 1. Extract node attributes
        name = self._get_node_attr(node_id, 'label')
        
        # 2. Get related nodes
        children = self._get_children(node_id, 'contains')
        
        # 3. Build code string
        code = f"{name} = MyNode(...)"
        
        return code

# Register it
register_codegen_extension(MyNodeCodeGen)
```

## Add-Activate Example

The add-activate example demonstrates the extension system with element-wise addition and ReLU activation:

### New XML Node Types
- `<Worker>` - Compute workers with core functions
- `<ExternalFunction>` - External C++ kernel functions
- `<CoreFunction>` - Python wrapper functions
- `<List>` - Collections of workers

### New Graph Patterns
- Split operations: `of.cons().split(obj_types=[...], offsets=[...], names=[...])`
- Join operations: `of.prod().join(obj_types=[...], names=[...], offsets=[...])`
- Worker placement: `Worker(core_fn=..., fn_args=[...], placement=Tile(x,y))`
- TensorAccessPattern: Complex tensor slicing patterns

### Generated Code Structure
```python
# Type definitions
chunk_a = np.ndarray[(size,), np.dtype[bfloat16]]

# ObjectFifos with splits/joins
MEM_L2_L1 = SHIM_L3_L2.cons().split(
    obj_types=[chunk_worker, chunk_worker],
    offsets=[0, size//2],
    names=["fifo1", "fifo2"],
    placement=Tile(0, 1)
)

# External functions
element_wise_add = ExternalFunction(
    name="eltwise_add_bf16_scalar",
    source_file="../../../aie_kernels/aie2/add.cc",
    arg_types=[chunk_a, chunk_b, chunk_d],
    include_dirs=["/path/to/kernels/"]
)

# Core functions
def eltwise_add(func, inputA, inputB, outputC):
    elementA = inputA.acquire(1)
    elementB = inputB.acquire(1)
    elementC = outputC.acquire(1)
    func(elementA, elementB, elementC)
    inputA.release(1)
    inputB.release(1)
    outputC.release(1)

# Workers
worker = Worker(
    core_fn=eltwise_add,
    fn_args=[element_wise_add, fifo1.cons(), fifo2.cons(), output.prod()],
    placement=Tile(0, 5)
)

Workers = [worker1, worker2, worker3]
```

## Integration with Main Pipeline

The extensions are automatically registered when:

1. **GraphDriver** calls `register_extensions(self)` in `__init__`
2. **CodeGenerator** calls `register_codegen_extensions(self)` in `__init__`

No changes to `main.py` are required - the extension system is transparent to the main compilation pipeline.

## Testing

To test new extensions:

```bash
# Generate code from XML
python main.py examples/applications/add_activate/add_activate.xml

# Compare with original
diff examples/applications/add_activate/add_activatejit.py \
     examples/applications/add_activate/generated_add_activate.py
```

## Future Extensions

Potential additions:
- `<TensorAccessPattern>` - Complex tensor slicing
- `<MemoryTile>` - Memory tile placement
- `<StreamSwitch>` - Stream switching logic
- `<DMA>` - DMA configuration
- `<Lock>` - Synchronization primitives

Each can be added without modifying core compiler code!