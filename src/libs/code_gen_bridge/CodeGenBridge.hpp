#pragma once

#include <expected>
#include <string>
#include <vector>
#include <map>
#include <optional>
#include <filesystem>

namespace codegen {

/// Error codes for code generation operations
enum class CodeGenError {
    SUCCESS,
    FILE_NOT_FOUND,
    INVALID_XML,
    PYTHON_ERROR,
    OUTPUT_ERROR,
    UNKNOWN_ERROR
};

/// Convert error code to string
inline const char* errorToString(CodeGenError error) {
    switch (error) {
        case CodeGenError::SUCCESS: return "SUCCESS";
        case CodeGenError::FILE_NOT_FOUND: return "FILE_NOT_FOUND";
        case CodeGenError::INVALID_XML: return "INVALID_XML";
        case CodeGenError::PYTHON_ERROR: return "PYTHON_ERROR";
        case CodeGenError::OUTPUT_ERROR: return "OUTPUT_ERROR";
        case CodeGenError::UNKNOWN_ERROR: return "UNKNOWN_ERROR";
    }
    return "UNKNOWN_ERROR";
}

/// Diagnostic information for code generation errors
struct CodeGenDiagnostic {
    CodeGenError error;
    std::string message;
    std::string details;

    CodeGenDiagnostic(CodeGenError err, std::string msg, std::string det = "")
        : error(err), message(std::move(msg)), details(std::move(det)) {}
};

/// Result type for code generation operations
template<typename T>
using CodeGenResult = std::expected<T, std::vector<CodeGenDiagnostic>>;

/// Code generation output information
struct CodeGenOutput {
    std::filesystem::path outputDirectory;
    std::vector<std::filesystem::path> generatedFiles;
    std::string pythonOutput;  // stdout from Python
    int exitCode;
};

/// Code generation options
struct CodeGenOptions {
    std::string backend = "default";  // Backend to use (e.g., "aie", "cpu", "gpu")
    std::string outputDir = "generated";  // Output directory
    bool verbose = false;  // Enable verbose output
    bool cleanOutput = true;  // Clean output directory before generation
    std::map<std::string, std::string> additionalArgs;  // Additional arguments
};

/// Bridge for running Python code generation from C++
class CodeGenBridge {
public:
    /// Constructor - initializes Python environment
    CodeGenBridge();

    /// Destructor - cleans up Python resources
    ~CodeGenBridge();

    // Prevent copying
    CodeGenBridge(const CodeGenBridge&) = delete;
    CodeGenBridge& operator=(const CodeGenBridge&) = delete;

    /// Run code generation from GUI XML file
    /// @param xmlFilePath Path to the GUI XML file from HLIR builder
    /// @param options Code generation options
    /// @return Output information on success, diagnostics on failure
    CodeGenResult<CodeGenOutput> runCodeGen(
        const std::filesystem::path& xmlFilePath,
        const CodeGenOptions& options = CodeGenOptions{});

    /// Run code generation with custom Python script
    /// @param scriptPath Path to Python script to run
    /// @param args Arguments to pass to the script
    /// @param workingDir Working directory for execution
    /// @return Output information on success, diagnostics on failure
    CodeGenResult<CodeGenOutput> runCustomScript(
        const std::filesystem::path& scriptPath,
        const std::vector<std::string>& args,
        const std::optional<std::filesystem::path>& workingDir = std::nullopt);

    /// Check if code generator is available
    /// @return true if Python and required modules are available
    bool isAvailable() const;

    /// Get version information from code generator
    /// @return Version string
    CodeGenResult<std::string> getVersion();

private:
    bool m_pythonInitialized;

    /// Run Python script and capture output
    CodeGenResult<CodeGenOutput> runPythonScript(
        const std::string& script,
        const std::vector<std::string>& args,
        const std::optional<std::filesystem::path>& workingDir = std::nullopt);

    /// Validate XML file exists and is readable
    CodeGenResult<void> validateXmlFile(const std::filesystem::path& xmlPath);

    /// Collect generated files from output directory
    std::vector<std::filesystem::path> collectGeneratedFiles(
        const std::filesystem::path& outputDir);
};

} // namespace codegen
