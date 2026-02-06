# Code Generation Bridge

A C++ to Python bridge for running the IRONSmith code generation system. This library provides a simple interface to execute Python code generation scripts from C++ applications, particularly for processing XML files created by the HLIR builder.

## Features

- **Execute Python Code Generation**: Run main.py from C++ with XML input files
- **Flexible Options**: Configure backend, output directory, verbosity, and custom arguments
- **Output Capture**: Capture stdout from Python scripts and collect generated files
- **Error Handling**: Comprehensive error codes and diagnostics
- **Custom Scripts**: Support for running custom Python code generation scripts
- **Python Integration**: Automatic Python environment initialization

## Architecture

```
C++ Application
      |
      v
CodeGenBridge (C++)
      |
      v
Python main.py
      |
      v
Code Generation Output
```

## Typical Workflow

1. **Build HLIR Design** - Use HlirBridge to construct hardware design
2. **Export to XML** - Call `HlirBridge::exportToGuiXml()` to create XML file
3. **Run Code Generation** - Use CodeGenBridge to process XML and generate code
4. **Collect Output** - Get generated files and Python output

### Complete Example

```cpp
#include "HlirBridge.hpp"
#include "CodeGenBridge.hpp"

// 1. Build HLIR design
hlir::HlirBridge hlirBridge("my_design");
// ... add tiles, FIFOs, kernels, etc ...
hlirBridge.build();

// 2. Export to XML
auto xmlPath = "design.xml";
hlirBridge.exportToGuiXml(xmlPath);

// 3. Run code generation
codegen::CodeGenBridge codegenBridge;
codegen::CodeGenOptions options;
options.backend = "aie";
options.outputDir = "generated";
options.verbose = true;

auto result = codegenBridge.runCodeGen(xmlPath, options);

// 4. Process results
if (result) {
    std::cout << "Generated " << result->generatedFiles.size() << " files\n";
    for (const auto& file : result->generatedFiles) {
        std::cout << "  - " << file << "\n";
    }
}
```

## API Overview

### Main Methods

#### `runCodeGen(xmlFilePath, options)`

Run code generation from a GUI XML file.

```cpp
CodeGenResult<CodeGenOutput> runCodeGen(
    const std::filesystem::path& xmlFilePath,
    const CodeGenOptions& options = CodeGenOptions{});
```

**Parameters:**
- `xmlFilePath` - Path to XML file from HLIR builder
- `options` - Code generation options (backend, output dir, flags)

**Returns:** `CodeGenOutput` on success, diagnostics on failure

#### `runCustomScript(scriptPath, args, workingDir)`

Run a custom Python code generation script.

```cpp
CodeGenResult<CodeGenOutput> runCustomScript(
    const std::filesystem::path& scriptPath,
    const std::vector<std::string>& args,
    const std::optional<std::filesystem::path>& workingDir = std::nullopt);
```

**Parameters:**
- `scriptPath` - Path to Python script
- `args` - Command line arguments for the script
- `workingDir` - Optional working directory for execution

**Returns:** `CodeGenOutput` on success, diagnostics on failure

#### `isAvailable()`

Check if code generator is available.

```cpp
bool isAvailable() const;
```

**Returns:** `true` if Python is initialized and main.py exists

#### `getVersion()`

Get Python version information.

```cpp
CodeGenResult<std::string> getVersion();
```

**Returns:** Python version string

## Data Structures

### CodeGenOptions

Configuration options for code generation.

```cpp
struct CodeGenOptions {
    std::string backend = "default";           // Backend: "aie", "cpu", "gpu"
    std::string outputDir = "generated";       // Output directory
    bool verbose = false;                      // Enable verbose output
    bool cleanOutput = true;                   // Clean output before generation
    std::map<std::string, std::string> additionalArgs;  // Custom arguments
};
```

### CodeGenOutput

Results from code generation.

```cpp
struct CodeGenOutput {
    std::filesystem::path outputDirectory;              // Where files were generated
    std::vector<std::filesystem::path> generatedFiles;  // List of generated files
    std::string pythonOutput;                           // stdout from Python
    int exitCode;                                       // Process exit code
};
```

### CodeGenDiagnostic

Error information.

```cpp
struct CodeGenDiagnostic {
    CodeGenError error;    // Error code
    std::string message;   // Error message
    std::string details;   // Additional details
};
```

## Error Codes

```cpp
enum class CodeGenError {
    SUCCESS,          // Operation succeeded
    FILE_NOT_FOUND,   // XML or script file not found
    INVALID_XML,      // Invalid XML file format
    PYTHON_ERROR,     // Python execution error
    OUTPUT_ERROR,     // Error processing output
    UNKNOWN_ERROR     // Unknown error occurred
};
```

## Error Handling

All operations return `CodeGenResult<T>` which is `std::expected<T, std::vector<CodeGenDiagnostic>>`.

### Example

```cpp
auto result = bridge.runCodeGen("design.xml");
if (!result) {
    // Handle error
    for (const auto& diag : result.error()) {
        std::cerr << errorToString(diag.error) << ": "
                  << diag.message << "\n";
        if (!diag.details.empty()) {
            std::cerr << "Details: " << diag.details << "\n";
        }
    }
    return;
}

// Use the output
const auto& output = result.value();
std::cout << "Generated " << output.generatedFiles.size() << " files\n";
```

## Command Line Mapping

The bridge maps C++ options to Python command line arguments:

```cpp
CodeGenOptions options;
options.backend = "aie";
options.outputDir = "generated";
options.verbose = true;
options.cleanOutput = true;
options.additionalArgs["target"] = "versal";
```

Becomes:

```bash
python main.py design.xml --output generated --backend aie --verbose --clean --target versal
```

## Dependencies

- **C++23** - Required for `std::expected` and `std::filesystem`
- **Python 3.x** - Python interpreter for code generation
- **main.py** - Expected at `src/aiecad_compiler/codegen/main.py`

## Building

The library is integrated with the IRONSmith CMake build system:

```cmake
# Automatically included via src/libs/CMakeLists.txt
add_subdirectory(code_gen_bridge)
```

Link against the library:

```cmake
target_link_libraries(your_target PRIVATE CodeGenBridge)
```

## File Structure

```
src/libs/code_gen_bridge/
├── CodeGenBridge.hpp       - Main interface
├── CodeGenBridge.cpp       - Implementation
├── example_usage.cpp       - Usage example
├── CMakeLists.txt          - Build configuration
└── README.md               - This file
```

## Integration with HLIR Bridge

The code generation bridge is designed to work seamlessly with the HLIR C++ bridge:

```cpp
// Step 1: Build HLIR design
hlir::HlirBridge hlir("design");
auto tileId = hlir.addTile("compute_0", hlir::TileKind::COMPUTE, 0, 5);
// ... build complete design ...
hlir.build();

// Step 2: Export to XML
std::string xmlPath = "output.xml";
hlir.exportToGuiXml(xmlPath);

// Step 3: Generate code
codegen::CodeGenBridge codegen;
codegen::CodeGenOptions opts;
opts.backend = "aie";
auto result = codegen.runCodeGen(xmlPath, opts);
```

## Python Script Requirements

For custom scripts used with `runCustomScript()`:

- Must be executable Python scripts
- Should accept command line arguments
- Return exit code 0 on success
- Output files to a known directory (passed via arguments)

## Platform Support

- **Windows**: Uses `_popen()` and `_pclose()`
- **Linux/macOS**: Uses `popen()` and `pclose()`

## Version

Version 1.0.0 - Initial release
