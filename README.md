# AIECAD — AIE Computer Aided Design

**AIECAD (AIE Computer Aided Design)** is an interactive **GUI-based design platform** that allows developers and performance engineers to visually construct and simulate **AMD AI Engine (AIE) / Ryzen AI NPU** programs using the [IRON](https://github.com/Xilinx/mlir-aie) framework.

Instead of hand-writing AIE Python or C++ code, users can **draw their designs on a grid**, connecting **tiles, memories, and shims** through directed edges that represent the **ObjectFifo** abstraction and then automatically generate executable IRON Python code.

---

## Setup & Install
AIECAD uses Conan to manage its C++ third party libraries. The following `Prerequisites` section walks you through the installation of Conan and any other tools required with setting up the AIECAD project.

### Prerequisites

This guide explains how to install and set up **Conan** for installing AIECAD's C++ dependencies using CMake.

### 1. Install Python

Conan requires Python 3.6 or newer.

Check if Python is installed:

```bash
python3 --version
```

If not, install it from [https://www.python.org/downloads/](https://www.python.org/downloads/).
On Windows, ensure you check “Add Python to PATH” during installation.

---

### 2. Install Conan

Install Conan using `pip`:

```bash
pip install conan
```

Verify the installation:

```bash
conan --version
```

---

### 3. Configure Conan

Detect your system configuration:

```bash
conan profile detect
```

This creates a default profile at:

```
~/.conan2/profiles/default
```

You can view or edit the profile:

```bash
conan profile show default
```

---

### 4. Install dependencies from the Conan configuration file

In the project root, where a file named `conanfile.py` exists, run the following command:

```bash
conan install . --build=missing
```

This will:

* Download the required dependencies.
* Generate CMake configuration files under the `build/` directory.

---

### 5. Build the project

After installing the required dependencies through Conan, run the following commands to build and run the project:

```bash
cmake --preset=dev
cmake --build --preset=dev
```
then to run the project:

```bash
./build/aiecad_app
```

---
