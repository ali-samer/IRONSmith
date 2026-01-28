# Ryzen NPU GUI

**Product Specification — Capstone Project**

---

## 1. Project Overview

The **Ryzen NPU GUI** is a standalone tool that allows developers to **visually map dataflows onto the Ryzen AI Engine (AIE) array** and **run them directly using IRON’s JIT compiler**.

This GUI is aimed at **external developers** who want to experiment with the Ryzen NPU but **do not want to learn low-level NPU programming**. Users define data movement and execution manually through an intuitive, graphical interface.

The GUI enables users to:

- Create and connect operations (e.g., add, multiply, filter, etc.).
- Assign each operation to specific AIE tiles.
- Generate IRON code automatically.
- Run workloads using IRON’s JIT.
- Compare results against _golden reference outputs_ for correctness.

---

## 2. Goals and Scope

### 2.1. Primary Goals

- Enable users to **manually define and visualize dataflow graphs**.
- Allow mapping of operations to individual NPU tiles.
- Support **automatic IRON JIT compilation and execution**.
- Provide tools to view results and compare them against user-provided golden references.

### 2.2. Out of Scope

- Automatic placement or optimization.
- Multi-device execution.
- Advanced performance profiling or debugging.
- Deep integration with other AMD tools.

---

## 3. Target Users

- Developers and researchers exploring Ryzen NPUs for experimentation or learning.
- Users who understand dataflow or ML concepts but lack NPU-specific knowledge.
- Ideal for educational purposes, demonstrations, or prototype workloads.

---

## 4. Core Functionality

| Feature                 | Description                                                                                      |
| ----------------------- | ------------------------------------------------------------------------------------------------ |
| **Graph Editor**        | Users create and connect operation nodes to define dataflows.                                    |
| **AIE Grid View**       | Interactive grid representing the NPU tile layout. Nodes can be assigned to tiles.               |
| **Parameter Editor**    | UI to set operation and connection parameters (e.g., constants, kernel sizes, data types, etc.). |
| **IRON JIT Execution**  | Automatically compiles and runs the dataflow using IRON’s JIT backend.                           |
| **Output Logging**      | Displays runtime logs and output tensor results.                                                 |
| **Verification Module** | Compares NPU outputs to user-provided golden references and highlights mismatches.               |

---

## 5. User Workflow

1. **Start a New Project**  
   Open the GUI and create a new dataflow project. The user can either start from a blank template, or choose from a few graph templates.

2. **Create and Connect Nodes**  
   Add operation nodes and connect their inputs and outputs. Operations are vectorized kernels that can run on an AIE, and can be defined as a core body with building blocks or kernel templates in the GUI.

3. **Map Operations to AIE Tiles**  
   Assign each operation to a tile using drag-and-drop or manual selection. Connections betweeen AIE tiles will be objectfifo objects from IRON. Data broadcasting is possible from any producer to multiple consumers, and join/distribute data movement can be specified with connections involving the Memory Tile.

4. **Configure Parameters**  
   Set any constants or operational parameters as needed. For connections, the user can choose parameters such as objectfifo depth, data types, array sizes, etc.

5. **Run with IRON JIT**  
   The GUI generates IRON code behind the scenes and executes it on the local Ryzen NPU.

6. **View and Verify Results**
    - Inspect logs and output tensors.
    - Upload a golden reference output file.
    - View a pass/fail summary or numerical error comparison.

---

## 6. System Architecture

### Ryzen NPU Dataflow GUI

1. Frontend (Visualization)
2. Backend (Graph Model)
3. IRON Interface (JIT)
4. Verification Module

#### IRON Runtime (JIT Compiler)

- Converts graph to binary
- Runs code on NPU
- Returns results to GUI

---

## 7. Verification and Validation

### 7.1. Golden Reference Validation

- Users provide a _golden reference_ output file (e.g., `.npy`, `.csv`, `.txt`).
- After JIT execution, GUI automatically compares the NPU output to the reference.
- The tool computes and displays:
    - Pass/fail result per output tensor.
    - Mean absolute or relative error.

### 7.2. Graph Validation

Before execution:

- Verify all node connections are valid.
- Ensure every operation is assigned to a tile.
- Check for missing or incompatible data types.

### 7.3. Success Criteria

| Metric                        | Target                                   |
| ----------------------------- | ---------------------------------------- |
| Successful IRON JIT execution | 100% of valid graphs                     |
| Stable runtime logs           | 100% of compiler/runtime output captured |
| GUI responsiveness            | < 100 ms per user interaction            |

---

## 8. Performance Expectations

- Supports up to **32 tiles/16 tiles** per project for AIE2/AIE1.
- GUI remains responsive during mapping and run phases.
- Designed for local Ryzen NPU execution (no remote mode).

---

## 9. Development Roadmap

### Phase 1 — MVP

**Goal:** Build the complete basic flow from graph creation to verification.

| Component       | Deliverable                                        |
| --------------- | -------------------------------------------------- |
| GUI Editor      | Node/edge creation and deletion                    |
| AIE Grid        | Visual tile layout with drag-and-drop mapping      |
| JIT Execution   | Run IRON JIT and capture runtime logs              |
| Verification    | Compare outputs with golden references             |
| Validation      | Basic checks for connectivity and mapping          |
| Prebuilt Blocks | Example templates (e.g., GEMM, eltwise operations) |

---

### Phase 2 — Enhancements

**Goal:** Improve usability and visualization features.

| Component          | Deliverable                                        |
| ------------------ | -------------------------------------------------- |
| Tile Visualization | Show active/idle tiles during execution            |
| Graph Editor       | Add undo/redo, snapping, and zoom/pan              |
| Logging            | Centralized log viewer with runtime output         |
| Save/Load          | Export and import projects as `.json` files        |
| Verification       | Configurable numeric tolerance and summary reports |
| End-to-End Flow    | 1 or 2 applications implemented using the GUI      |

---

## 10. Deliverables Summary

- Functional standalone GUI application.
- Visual mapping of dataflows to AIE tiles.
- End-to-end JIT execution workflow using IRON.
- Golden reference verification system.
- 2 example applications, one of which must be a DNN, showcasing the utility of the GUI.
- User documentation.
- Research paper.

---

## 11. Success Definition

The project is **successful** when:

- A user can visually design a dataflow, map it to tiles, and execute it using IRON JIT.
- The GUI automatically compares results to a golden reference and reports correctness.
- All major features work without requiring command-line interaction.
- 2 applications have been implemented to showcase the end-to-end flow of the GUI.
- A research paper has been written, ideally one that is conference-ready.

---

**Last Updated:** November 2025  
**Project Owners/Contact:** Prof. Aman Arora