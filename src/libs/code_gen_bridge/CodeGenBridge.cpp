#include "CodeGenBridge.hpp"
#include <Python.h>
#include <fstream>
#include <sstream>
#include <array>
#include <cstdio>
#include <memory>

namespace codegen {

CodeGenBridge::CodeGenBridge()
    : m_pythonInitialized(false)
{
    // Initialize Python if not already initialized
    if (!Py_IsInitialized()) {
        Py_Initialize();
        m_pythonInitialized = true;
    }
}

CodeGenBridge::~CodeGenBridge() {
    // Only finalize if we initialized
    if (m_pythonInitialized && Py_IsInitialized()) {
        Py_Finalize();
    }
}

CodeGenResult<CodeGenOutput> CodeGenBridge::runCodeGen(
    const std::filesystem::path& xmlFilePath,
    const CodeGenOptions& options)
{
    // Validate XML file exists
    auto validation = validateXmlFile(xmlFilePath);
    if (!validation) {
        return std::unexpected(validation.error());
    }

    // Build arguments for main.py
    std::vector<std::string> args;

    // Add XML file path
    args.push_back(xmlFilePath.string());

    // Add output directory
    args.push_back("--output");
    args.push_back(options.outputDir);

    // Add backend
    if (!options.backend.empty() && options.backend != "default") {
        args.push_back("--backend");
        args.push_back(options.backend);
    }

    // Add verbose flag
    if (options.verbose) {
        args.push_back("--verbose");
    }

    // Add clean flag
    if (options.cleanOutput) {
        args.push_back("--clean");
    }

    // Add additional arguments
    for (const auto& [key, value] : options.additionalArgs) {
        args.push_back("--" + key);
        if (!value.empty()) {
            args.push_back(value);
        }
    }

    // Find main.py in the project
    std::filesystem::path mainPyPath = "src/aiecad_compiler/codegen/main.py";

    if (!std::filesystem::exists(mainPyPath)) {
        return std::unexpected(std::vector<CodeGenDiagnostic>{
            CodeGenDiagnostic{CodeGenError::FILE_NOT_FOUND,
                "Code generator main.py not found",
                "Expected at: " + mainPyPath.string()}
        });
    }

    // Run the code generator
    return runPythonScript(mainPyPath.string(), args);
}

CodeGenResult<CodeGenOutput> CodeGenBridge::runCustomScript(
    const std::filesystem::path& scriptPath,
    const std::vector<std::string>& args,
    const std::optional<std::filesystem::path>& workingDir)
{
    if (!std::filesystem::exists(scriptPath)) {
        return std::unexpected(std::vector<CodeGenDiagnostic>{
            CodeGenDiagnostic{CodeGenError::FILE_NOT_FOUND,
                "Python script not found: " + scriptPath.string()}
        });
    }

    return runPythonScript(scriptPath.string(), args, workingDir);
}

bool CodeGenBridge::isAvailable() const {
    // Check if Python is initialized
    if (!Py_IsInitialized()) {
        return false;
    }

    // Check if main.py exists
    std::filesystem::path mainPyPath = "src/aiecad_compiler/codegen/main.py";
    return std::filesystem::exists(mainPyPath);
}

CodeGenResult<std::string> CodeGenBridge::getVersion() {
    // Run Python to get version
    PyRun_SimpleString("import sys");

    PyObject* sysModule = PyImport_ImportModule("sys");
    if (!sysModule) {
        return std::unexpected(std::vector<CodeGenDiagnostic>{
            CodeGenDiagnostic{CodeGenError::PYTHON_ERROR, "Failed to import sys module"}
        });
    }

    PyObject* version = PyObject_GetAttrString(sysModule, "version");
    Py_DECREF(sysModule);

    if (!version) {
        return std::unexpected(std::vector<CodeGenDiagnostic>{
            CodeGenDiagnostic{CodeGenError::PYTHON_ERROR, "Failed to get Python version"}
        });
    }

    const char* versionStr = PyUnicode_AsUTF8(version);
    std::string result = versionStr ? versionStr : "unknown";
    Py_DECREF(version);

    return result;
}

CodeGenResult<CodeGenOutput> CodeGenBridge::runPythonScript(
    const std::string& script,
    const std::vector<std::string>& args,
    const std::optional<std::filesystem::path>& workingDir)
{
    // Build command line
    std::string command = "python \"" + script + "\"";
    for (const auto& arg : args) {
        // Quote arguments that contain spaces
        if (arg.find(' ') != std::string::npos) {
            command += " \"" + arg + "\"";
        } else {
            command += " " + arg;
        }
    }

    // Change to working directory if specified
    std::filesystem::path originalDir;
    if (workingDir) {
        originalDir = std::filesystem::current_path();
        std::filesystem::current_path(*workingDir);
    }

    // Execute command and capture output
    std::array<char, 128> buffer;
    std::string result;
    int exitCode = 0;

#ifdef _WIN32
    std::unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(command.c_str(), "r"), _pclose);
#else
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);
#endif

    if (!pipe) {
        // Restore original directory
        if (workingDir) {
            std::filesystem::current_path(originalDir);
        }

        return std::unexpected(std::vector<CodeGenDiagnostic>{
            CodeGenDiagnostic{CodeGenError::PYTHON_ERROR,
                "Failed to execute Python script",
                "Command: " + command}
        });
    }

    // Read output
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }

    // Get exit code
#ifdef _WIN32
    exitCode = _pclose(pipe.release());
#else
    exitCode = pclose(pipe.release());
#endif

    // Restore original directory
    if (workingDir) {
        std::filesystem::current_path(originalDir);
    }

    // Check for errors
    if (exitCode != 0) {
        return std::unexpected(std::vector<CodeGenDiagnostic>{
            CodeGenDiagnostic{CodeGenError::PYTHON_ERROR,
                "Code generation failed with exit code " + std::to_string(exitCode),
                result}
        });
    }

    // Parse output directory from args or use default
    std::filesystem::path outputDir = "generated";
    for (size_t i = 0; i < args.size(); ++i) {
        if (args[i] == "--output" && i + 1 < args.size()) {
            outputDir = args[i + 1];
            break;
        }
    }

    // Collect generated files
    auto generatedFiles = collectGeneratedFiles(outputDir);

    CodeGenOutput output;
    output.outputDirectory = outputDir;
    output.generatedFiles = std::move(generatedFiles);
    output.pythonOutput = result;
    output.exitCode = exitCode;

    return output;
}

CodeGenResult<void> CodeGenBridge::validateXmlFile(const std::filesystem::path& xmlPath) {
    if (!std::filesystem::exists(xmlPath)) {
        return std::unexpected(std::vector<CodeGenDiagnostic>{
            CodeGenDiagnostic{CodeGenError::FILE_NOT_FOUND,
                "XML file not found: " + xmlPath.string()}
        });
    }

    if (xmlPath.extension() != ".xml") {
        return std::unexpected(std::vector<CodeGenDiagnostic>{
            CodeGenDiagnostic{CodeGenError::INVALID_XML,
                "File is not an XML file: " + xmlPath.string()}
        });
    }

    // Try to open the file to check if it's readable
    std::ifstream file(xmlPath);
    if (!file.is_open()) {
        return std::unexpected(std::vector<CodeGenDiagnostic>{
            CodeGenDiagnostic{CodeGenError::FILE_NOT_FOUND,
                "Cannot read XML file: " + xmlPath.string()}
        });
    }

    return {};
}

std::vector<std::filesystem::path> CodeGenBridge::collectGeneratedFiles(
    const std::filesystem::path& outputDir)
{
    std::vector<std::filesystem::path> files;

    if (!std::filesystem::exists(outputDir)) {
        return files;
    }

    try {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(outputDir)) {
            if (entry.is_regular_file()) {
                files.push_back(entry.path());
            }
        }
    } catch (const std::filesystem::filesystem_error&) {
        // If we can't iterate, just return empty list
        return files;
    }

    return files;
}

} // namespace codegen
