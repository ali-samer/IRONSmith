"""
Type system for AIECAD HLIR.

Provides rich type representations for tensors, scalars, and complex data types
used in AIE programming.
"""

from dataclasses import dataclass, field
from typing import List, Union, Optional
from enum import Enum


class DataType(Enum):
    """Scalar data types supported by AIE."""

    # Integer types
    INT8 = "int8"
    INT16 = "int16"
    INT32 = "int32"
    INT64 = "int64"
    UINT8 = "uint8"
    UINT16 = "uint16"
    UINT32 = "uint32"
    UINT64 = "uint64"

    # Floating point types
    FLOAT16 = "float16"
    FLOAT32 = "float32"
    FLOAT64 = "float64"
    BFLOAT16 = "bfloat16"

    def __str__(self):
        return self.value


@dataclass(frozen=True)
class ScalarType:
    """Represents a scalar type."""

    dtype: DataType

    def __str__(self):
        return str(self.dtype)


@dataclass(frozen=True)
class TensorType:
    """
    Represents a tensor type with shape and data type.

    The shape can contain symbolic expressions (as strings) or concrete integers.
    Examples:
        - TensorType(shape=[128], dtype=DataType.INT32)
        - TensorType(shape=["N"], dtype=DataType.FLOAT32)
        - TensorType(shape=["N // 4"], dtype=DataType.BFLOAT16)
    """

    shape: tuple[Union[int, str], ...]  # Shape dimensions (can be symbolic)
    dtype: DataType
    layout: Optional[str] = None  # Optional layout hint (e.g., "row_major")

    def __post_init__(self):
        # Convert list to tuple if needed
        if isinstance(self.shape, list):
            object.__setattr__(self, 'shape', tuple(self.shape))

    def __str__(self):
        shape_str = ", ".join(str(d) for d in self.shape)
        result = f"Tensor[{shape_str}]<{self.dtype}>"
        if self.layout:
            result += f"@{self.layout}"
        return result

    @property
    def ndim(self) -> int:
        """Number of dimensions."""
        return len(self.shape)

    @property
    def is_symbolic(self) -> bool:
        """True if any dimension is symbolic (not a concrete int)."""
        return any(isinstance(d, str) for d in self.shape)


# Type alias for any type representation
AnyType = Union[ScalarType, TensorType, str]


def parse_dtype(dtype_str: str) -> DataType:
    """
    Parse a data type string to DataType enum.

    Args:
        dtype_str: String like "int32", "bfloat16", etc.

    Returns:
        DataType enum value

    Raises:
        ValueError: If dtype_str is not recognized
    """
    try:
        return DataType(dtype_str.lower())
    except ValueError:
        valid = ", ".join(dt.value for dt in DataType)
        raise ValueError(f"Unknown data type '{dtype_str}'. Valid types: {valid}")


def make_tensor_type(shape: List[Union[int, str]], dtype: Union[str, DataType],
                     layout: Optional[str] = None) -> TensorType:
    """
    Convenience function to create a TensorType.

    Args:
        shape: List of dimensions (int or symbolic string)
        dtype: Data type as string or DataType enum
        layout: Optional layout hint

    Returns:
        TensorType instance

    Example:
        >>> make_tensor_type([128, 128], "float32")
        TensorType(shape=(128, 128), dtype=DataType.FLOAT32)
    """
    if isinstance(dtype, str):
        dtype = parse_dtype(dtype)
    return TensorType(shape=tuple(shape), dtype=dtype, layout=layout)
