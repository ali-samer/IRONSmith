"""
Fluent builder API for constructing AIECAD programs.

Provides user-friendly methods for building programs without
manually wiring every field. Enforces invariants and provides
helpful validation.
"""

from typing import List, Union, Optional, Dict, Any
from .core import (
    Program, Tile, TileKind, ObjectFifo, ExternalKernel,
    CoreFunction, Worker, RuntimeSequence, Symbol,
    FifoAccessMode, FifoBinding, Acquire, Release, KernelCall,
    RuntimeFill, RuntimeDrain
)
from .operations import (
    SplitOperation, JoinOperation, ForwardOperation,
    TensorAccessPattern, FillOperation, DrainOperation
)
from .types import DataType, TensorType, AnyType, make_tensor_type


class ProgramBuilder:
    """
    Fluent builder for constructing AIECAD programs.

    Example usage:
        builder = ProgramBuilder("my_program")

        # Add tiles
        shim = builder.add_tile("shim0", kind="shim", x=0, y=0)
        compute = builder.add_tile("compute0", kind="compute", x=0, y=2)

        # Add types
        chunk_ty = builder.add_tensor_type("chunk_ty", shape=[1024], dtype="int32")

        # Add FIFOs
        fifo_in = builder.add_fifo("fifo_in", obj_type="chunk_ty", depth=2)

        # Build program
        program = builder.build()
    """

    def __init__(self, name: str):
        """
        Initialize a new program builder.

        Args:
            name: Program name
        """
        self.program = Program(name=name)

    def add_symbol(self, name: str, value: Any, type_hint: Optional[str] = None,
                   is_constant: bool = False) -> 'ProgramBuilder':
        """
        Add a symbol (variable, constant, type alias) to the program.

        Args:
            name: Symbol name
            value: Symbol value
            type_hint: Optional type hint
            is_constant: Whether this is a constant

        Returns:
            Self for chaining

        Raises:
            ValueError: If symbol name already exists
        """
        if name in self.program.symbols:
            raise ValueError(f"Symbol '{name}' already exists")

        self.program.symbols[name] = Symbol(
            name=name,
            value=value,
            type_hint=type_hint,
            is_constant=is_constant
        )
        return self

    def add_constant(self, name: str, value: Union[int, float, str],
                    type_hint: Optional[str] = None) -> 'ProgramBuilder':
        """
        Add a constant to the program.

        Args:
            name: Constant name
            value: Constant value
            type_hint: Optional type hint

        Returns:
            Self for chaining
        """
        return self.add_symbol(name, value, type_hint, is_constant=True)

    def add_tensor_type(self, name: str, shape: List[Union[int, str]],
                       dtype: Union[str, DataType],
                       layout: Optional[str] = None) -> TensorType:
        """
        Add a tensor type definition to the program.

        Args:
            name: Type name
            shape: Shape dimensions (can be symbolic)
            dtype: Data type
            layout: Optional layout hint

        Returns:
            Created TensorType

        Example:
            >>> builder.add_tensor_type("chunk_ty", shape=[1024], dtype="int32")
        """
        tensor_ty = make_tensor_type(shape, dtype, layout)
        self.add_symbol(name, tensor_ty, type_hint="TensorType")
        return tensor_ty

    def add_tile(self, name: str, kind: Union[str, TileKind],
                 x: int, y: int, **metadata) -> Tile:
        """
        Add a tile to the program.

        Args:
            name: Unique tile name
            kind: Tile kind (shim, mem, compute)
            x: Column coordinate
            y: Row coordinate
            **metadata: Additional properties

        Returns:
            Created Tile

        Raises:
            ValueError: If tile name already exists
        """
        if name in self.program.tiles:
            raise ValueError(f"Tile '{name}' already exists")

        if isinstance(kind, str):
            kind = TileKind(kind.lower())

        tile = Tile(name=name, kind=kind, x=x, y=y, metadata=metadata)
        self.program.tiles[name] = tile
        return tile

    def add_fifo(self, name: str, obj_type: Union[AnyType, str],
                 depth: int = 2, producer: Optional[Union[Tile, str]] = None,
                 consumers: Optional[List[Union[Tile, str]]] = None,
                 **metadata) -> ObjectFifo:
        """
        Add an ObjectFifo to the program.

        Args:
            name: Unique FIFO name
            obj_type: Type of data (TensorType or type name)
            depth: Buffer depth (default 2)
            producer: Optional producer tile
            consumers: Optional consumer tiles
            **metadata: Additional properties (context, direction, etc.)

        Returns:
            Created ObjectFifo

        Raises:
            ValueError: If FIFO name already exists
        """
        if name in self.program.fifos:
            raise ValueError(f"FIFO '{name}' already exists")

        # Resolve tile references
        if isinstance(producer, str):
            producer = self.program.tiles.get(producer)

        if consumers:
            resolved_consumers = []
            for c in consumers:
                if isinstance(c, str):
                    c = self.program.tiles.get(c)
                if c:
                    resolved_consumers.append(c)
            consumers = resolved_consumers
        else:
            consumers = []

        fifo = ObjectFifo(
            name=name,
            obj_type=obj_type,
            depth=depth,
            producer=producer,
            consumers=consumers,
            metadata=metadata
        )
        self.program.fifos[name] = fifo
        return fifo

    def add_fifo_split(self, name: str, source: Union[ObjectFifo, str],
                       num_outputs: int, output_type: Union[AnyType, str],
                       output_names: List[str], offsets: List[Union[int, str]],
                       placement: Union[Tile, str], **metadata) -> SplitOperation:
        """
        Add a split operation on a FIFO.

        Args:
            name: Name of split result
            source: Source FIFO to split
            num_outputs: Number of outputs
            output_type: Type of each output
            output_names: Names for each output
            offsets: Byte offsets for each output
            placement: Tile where split occurs
            **metadata: Additional properties

        Returns:
            Created SplitOperation
        """
        if isinstance(source, str):
            source = self.program.fifos.get(source, source)
        if isinstance(placement, str):
            placement = self.program.tiles.get(placement, placement)

        split_op = SplitOperation(
            name=name,
            source=source,
            num_outputs=num_outputs,
            output_type=output_type,
            output_names=output_names,
            offsets=offsets,
            placement=placement,
            metadata=metadata
        )

        # Store split result as a symbol so it can be referenced
        self.add_symbol(name, split_op, type_hint="SplitOperation")
        return split_op

    def add_fifo_join(self, name: str, dest: Union[ObjectFifo, str],
                      num_inputs: int, input_type: Union[AnyType, str],
                      input_names: List[str], offsets: List[Union[int, str]],
                      placement: Union[Tile, str], **metadata) -> JoinOperation:
        """
        Add a join operation on a FIFO.

        Args:
            name: Name of join result
            dest: Destination FIFO to join into
            num_inputs: Number of inputs
            input_type: Type of each input
            input_names: Names for each input
            offsets: Byte offsets for each input
            placement: Tile where join occurs
            **metadata: Additional properties

        Returns:
            Created JoinOperation
        """
        if isinstance(dest, str):
            dest = self.program.fifos.get(dest, dest)
        if isinstance(placement, str):
            placement = self.program.tiles.get(placement, placement)

        join_op = JoinOperation(
            name=name,
            dest=dest,
            num_inputs=num_inputs,
            input_type=input_type,
            input_names=input_names,
            offsets=offsets,
            placement=placement,
            metadata=metadata
        )

        # Store join result as a symbol
        self.add_symbol(name, join_op, type_hint="JoinOperation")
        return join_op

    def add_fifo_forward(self, name: str, source: Union[ObjectFifo, str],
                        **metadata) -> ForwardOperation:
        """
        Add a forward operation on a FIFO.

        Args:
            name: Name of forward result
            source: Source FIFO to forward
            **metadata: Additional properties

        Returns:
            Created ForwardOperation
        """
        if isinstance(source, str):
            source = self.program.fifos.get(source, source)

        forward_op = ForwardOperation(
            name=name,
            source=source,
            metadata=metadata
        )

        # Store forward result as a symbol
        self.add_symbol(name, forward_op, type_hint="ForwardOperation")
        return forward_op

    def add_external_kernel(self, name: str, kernel_name: str,
                           source_file: str, arg_types: List[Union[AnyType, str]],
                           include_dirs: Optional[List[str]] = None,
                           **metadata) -> ExternalKernel:
        """
        Add an external kernel declaration.

        Args:
            name: Unique kernel name
            kernel_name: C/C++ function name
            source_file: Path to source file
            arg_types: Argument types
            include_dirs: Optional include directories
            **metadata: Additional properties

        Returns:
            Created ExternalKernel

        Raises:
            ValueError: If kernel name already exists
        """
        if name in self.program.external_kernels:
            raise ValueError(f"External kernel '{name}' already exists")

        kernel = ExternalKernel(
            name=name,
            kernel_name=kernel_name,
            source_file=source_file,
            arg_types=arg_types,
            include_dirs=include_dirs or [],
            metadata=metadata
        )
        self.program.external_kernels[name] = kernel
        return kernel

    def add_core_function(self, name: str, parameters: List[str],
                         acquires: Optional[List[tuple]] = None,
                         kernel_call: Optional[tuple] = None,
                         releases: Optional[List[tuple]] = None,
                         **metadata) -> CoreFunction:
        """
        Add a core function definition.

        Args:
            name: Function name
            parameters: Parameter names (first is kernel, rest are FIFOs)
            acquires: List of (fifo_param, count, local_var) tuples
            kernel_call: Tuple of (kernel_param, args_list)
            releases: List of (fifo_param, count) tuples
            **metadata: Additional properties

        Returns:
            Created CoreFunction

        Raises:
            ValueError: If function name already exists

        Example:
            >>> builder.add_core_function(
            ...     "add_fn",
            ...     parameters=["kernel", "fifoA", "fifoB", "fifoC"],
            ...     acquires=[("fifoA", 1, "elemA"), ("fifoB", 1, "elemB"), ("fifoC", 1, "elemC")],
            ...     kernel_call=("kernel", ["elemA", "elemB", "elemC"]),
            ...     releases=[("fifoA", 1), ("fifoB", 1), ("fifoC", 1)]
            ... )
        """
        if name in self.program.core_functions:
            raise ValueError(f"Core function '{name}' already exists")

        # Convert tuples to proper objects
        acquire_objs = []
        if acquires:
            for fifo_param, count, local_var in acquires:
                acquire_objs.append(Acquire(fifo_param, count, local_var))

        kernel_call_obj = None
        if kernel_call:
            kernel_param, args = kernel_call
            kernel_call_obj = KernelCall(kernel_param, args)

        release_objs = []
        if releases:
            for fifo_param, count in releases:
                release_objs.append(Release(fifo_param, count))

        func = CoreFunction(
            name=name,
            parameters=parameters,
            acquires=acquire_objs,
            kernel_call=kernel_call_obj,
            releases=release_objs,
            metadata=metadata
        )
        self.program.core_functions[name] = func
        return func

    def add_worker(self, name: str, core_fn: Union[CoreFunction, str],
                   fn_args: List[Union[str, tuple]], placement: Union[Tile, str],
                   **metadata) -> Worker:
        """
        Add a worker to the program.

        Args:
            name: Unique worker name
            core_fn: CoreFunction to execute (or name)
            fn_args: List of arguments - first is kernel name, rest are (fifo, mode, index) tuples
            placement: Tile where worker executes
            **metadata: Additional properties

        Returns:
            Created Worker

        Raises:
            ValueError: If worker name already exists

        Example:
            >>> builder.add_worker(
            ...     "worker_0",
            ...     core_fn="add_fn",
            ...     fn_args=["kernel", ("fifo_a", "cons", 0), ("fifo_b", "cons", 0), ("fifo_c", "prod", None)],
            ...     placement="tile_compute0"
            ... )
        """
        if name in self.program.workers:
            raise ValueError(f"Worker '{name}' already exists")

        # Resolve references
        if isinstance(core_fn, str):
            core_fn = self.program.core_functions.get(core_fn, core_fn)
        if isinstance(placement, str):
            placement = self.program.tiles.get(placement, placement)

        # Convert fn_args tuples to proper objects
        processed_args = []
        for arg in fn_args:
            if isinstance(arg, tuple):
                # It's a FIFO binding: (fifo, mode, index)
                fifo, mode, index = arg
                if isinstance(fifo, str):
                    fifo = self.program.fifos.get(fifo, fifo)
                if isinstance(mode, str):
                    mode = FifoAccessMode(mode.lower())
                processed_args.append(FifoBinding(fifo, mode, index))
            elif isinstance(arg, str):
                # It's a reference to external kernel or other symbol
                processed_args.append(arg)
            else:
                processed_args.append(arg)

        worker = Worker(
            name=name,
            core_fn=core_fn,
            fn_args=processed_args,
            placement=placement,
            metadata=metadata
        )
        self.program.workers[name] = worker
        return worker

    def create_runtime(self, name: str = "runtime") -> 'RuntimeBuilder':
        """
        Create a runtime sequence builder.

        Args:
            name: Runtime name

        Returns:
            RuntimeBuilder for fluent construction
        """
        return RuntimeBuilder(self, name)

    def set_runtime(self, runtime: RuntimeSequence) -> 'ProgramBuilder':
        """
        Set the program's runtime sequence.

        Args:
            runtime: RuntimeSequence to set

        Returns:
            Self for chaining
        """
        self.program.runtime = runtime
        return self

    def build(self) -> Program:
        """
        Build and validate the final program.

        Returns:
            Constructed Program

        Raises:
            ValueError: If program is invalid
        """
        errors = self.program.validate()
        if errors:
            error_msg = "\n  - ".join(["Program validation failed:"] + errors)
            raise ValueError(error_msg)

        return self.program

    def get_program(self) -> Program:
        """Get the current program (without validation)."""
        return self.program


class RuntimeBuilder:
    """
    Builder for constructing runtime sequences.

    Example:
        rt = builder.create_runtime()
        rt.add_input_type("vector_ty", [4096], "int32")
        rt.add_output_type("vector_ty", [4096], "int32")
        rt.add_params(["a_in", "c_out"])
        rt.add_worker("worker_0")
        rt.add_fill("fill_0", "fifo_in", "a_in", "shim0")
        rt.add_drain("drain_0", "fifo_out", "c_out", "shim0", wait=True)
        rt.build()
    """

    def __init__(self, prog_builder: ProgramBuilder, name: str):
        """
        Initialize runtime builder.

        Args:
            prog_builder: Parent program builder
            name: Runtime name
        """
        self.prog_builder = prog_builder
        self.runtime = RuntimeSequence(name=name)

    def add_input_type(self, type_ref: Union[AnyType, str]) -> 'RuntimeBuilder':
        """
        Add an input type to the runtime sequence.

        Args:
            type_ref: Type reference or TensorType

        Returns:
            Self for chaining
        """
        self.runtime.input_types.append(type_ref)
        return self

    def add_output_type(self, type_ref: Union[AnyType, str]) -> 'RuntimeBuilder':
        """
        Add an output type to the runtime sequence.

        Args:
            type_ref: Type reference or TensorType

        Returns:
            Self for chaining
        """
        self.runtime.output_types.append(type_ref)
        return self

    def add_params(self, param_names: List[str]) -> 'RuntimeBuilder':
        """
        Set runtime parameter names.

        Args:
            param_names: List of parameter names

        Returns:
            Self for chaining
        """
        self.runtime.param_names = param_names
        return self

    def add_worker(self, worker: Union[Worker, str]) -> 'RuntimeBuilder':
        """
        Add a worker to start in the runtime.

        Args:
            worker: Worker object or name

        Returns:
            Self for chaining
        """
        if isinstance(worker, str):
            worker = self.prog_builder.program.workers.get(worker, worker)
        self.runtime.workers.append(worker)
        return self

    def add_fill(self, name: str, fifo: Union[ObjectFifo, str],
                 source_param: str, placement: Union[Tile, str],
                 tap: Optional[TensorAccessPattern] = None,
                 **metadata) -> 'RuntimeBuilder':
        """
        Add a fill operation.

        Args:
            name: Operation name
            fifo: Target FIFO
            source_param: Source parameter name
            placement: Tile where fill occurs
            tap: Optional tensor access pattern
            **metadata: Additional properties

        Returns:
            Self for chaining
        """
        if isinstance(fifo, str):
            fifo = self.prog_builder.program.fifos.get(fifo, fifo)
        if isinstance(placement, str):
            placement = self.prog_builder.program.tiles.get(placement, placement)

        fill_op = RuntimeFill(
            placement=placement,
            fifo=fifo,
            source_param=source_param,
            tap=tap
        )
        self.runtime.operations.append(fill_op)
        return self

    def add_drain(self, name: str, fifo: Union[ObjectFifo, str],
                  dest_param: str, placement: Union[Tile, str],
                  wait: bool = True, tap: Optional[TensorAccessPattern] = None,
                  **metadata) -> 'RuntimeBuilder':
        """
        Add a drain operation.

        Args:
            name: Operation name
            fifo: Source FIFO
            dest_param: Destination parameter name
            placement: Tile where drain occurs
            wait: Whether to wait for completion
            tap: Optional tensor access pattern
            **metadata: Additional properties

        Returns:
            Self for chaining
        """
        if isinstance(fifo, str):
            fifo = self.prog_builder.program.fifos.get(fifo, fifo)
        if isinstance(placement, str):
            placement = self.prog_builder.program.tiles.get(placement, placement)

        drain_op = RuntimeDrain(
            placement=placement,
            fifo=fifo,
            dest_param=dest_param,
            wait=wait,
            tap=tap
        )
        self.runtime.operations.append(drain_op)
        return self

    def build(self) -> RuntimeSequence:
        """
        Build the runtime and attach it to the program.

        Returns:
            Constructed RuntimeSequence
        """
        self.prog_builder.set_runtime(self.runtime)
        return self.runtime
