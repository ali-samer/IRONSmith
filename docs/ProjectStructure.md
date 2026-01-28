# Project Structure and Setup

The **AIECAD** project is organized as a modern **C++** project, designed for modularity.  
This document provides an overview of the project’s structure and how to get started contributing to it.

---

## Overview

AIECAD follows a standard **CMake-based** build system and uses a **dependency manager** to handle external libraries and tools.  
For details on how dependencies are managed, see the [Dependency Management](./DependencyManagement.md) page.

---

## Contribution

To start contributing, follow these two instructions:
1. **Activate the development environment:**
   ```bash
   source ironenv/bin/activate
    ```
2. **Set up project environment variables and tools:**
   ```bash
   source scripts/env_setup.sh
   ```

---

### Writing Tests

Testing your components is a straightforward process.

To add new tests:

1. Navigate to the CMake directory that contains the **AIECAD_Tests.cmake** file:

```bash
cd cmake/
```

2. Open **AIECAD_Tests.cmake** and scroll to the bottom. You’ll see a call to:

```cmake
aiecad_define_tests(
    DIRECTORY test/
        TEST dummy_calc_test SOURCES CalcTest.cpp
)
```

3. To register your own test, simply append it to this function call. For example:

```cmake
aiecad_define_tests(
    DIRECTORY test/
        TEST dummy_calc_test SOURCES CalcTest.cpp
        TEST my_new_feature_test SOURCES MyNewFeatureTest.cpp
)
```

4. All test source files should live under:

```bash
aiecad/aiecad/test/
```

5. By default, all tests share a common `main()` function defined in:

```aiecad/aiecad/test/common/TestMain.cpp```

This uses:
```cpp
#include <aiecad/Portability.h>
#include <aiecad/portability/GTest.hpp>

AIECAD_ATTR_WEAK int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
```

Because this main is marked **AIECAD_ATTR_WEAK**, you can optionally define your own `main()` with a **strong symbol** if your tests require custom initialization before running GoogleTest.

6. Once you’ve added your test entry and source file, rebuild the project. The new test target will be automatically detected and available through CTest or your IDE’s test runner.

---
## Directory Structure
TODO