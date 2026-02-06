/**
 * Code Generation Bridge - Example Usage
 *
 * Demonstrates how to run Python code generation from C++ using XML files
 * created by the HLIR builder.
 */

#include "CodeGenBridge.hpp"
#include <iostream>
#include <filesystem>

using namespace codegen;

void printError(const std::vector<CodeGenDiagnostic>& errors) {
    for (const auto& err : errors) {
        std::cerr << "ERROR [" << errorToString(err.error) << "]: "
                  << err.message << "\n";
        if (!err.details.empty()) {
            std::cerr << "   Details: " << err.details << "\n";
        }
    }
}

int main() {
    try {
        std::cout << "=== Code Generation Bridge - Example Usage ===\n\n";

        // ====================================================================
        // 1. Create bridge instance
        // ====================================================================
        std::cout << "Step 1: Creating CodeGenBridge...\n";
        CodeGenBridge bridge;

        // Check if code generator is available
        if (!bridge.isAvailable()) {
            std::cerr << "ERROR: Code generator not available!\n";
            std::cerr << "Make sure Python is installed and main.py exists.\n";
            return 1;
        }
        std::cout << "   OK: Code generator is available\n";

        // ====================================================================
        // 2. Get Python version
        // ====================================================================
        std::cout << "\nStep 2: Checking Python version...\n";
        auto version = bridge.getVersion();
        if (version) {
            std::cout << "   OK: Python version: " << *version << "\n";
        } else {
            printError(version.error());
        }

        // ====================================================================
        // 3. Set up code generation options
        // ====================================================================
        std::cout << "\nStep 3: Configuring code generation options...\n";

        CodeGenOptions options;
        options.backend = "aie";                    // AIE backend
        options.outputDir = "generated_output";     // Output directory
        options.verbose = true;                     // Enable verbose output
        options.cleanOutput = true;                 // Clean before generation

        // Add custom arguments
        options.additionalArgs["target"] = "versal";
        options.additionalArgs["opt-level"] = "2";

        std::cout << "   OK: Options configured\n";
        std::cout << "      Backend: " << options.backend << "\n";
        std::cout << "      Output: " << options.outputDir << "\n";
        std::cout << "      Verbose: " << (options.verbose ? "yes" : "no") << "\n";

        // ====================================================================
        // 4. Run code generation from XML file
        // ====================================================================
        std::cout << "\nStep 4: Running code generation...\n";

        // This XML file would typically be created by HlirBridge.exportToGuiXml()
        std::filesystem::path xmlFile = "example_design.xml";

        // Check if XML file exists
        if (!std::filesystem::exists(xmlFile)) {
            std::cout << "   NOTE: XML file 'example_design.xml' not found.\n";
            std::cout << "   In a real scenario, you would:\n";
            std::cout << "   1. Use HlirBridge to build your design\n";
            std::cout << "   2. Export to XML with bridge.exportToGuiXml()\n";
            std::cout << "   3. Pass that XML file to CodeGenBridge\n";
            std::cout << "\n   Skipping actual code generation for this example.\n";
            return 0;
        }

        // Run code generation
        auto result = bridge.runCodeGen(xmlFile, options);

        if (!result) {
            std::cerr << "\nCode generation FAILED:\n";
            printError(result.error());
            return 1;
        }

        // ====================================================================
        // 5. Process results
        // ====================================================================
        std::cout << "\nStep 5: Code generation completed successfully!\n";

        const auto& output = result.value();

        std::cout << "   Output directory: " << output.outputDirectory << "\n";
        std::cout << "   Exit code: " << output.exitCode << "\n";
        std::cout << "   Generated files (" << output.generatedFiles.size() << "):\n";

        for (const auto& file : output.generatedFiles) {
            std::cout << "      - " << file << "\n";
        }

        if (!output.pythonOutput.empty()) {
            std::cout << "\n   Python output:\n";
            std::cout << "   " << std::string(60, '-') << "\n";
            std::cout << output.pythonOutput;
            std::cout << "   " << std::string(60, '-') << "\n";
        }

        // ====================================================================
        // 6. Run custom Python script (optional)
        // ====================================================================
        std::cout << "\nStep 6: Running custom Python script (optional)...\n";

        std::filesystem::path customScript = "custom_codegen.py";
        std::vector<std::string> customArgs = {"--format", "verilog"};

        if (std::filesystem::exists(customScript)) {
            auto customResult = bridge.runCustomScript(
                customScript,
                customArgs,
                std::filesystem::current_path()
            );

            if (customResult) {
                std::cout << "   OK: Custom script completed\n";
                std::cout << "   Generated " << customResult->generatedFiles.size()
                          << " files\n";
            } else {
                std::cout << "   WARN: Custom script failed\n";
                printError(customResult.error());
            }
        } else {
            std::cout << "   NOTE: No custom script found, skipping.\n";
        }

        // ====================================================================
        // Success!
        // ====================================================================
        std::cout << "\n=== SUCCESS ===\n";
        std::cout << "Code generation bridge executed successfully!\n";
        std::cout << "===================================================\n";

        return 0;

    } catch (const std::exception& e) {
        std::cerr << "FATAL ERROR: " << e.what() << "\n";
        return 1;
    }
}
