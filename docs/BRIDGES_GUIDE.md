# IRONSmith C++ Bridges Guide

This document explains the two C++ bridge libraries that connect IRONSmith's C++ application to the Python-based HLIR and code generation systems.

## Overview

```
                        IRONSmith C++ Application
                                  |
              +-------------------+-------------------+
              |                                       |
              v                                       v
       HLIR C++ Bridge                      CodeGen C++ Bridge
              |                                       |
              v                                       v
    Python HLIR Builder                    Python Code Generator
              |                                       |
              v                                       v
         GUI XML File  ---------------------->  Generated Python Code
```

---

## 1. HLIR C++ Bridge

**Purpose**: Build AIE designs programmatically from C++

**Location**: `src/libs/hlir_cpp_bridge/`

### What It Does

The HLIR Bridge lets you create AIE hardware designs using C++ code. It provides a type-safe interface to Python's HLIR (High-Level Intermediate Representation) system.

### Key Concepts

| Concept | Description |
|---------|-------------|
| **ComponentId** | Unique identifier for every component (tiles, FIFOs, workers, etc.) |
| **HlirResult<T>** | Return type that holds either a success value or error diagnostics |
| **HlirBridge** | Main class that manages the design and communicates with Python |

### Basic Workflow

```cpp
#include "hlir_cpp_bridge/HlirBridge.hpp"

// 1. Create a bridge with your program name
hlir::HlirBridge bridge("my_design");

// 2. Add components (each returns a ComponentId)
auto constId = bridge.addConstant("N", "1024", "int");
auto tensorType = bridge.addTensorType("data_ty", {"N"}, "bfloat16");
auto tile = bridge.addTile("compute_0", hlir::TileKind::COMPUTE, 0, 5);
auto fifo = bridge.addFifo("my_fifo", *tensorType, 2);

// 3. Build runtime sequence
auto runtime = bridge.createRuntime("runtime");
bridge.runtimeAddInputType(*tensorType);
bridge.runtimeAddParam("input_data");
bridge.runtimeAddFill("fill_0", *fifo, "input_data", *tile);
bridge.runtimeBuild();

// 4. Validate and export
bridge.build();  // Validates the design
bridge.exportToGuiXml("output.xml");  // Creates GUI XML file
```

### Component Types You Can Create

| Method | Creates |
|--------|---------|
| `addConstant()` | Named constants (e.g., buffer sizes) |
| `addTensorType()` | Tensor/array type definitions |
| `addTile()` | Hardware tiles (SHIM, MEM, COMPUTE) |
| `addFifo()` | Data movement FIFOs |
| `addFifoSplit()` | Split one FIFO into multiple |
| `addFifoJoin()` | Join multiple FIFOs into one |
| `addExternalKernel()` | External C/C++ kernel functions |
| `addCoreFunction()` | Kernel wrappers with acquire/release |
| `addWorker()` | Binds functions to tiles |

### Error Handling

```cpp
auto result = bridge.addTile("tile0", hlir::TileKind::COMPUTE, 0, 5);

if (!result) {
    // Handle error
    for (const auto& diag : result.error()) {
        std::cerr << "Error: " << diag.message << "\n";
    }
} else {
    // Use the component ID
    hlir::ComponentId tileId = result.value();
}
```

---

## 2. CodeGen C++ Bridge

**Purpose**: Run Python code generation from C++

**Location**: `src/libs/code_gen_bridge/`

### What It Does

The CodeGen Bridge takes the GUI XML file (created by HLIR Bridge) and runs the Python code generator to produce final Python code for the AIE hardware.

### Key Concepts

| Concept | Description |
|---------|-------------|
| **CodeGenBridge** | Main class that runs Python code generation |
| **CodeGenOptions** | Configuration for code generation (backend, output dir, etc.) |
| **CodeGenOutput** | Result containing generated file paths and status |

### Basic Workflow

```cpp
#include "code_gen_bridge/CodeGenBridge.hpp"

// 1. Create the bridge
codegen::CodeGenBridge codeGen;

// 2. Check if Python code generator is available
if (!codeGen.isAvailable()) {
    std::cerr << "Code generator not found!\n";
    return;
}

// 3. Configure options (optional)
codegen::CodeGenOptions options;
options.outputDir = "generated";
options.verbose = true;

// 4. Run code generation on GUI XML file
auto result = codeGen.runCodeGen("my_design_gui.xml", options);

if (result) {
    // Success - list generated files
    for (const auto& file : result.value().generatedFiles) {
        std::cout << "Generated: " << file << "\n";
    }
} else {
    // Error
    for (const auto& diag : result.error()) {
        std::cerr << "Error: " << diag.message << "\n";
    }
}
```

### Code Generation Pipeline

```
GUI XML File (from HLIR Bridge)
        |
        v
   XMLGenerator (expands to Complete XML)
        |
        v
   GraphDriver (builds semantic graph)
        |
        v
   CodeGenerator (produces Python code)
        |
        v
Generated Python Files (.py, .graphml)
```

### Options

| Option | Description | Default |
|--------|-------------|---------|
| `backend` | Target backend | `"default"` |
| `outputDir` | Where to put generated files | `"generated"` |
| `verbose` | Print detailed output | `false` |
| `cleanOutput` | Clear output dir first | `true` |

---

## Complete Example: End-to-End Pipeline

```cpp
#include "hlir_cpp_bridge/HlirBridge.hpp"
#include "code_gen_bridge/CodeGenBridge.hpp"

int main() {
    // === Step 1: Build the design with HLIR Bridge ===
    hlir::HlirBridge hlir("passthrough");

    // Add components
    auto N = hlir.addConstant("N", "4096", "int");
    auto vectorTy = hlir.addTensorType("vector_ty", {"N"}, "int32");
    auto shim = hlir.addTile("shim0", hlir::TileKind::SHIM, 0, 0);
    auto fifoIn = hlir.addFifo("of_in", *vectorTy, 2);
    auto fifoOut = hlir.addFifoForward("of_out", *fifoIn);

    // Build runtime
    auto rt = hlir.createRuntime("runtime");
    hlir.runtimeAddInputType(*vectorTy);
    hlir.runtimeAddOutputType(*vectorTy);
    hlir.runtimeAddParam("inputA");
    hlir.runtimeAddParam("outputC");
    hlir.runtimeAddFill("fill_0", *fifoIn, "inputA", *shim);
    hlir.runtimeAddDrain("drain_0", *fifoOut, "outputC", *shim);
    hlir.runtimeBuild();

    // Validate and export
    hlir.build();
    hlir.exportToGuiXml("passthrough_gui.xml");

    // === Step 2: Generate code with CodeGen Bridge ===
    codegen::CodeGenBridge codeGen;

    auto result = codeGen.runCodeGen("passthrough_gui.xml");

    if (result) {
        std::cout << "Success! Generated files:\n";
        for (const auto& file : result.value().generatedFiles) {
            std::cout << "  - " << file.filename() << "\n";
        }
    }

    return 0;
}
```

---

## Summary

| Bridge | Input | Output | Purpose |
|--------|-------|--------|---------|
| **HLIR Bridge** | C++ code | GUI XML file | Design AIE hardware programmatically |
| **CodeGen Bridge** | GUI XML file | Python code | Generate executable AIE code |

Both bridges use:
- Type-safe C++ interfaces
- `std::expected` for error handling
- Python interop via embedded interpreter
- Comprehensive error diagnostics
