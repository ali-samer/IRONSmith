# HLIR C++ Bridge

A C++ to Python bridge for the IRONSmith HLIR (High Level Intermediate Representation) system. This library provides a type-safe, ID-based C++ interface to the Python HLIR ProgramBuilder.

## Features

- **ID-Based Architecture**: All component references use `ComponentId` for type safety and better tracking
- **Complete HLIR API**: All 11 `add_*` methods plus lookup, update, and remove operations
- **Error Handling**: Comprehensive error codes and diagnostics with dependency tracking
- **Runtime Sequence Building**: Full support for creating and configuring runtime sequences
- **Program Validation**: Build and validate complete HLIR programs
- **XML Export**: Export to GUI-compatible XML format

## Architecture

```
C++ Application
      |
      v
HlirBridge (C++)
      |
      v
Python Wrapper (hlir_bridge_wrapper.py)
      |
      v
HLIR ProgramBuilder (Python)
```

## Component ID Workflow

The bridge uses a consistent ID-based workflow:

1. **Create** component with all parameters -> Returns `ComponentId`
2. **Reference** other components using their `ComponentId`
3. **Modify/Lookup/Delete** components using their `ComponentId`

### Example

```cpp
// 1. Create a tile -> Get ID
auto tileId = bridge.addTile("compute_0", TileKind::COMPUTE, 0, 5);

// 2. Create a FIFO referencing the tile by ID
auto fifoId = bridge.addFifo("my_fifo", tensorTypeId, 2,
                              tileId,      // producer ID
                              {tileId2});  // consumer IDs

// 3. Update components using their ID (preserves dependencies!)
auto updatedTileId = bridge.addTile("compute_0", TileKind::COMPUTE, 2, 7, tileId);
// Same ID, new location - FIFO still references this tile correctly!

// 4. Specialized update methods
bridge.updateFifoDepth(fifoId, 4);

// 5. Lookup using ID
auto data = bridge.lookupById(fifoId);

// 6. Remove using ID
bridge.remove(fifoId);
```

## API Overview

### Component Creation (11 methods)

- `addSymbol()` / `addConstant()` - Add symbols and constants
- `addTensorType()` - Add tensor type definitions
- `addTile()` - Add hardware tiles (shim, mem, compute)
- `addFifo()` / `addFifoSimpleType()` - Add data movement FIFOs
- `addFifoSplit()` / `addFifoJoin()` / `addFifoForward()` - Add FIFO operations
- `addExternalKernel()` - Add external C/C++ kernels
- `addCoreFunction()` - Add kernel wrappers with acquire/release semantics
- `addWorker()` - Bind core functions to tiles

### Component Lookup

- `lookupById(ComponentId)` - Get component data as JSON
- `lookupByName(ComponentType, name)` - Find component ID by name
- `getAllIds(ComponentType)` - Get all component IDs of a type

### Component Modification

- **Component Updates** - All `add_*` methods accept an optional `ComponentId` to update existing components
- `updateFifoDepth(ComponentId, newDepth)` - Update FIFO depth (specialized method)
- `remove(ComponentId)` - Remove component (with dependency checking)

#### Updating Components

Any component can be updated by calling its corresponding `add_*` method with the existing component ID:

```cpp
// Create a tile
auto result = bridge.addTile("compute_0", TileKind::COMPUTE, 0, 5);
ComponentId tileId = result.value();

// Later, update the tile's location (same ID, new coordinates)
auto updateResult = bridge.addTile("compute_0", TileKind::COMPUTE, 2, 7, tileId);
// Dependencies remain intact - all workers, FIFOs still reference this tile!
```

**Key Benefits:**
- Component ID stays the same → dependencies are preserved
- All references remain valid → no need to update dependent components
- Enable interactive GUI editing without breaking designs
- Support design iteration and refinement

**All updatable components:**
- Symbols, Constants, Tensor Types
- Tiles (location, kind, metadata)
- FIFOs (depth, producer, consumers, type)
- FIFO operations (split, join, forward)
- External Kernels (source file, arguments)
- Core Functions (parameters, body)
- Workers (placement, arguments)

### Runtime Operations

- `createRuntime(name)` - Create runtime sequence
- `runtimeAddInputType(ComponentId)` - Add input type
- `runtimeAddOutputType(ComponentId)` - Add output type
- `runtimeAddParam(name)` - Add parameter
- `runtimeAddFill(name, fifoId, inputName, tileId)` - Add fill operation
- `runtimeAddDrain(name, fifoId, outputName, tileId)` - Add drain operation
- `runtimeBuild()` - Build runtime sequence

### Program Building

- `build()` - Build and validate program
- `getProgram()` - Get program without validation
- `exportToGuiXml(filePath)` - Export to GUI XML file
- `exportToGuiXmlString()` - Export to XML string
- `getStats()` - Get program statistics

## Error Handling

All operations return `HlirResult<T>` which is `std::expected<T, std::vector<HlirDiagnostic>>`.

### Error Codes

- `SUCCESS` - Operation succeeded
- `DUPLICATE_NAME` - Component with this name already exists
- `NOT_FOUND` - Component not found
- `DEPENDENCY_EXISTS` - Cannot remove due to dependencies
- `INVALID_PARAMETER` - Invalid parameter value
- `PYTHON_EXCEPTION` - Python runtime error
- `JSON_PARSE_ERROR` - JSON parsing error

### Error Handling Example

```cpp
auto result = bridge.addTile("tile0", TileKind::COMPUTE, 0, 5);
if (!result) {
    // Handle error
    for (const auto& diag : result.error()) {
        std::cerr << errorCodeToString(diag.code) << ": "
                  << diag.message << "\n";
        if (!diag.dependencies.empty()) {
            std::cerr << "  Dependencies: ";
            for (const auto& dep : diag.dependencies) {
                std::cerr << dep << " ";
            }
        }
    }
    return;
}

// Use the component ID
ComponentId tileId = result.value();
```

## Dependencies

- **C++23** - Required for `std::expected`
- **Python 3.x** - Python interpreter with HLIR module
- **nlohmann/json** - JSON parsing (auto-fetched by CMake)

## Building

The library is integrated with the IRONSmith CMake build system:

```cmake
# Automatically included via src/libs/CMakeLists.txt
add_subdirectory(hlir_cpp_bridge)
```

Link against the library:

```cmake
target_link_libraries(your_target PRIVATE HlirCppBridge)
```

## Example Usage

See [example_usage.cpp](example_usage.cpp) for a complete example demonstrating:

1. Creating constants and tensor types
2. Defining hardware tiles
3. Creating FIFOs with ID-based producer/consumer references
4. Adding external kernels with typed arguments
5. Creating core functions with acquire/release semantics
6. Binding workers to tiles with ID-based function arguments
7. Building runtime sequences
8. Validating and exporting programs

## File Structure

```
src/libs/hlir_cpp_bridge/
├── HlirTypes.hpp         - Error codes, ComponentId, result types
├── HlirComponents.hpp    - Component data structures
├── HlirBridge.hpp        - Main bridge interface
├── HlirBridge.cpp        - Implementation
├── python/
│   └── hlir_bridge_wrapper.py  - Python wrapper module
├── example_usage.cpp     - Complete usage example
├── CMakeLists.txt        - Build configuration
└── README.md             - This file
```

## Integration with HLIR

The bridge wraps the Python HLIR `ProgramBuilder` class located at:
`src/aiecad_compiler/hlir/builder.py`

All component IDs are UUIDs generated by the Python `ProgramBuilder` and tracked throughout the design lifecycle.

## Type Safety

The ID-based design provides several type safety benefits:

1. **No string typos**: Reference components by ID instead of name
2. **Compile-time checking**: ComponentId type prevents mixing different component types
3. **Automatic dependency tracking**: Python builder tracks all component dependencies
4. **Safe deletion**: Cannot delete components with active dependencies

## Version

Version 1.0.0 - Initial release with complete ID-based API
