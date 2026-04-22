## Installation Process

Installing IRONSmith is straightforward once the required tools are available. The provided CMake preset configuration uses **Ninja** as the generator, requires **CMake 3.27.0 or newer**, writes build files to `out/build/<preset>`, writes install output to `out/install/<preset>`, and already defines the main development presets `dev-debug`, `dev-relWithDeb`, `dev-release`, and `dev-asan`.

1. **Install the required development tools.**  
   IRONSmith should be built with a Unix-like toolchain such as **GCC**, **Clang**, **Apple Clang**, or **WSL**. If you intend to use the default development presets, install **CMake**, **Ninja**, **Python 3**, **Qt 6**, **QScintilla**, and a **GoogleTest/GMock** package as well, since the preset configuration enables tests in the common development flows.

   **macOS (Homebrew example):**

   ```bash
   brew install cmake ninja python qt qscintilla2 googletest
   ```

   **Ubuntu / WSL example:**

   ```bash
   sudo apt update
   sudo apt install build-essential clang cmake ninja-build python3 python3-venv qt6-base-dev libqscintilla2-qt6-dev libgtest-dev libgmock-dev
   ```

2. **Create and activate a Python virtual environment.**  
   A local preset is already provided that points `Python3_EXECUTABLE` to a virtual-environment interpreter, so creating a venv in the project root is the cleanest approach.

   ```bash
   python3 -m venv venv
   source venv/bin/activate
   ```

3. **Set `Python3_EXECUTABLE` and `Qt6_DIR` before configuring.**  
   The supplied `CMakeUserPresets.json` already shows the intended pattern: it defines a local preset named `dev-debug-local` that inherits from `dev-debug` and sets `Python3_EXECUTABLE` to the virtual-environment Python path. To avoid configuration failures, extend that same local preset so it also sets `Qt6_DIR`:

   ```json
   {
     "version": 6,
     "configurePresets": [
       {
         "name": "dev-debug-local",
         "inherits": "dev-debug",
         "cacheVariables": {
           "Python3_EXECUTABLE": "/absolute/path/to/venv/bin/python",
           "Qt6_DIR": "/absolute/path/to/Qt6/lib/cmake/Qt6"
         }
       }
     ]
   }
   ```

   On many systems, `Qt6_DIR` will point to the directory that contains `Qt6Config.cmake`. For example, this is often one of:

   - `/opt/homebrew/opt/qt/lib/cmake/Qt6`
   - `/usr/local/opt/qt/lib/cmake/Qt6`
   - `/usr/lib/x86_64-linux-gnu/cmake/Qt6`

4. **Install QScintilla and make sure Qt can see it.**  
   QScintilla is a required dependency for the project, so it should be installed before configuration. On Linux/WSL, the package-manager install is usually sufficient. On macOS, installing it through Homebrew alongside Qt is the simplest route. If CMake still fails to locate it after installation, verify that Qt 6 is being found first by setting `Qt6_DIR` correctly, then re-run configuration.

5. **Configure the project using one of the provided presets.**  
   The main configure presets are `dev-debug`, `dev-relWithDeb`, `dev-release`, and `dev-asan`, and the local file already demonstrates how to define a machine-specific preset layered on top of them.

   ```bash
   cmake --preset dev-debug-local
   ```

   If you do not want a local preset, you can also pass the variables directly:

   ```bash
   cmake --preset dev-debug \
     -DPython3_EXECUTABLE=/absolute/path/to/venv/bin/python \
     -DQt6_DIR=/absolute/path/to/Qt6/lib/cmake/Qt6
   ```

6. **Build the project.**  
   The preset file defines matching build presets, including `build-dev-debug`, `build-dev-relWithDeb`, `build-dev-release`, and `build-dev-asan`. The user-local preset file also defines `build-dev-debug-local`.

   ```bash
   cmake --build --preset build-dev-debug-local
   ```

7. **Run tests if needed.**  
   Test presets are also provided, including `test-dev-debug`, `test-dev-relWithDeb`, `test-dev-asan`, and the local `test-dev-debug-local`, all configured to print output on failure.

   ```bash
   ctest --preset test-dev-debug-local
   ```

In practice, the most common causes of configuration failure are a missing **Qt 6** installation, a missing **QScintilla** package, or not setting `Python3_EXECUTABLE` and `Qt6_DIR` to valid local paths before running CMake.
