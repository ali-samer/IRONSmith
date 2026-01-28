"""
AIECAD High-Level Intermediate Representation (HLIR)

A clean, XML-free Python IR for AIECAD designs. This package provides:
- Semantic model classes for AIE programs
- Fluent builder API for construction
- Graph-based representation with ObjectFifos as edges
- Rich type system for tensor types
- XML serialization support

Example usage:
    from aiecad_compiler.hlir import Program, ProgramBuilder

    # Create program using fluent builder
    builder = ProgramBuilder("my_program")

    # Add tiles
    shim = builder.add_tile(kind="shim", x=0, y=0)
    mem = builder.add_tile(kind="mem", x=0, y=1)
    compute = builder.add_tile(kind="compute", x=0, y=2)

    # Define types
    chunk_ty = builder.add_tensor_type("chunk_ty", shape=[1024], dtype="int32")

    # Create FIFOs
    fifo_in = builder.add_fifo("fifo_in", obj_type=chunk_ty, depth=2)
    fifo_out = builder.add_fifo_forward("fifo_out", source=fifo_in)

    # Export to XML
    xml_str = builder.to_xml()
"""

from .types import DataType, TensorType, ScalarType
from .core import (
    Program,
    Tile,
    ObjectFifo,
    ExternalKernel,
    CoreFunction,
    Worker,
    RuntimeSequence,
    Symbol
)
from .operations import (
    FifoOperation,
    SplitOperation,
    JoinOperation,
    ForwardOperation,
    FillOperation,
    DrainOperation,
    TensorAccessPattern
)
from .builder import ProgramBuilder
from .serializer import XMLSerializer
from .gui_serializer import GUIXMLSerializer

__all__ = [
    # Types
    "DataType",
    "TensorType",
    "ScalarType",

    # Core
    "Program",
    "Tile",
    "ObjectFifo",
    "ExternalKernel",
    "CoreFunction",
    "Worker",
    "RuntimeSequence",
    "Symbol",

    # Operations
    "FifoOperation",
    "SplitOperation",
    "JoinOperation",
    "ForwardOperation",
    "FillOperation",
    "DrainOperation",
    "TensorAccessPattern",

    # Builder & Serializer
    "ProgramBuilder",
    "XMLSerializer",
    "GUIXMLSerializer",
]

__version__ = "1.0.0"
