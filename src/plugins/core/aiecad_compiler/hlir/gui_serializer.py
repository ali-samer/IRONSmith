"""
GUI XML serializer for AIECAD HLIR.

Converts HLIR Program objects to GUI XML format (simplified format)
which is then processed by XMLGenerator to create complete XML.

This is the correct serialization path:
  HLIR → GUIXMLSerializer → GUI XML → XMLGenerator → Complete XML → CodeGenerator
"""

from typing import Union, List, Any, Optional
from xml.etree.ElementTree import Element, SubElement, tostring
from xml.dom import minidom
from .core import (
    Program, Tile, ObjectFifo, ExternalKernel, CoreFunction,
    Worker, RuntimeSequence, Symbol, FifoBinding, FifoAccessMode,
    RuntimeFill, RuntimeDrain
)
from .operations import (
    SplitOperation, JoinOperation, ForwardOperation,
    TensorAccessPattern
)
from .types import TensorType, DataType


class GUIXMLSerializer:
    """
    Serializes HLIR Program to GUI XML format.

    GUI XML is the simplified format designed for visual editors.
    It uses simplified syntax that is then expanded by XMLGenerator.

    Example:
        serializer = GUIXMLSerializer()
        xml_str = serializer.serialize(program)
    """

    def __init__(self, pretty_print: bool = True):
        """
        Initialize serializer.

        Args:
            pretty_print: Whether to format output with indentation
        """
        self.pretty_print = pretty_print

    def serialize(self, program: Program) -> str:
        """
        Serialize a Program to GUI XML string.

        Args:
            program: Program to serialize

        Returns:
            GUI XML string
        """
        root = self._build_xml(program)

        if self.pretty_print:
            return self._prettify(root)
        else:
            return tostring(root, encoding='unicode')

    def serialize_to_file(self, program: Program, filepath: str):
        """
        Serialize a Program to GUI XML file.

        Args:
            program: Program to serialize
            filepath: Output file path
        """
        xml_str = self.serialize(program)
        with open(filepath, 'w', encoding='utf-8') as f:
            f.write(xml_str)

    def _prettify(self, elem: Element) -> str:
        """Format XML with indentation."""
        rough_string = tostring(elem, encoding='unicode')
        reparsed = minidom.parseString(rough_string)
        # Get pretty XML and remove extra blank lines
        pretty = reparsed.toprettyxml(indent="    ")
        # Remove blank lines
        lines = [line for line in pretty.split('\n') if line.strip()]
        return '\n'.join(lines)

    def _build_xml(self, program: Program) -> Element:
        """Build GUI XML tree from Program."""
        root = Element('Module')
        root.set('name', program.name)
        root.text = '\n'
        root.tail = '\n'

        # Add Symbols section
        if program.symbols or program.fifos:
            symbols_elem = SubElement(root, 'Symbols')
            symbols_elem.text = '\n'
            symbols_elem.tail = '\n'
            self._add_symbols(symbols_elem, program)

        # Add DataFlow section
        if program.fifos or program.workers or program.runtime:
            dataflow_elem = SubElement(root, 'DataFlow')
            dataflow_elem.text = '\n'
            dataflow_elem.tail = '\n'
            self._add_dataflow(dataflow_elem, program)

        # Add Functions section (if we have runtime)
        if program.runtime:
            self._add_functions(root, program)

        # Add EntryPoint section (if we have runtime)
        if program.runtime:
            self._add_entrypoint(root, program)

        return root

    def _add_symbols(self, parent: Element, program: Program):
        """Add symbols section in GUI XML format."""
        # Add type definitions
        for symbol in program.symbols.values():
            if isinstance(symbol.value, TensorType):
                self._add_gui_type_abstraction(parent, symbol.name, symbol.value)

    def _add_gui_type_abstraction(self, parent: Element, name: str, tensor_type: TensorType):
        """Add TensorType in GUI XML format (simplified)."""
        type_elem = SubElement(parent, 'TypeAbstraction')
        type_elem.set('name', name)
        type_elem.text = '\n'
        type_elem.tail = '\n'

        ndarray_elem = SubElement(type_elem, 'ndarray')
        ndarray_elem.text = '\n'
        ndarray_elem.tail = '\n'

        # Shape - simplified format
        shape_elem = SubElement(ndarray_elem, 'shape')
        # Convert shape to simple string expression
        shape_str = self._shape_to_string(tensor_type.shape)
        shape_elem.text = shape_str
        shape_elem.tail = '\n'

        # Dtype - simplified format
        dtype_elem = SubElement(ndarray_elem, 'dtype')
        dtype_elem.text = str(tensor_type.dtype.value)
        dtype_elem.tail = '\n'

    def _shape_to_string(self, shape: tuple) -> str:
        """Convert shape tuple to GUI XML string format."""
        if len(shape) == 1:
            return str(shape[0])
        else:
            # Multi-dimensional: return as comma-separated
            return ', '.join(str(d) for d in shape)

    def _add_dataflow(self, parent: Element, program: Program):
        """Add DataFlow section in GUI XML format."""
        # Add ObjectFifos
        for fifo in program.fifos.values():
            self._add_gui_object_fifo(parent, fifo)

        # Add split/join/forward operations from symbols
        for symbol in program.symbols.values():
            if isinstance(symbol.value, ForwardOperation):
                self._add_gui_forward_operation(parent, symbol.value)
            elif isinstance(symbol.value, SplitOperation):
                self._add_gui_split_operation(parent, symbol.value)
            elif isinstance(symbol.value, JoinOperation):
                self._add_gui_join_operation(parent, symbol.value)

        # Add Workers
        for worker in program.workers.values():
            self._add_gui_worker(parent, worker)

        # Add Runtime
        if program.runtime:
            self._add_gui_runtime(parent, program.runtime)

        # Add Program definition
        self._add_gui_program(parent, program)

    def _add_gui_object_fifo(self, parent: Element, fifo: ObjectFifo):
        """Add ObjectFifo in GUI XML format."""
        fifo_elem = SubElement(parent, 'ObjectFifo')
        fifo_elem.set('name', fifo.name)
        fifo_elem.text = '\n'
        fifo_elem.tail = '\n'

        # Type - simple child element
        type_elem = SubElement(fifo_elem, 'type')
        type_elem.text = str(fifo.obj_type)
        type_elem.tail = '\n'

        # Depth - simple child element
        depth_elem = SubElement(fifo_elem, 'depth')
        depth_elem.text = str(fifo.depth)
        depth_elem.tail = '\n'

    def _add_gui_forward_operation(self, parent: Element, forward_op: ForwardOperation):
        """Add forward operation in GUI XML format."""
        forward_elem = SubElement(parent, 'ObjectFifoForward')
        forward_elem.set('name', forward_op.name)
        source_name = forward_op.source if isinstance(forward_op.source, str) else forward_op.source.name
        forward_elem.set('source', source_name)
        forward_elem.tail = '\n'

    def _add_gui_split_operation(self, parent: Element, split_op: SplitOperation):
        """Add split operation in GUI XML format."""
        split_elem = SubElement(parent, 'ObjectFifoSplit')
        split_elem.set('name', split_op.name)
        source_name = split_op.source if isinstance(split_op.source, str) else split_op.source.name
        split_elem.set('source', source_name)
        split_elem.set('num_outputs', str(split_op.num_outputs))
        split_elem.text = '\n'
        split_elem.tail = '\n'

        # Output type
        type_elem = SubElement(split_elem, 'output_type')
        type_elem.text = str(split_op.output_type)
        type_elem.tail = '\n'

        # Output names
        names_elem = SubElement(split_elem, 'output_names')
        names_elem.text = ', '.join(split_op.output_names)
        names_elem.tail = '\n'

        # Offsets
        offsets_elem = SubElement(split_elem, 'offsets')
        offsets_elem.text = ', '.join(str(o) for o in split_op.offsets)
        offsets_elem.tail = '\n'

        # Placement
        place_elem = SubElement(split_elem, 'placement')
        place_elem.text = f"Tile({split_op.placement.x}, {split_op.placement.y})"
        place_elem.tail = '\n'

    def _add_gui_join_operation(self, parent: Element, join_op: JoinOperation):
        """Add join operation in GUI XML format."""
        join_elem = SubElement(parent, 'ObjectFifoJoin')
        join_elem.set('name', join_op.name)
        dest_name = join_op.dest if isinstance(join_op.dest, str) else join_op.dest.name
        join_elem.set('dest', dest_name)
        join_elem.set('num_inputs', str(join_op.num_inputs))
        join_elem.text = '\n'
        join_elem.tail = '\n'

        # Input type
        type_elem = SubElement(join_elem, 'input_type')
        type_elem.text = str(join_op.input_type)
        type_elem.tail = '\n'

        # Input names
        names_elem = SubElement(join_elem, 'input_names')
        names_elem.text = ', '.join(join_op.input_names)
        names_elem.tail = '\n'

        # Offsets
        offsets_elem = SubElement(join_elem, 'offsets')
        offsets_elem.text = ', '.join(str(o) for o in join_op.offsets)
        offsets_elem.tail = '\n'

        # Placement
        place_elem = SubElement(join_elem, 'placement')
        place_elem.text = f"Tile({join_op.placement.x}, {join_op.placement.y})"
        place_elem.tail = '\n'

    def _add_gui_worker(self, parent: Element, worker: Worker):
        """Add Worker in GUI XML format."""
        # Note: Workers in GUI XML might need external kernel and core function info
        # This is a simplified version
        worker_elem = SubElement(parent, 'Worker')
        worker_elem.set('name', worker.name)
        worker_elem.text = '\n'
        worker_elem.tail = '\n'

        # Core function reference
        cf_name = worker.core_fn if isinstance(worker.core_fn, str) else worker.core_fn.name
        cf_elem = SubElement(worker_elem, 'core_function')
        cf_elem.text = cf_name
        cf_elem.tail = '\n'

        # Placement
        place_elem = SubElement(worker_elem, 'placement')
        place_elem.text = f"Tile({worker.placement.x}, {worker.placement.y})"
        place_elem.tail = '\n'

    def _add_gui_runtime(self, parent: Element, runtime: RuntimeSequence):
        """Add Runtime in GUI XML format."""
        rt_elem = SubElement(parent, 'Runtime')
        rt_elem.set('name', runtime.name)
        rt_elem.text = '\n'
        rt_elem.tail = '\n'

        # Sequence
        seq_elem = SubElement(rt_elem, 'Sequence')

        # Input/output types as comma-separated string
        all_types = runtime.input_types + runtime.output_types
        if all_types:
            types_str = ', '.join(str(t) for t in all_types)
            seq_elem.set('inputs', types_str)

        # Parameter names as comma-separated string
        if runtime.param_names:
            params_str = ', '.join(runtime.param_names)
            seq_elem.set('as', params_str)

        seq_elem.text = '\n'
        seq_elem.tail = '\n'

        # Start workers (if any)
        if runtime.workers:
            start_elem = SubElement(seq_elem, 'StartWorkers')
            worker_names = []
            for w in runtime.workers:
                name = w if isinstance(w, str) else w.name
                worker_names.append(name)
            start_elem.text = ', '.join(worker_names)
            start_elem.tail = '\n'

        # Operations (fill/drain)
        for op in runtime.operations:
            if isinstance(op, RuntimeFill):
                self._add_gui_fill_operation(seq_elem, op)
            elif isinstance(op, RuntimeDrain):
                self._add_gui_drain_operation(seq_elem, op)

    def _add_gui_fill_operation(self, parent: Element, fill_op: RuntimeFill):
        """Add Fill operation in GUI XML format."""
        fill_elem = SubElement(parent, 'Fill')

        # Target FIFO
        fifo_name = fill_op.fifo if isinstance(fill_op.fifo, str) else fill_op.fifo.name
        fill_elem.set('target', fifo_name)

        # Source parameter
        fill_elem.set('source', fill_op.source_param)

        # Use TAP
        use_tap = "true" if fill_op.tap else "false"
        fill_elem.set('use_tap', use_tap)

        fill_elem.text = '\n'
        fill_elem.tail = '\n'

        # Placement
        place_elem = SubElement(fill_elem, 'placement')
        place_elem.text = f"Tile({fill_op.placement.x}, {fill_op.placement.y})"
        place_elem.tail = '\n'

        # TAP if present
        if fill_op.tap:
            self._add_gui_tap(fill_elem, fill_op.tap)

    def _add_gui_drain_operation(self, parent: Element, drain_op: RuntimeDrain):
        """Add Drain operation in GUI XML format."""
        drain_elem = SubElement(parent, 'Drain')

        # Source FIFO
        fifo_name = drain_op.fifo if isinstance(drain_op.fifo, str) else drain_op.fifo.name
        drain_elem.set('source', fifo_name)

        # Target parameter
        drain_elem.set('target', drain_op.dest_param)

        # Use TAP
        use_tap = "true" if drain_op.tap else "false"
        drain_elem.set('use_tap', use_tap)

        drain_elem.text = '\n'
        drain_elem.tail = '\n'

        # Placement
        place_elem = SubElement(drain_elem, 'placement')
        place_elem.text = f"Tile({drain_op.placement.x}, {drain_op.placement.y})"
        place_elem.tail = '\n'

        # Wait
        if drain_op.wait:
            wait_elem = SubElement(drain_elem, 'wait')
            wait_elem.text = "true"
            wait_elem.tail = '\n'

        # TAP if present
        if drain_op.tap:
            self._add_gui_tap(drain_elem, drain_op.tap)

    def _add_gui_tap(self, parent: Element, tap: TensorAccessPattern):
        """Add TensorAccessPattern in GUI XML format."""
        tap_elem = SubElement(parent, 'TensorAccessPattern')
        tap_elem.text = '\n'
        tap_elem.tail = '\n'

        # Tensor dims
        dims_elem = SubElement(tap_elem, 'tensor_dims')
        dims_elem.text = ', '.join(str(d) for d in tap.tensor_dims)
        dims_elem.tail = '\n'

        # Offset
        offset_elem = SubElement(tap_elem, 'offset')
        offset_elem.text = str(tap.offset)
        offset_elem.tail = '\n'

        # Sizes
        sizes_elem = SubElement(tap_elem, 'sizes')
        sizes_elem.text = ', '.join(str(s) for s in tap.sizes)
        sizes_elem.tail = '\n'

        # Strides
        strides_elem = SubElement(tap_elem, 'strides')
        strides_elem.text = ', '.join(str(s) for s in tap.strides)
        strides_elem.tail = '\n'

    def _add_gui_program(self, parent: Element, program: Program):
        """Add Program definition in GUI XML format."""
        prog_elem = SubElement(parent, 'Program')
        prog_elem.set('name', 'program')
        prog_elem.text = '\n'
        prog_elem.tail = '\n'

        # Device
        device_elem = SubElement(prog_elem, 'device')
        device_elem.text = 'current_device'
        device_elem.tail = '\n'

        # Runtime
        if program.runtime:
            rt_elem = SubElement(prog_elem, 'runtime')
            rt_elem.text = program.runtime.name
            rt_elem.tail = '\n'

        # Placer
        placer_elem = SubElement(prog_elem, 'placer')
        placer_elem.text = 'SequentialPlacer'
        placer_elem.tail = '\n'

    def _add_functions(self, parent: Element, program: Program):
        """Add JIT function wrapper."""
        func_elem = SubElement(parent, 'Function')
        func_elem.set('name', 'jit_function')
        func_elem.set('decorator', 'iron.jit')
        func_elem.set('entry', f'{program.name}_jit')
        func_elem.text = '\n'
        func_elem.tail = '\n'

        # Parameters
        params_elem = SubElement(func_elem, 'parameters')
        params_elem.text = '\n'
        params_elem.tail = '\n'

        # Add parameters based on runtime
        if program.runtime and program.runtime.param_names:
            for i, param_name in enumerate(program.runtime.param_names):
                param_elem = SubElement(params_elem, 'param')
                param_elem.set('name', param_name)

                # Determine type
                if i < len(program.runtime.input_types):
                    type_ref = str(program.runtime.input_types[i])
                elif i - len(program.runtime.input_types) < len(program.runtime.output_types):
                    type_ref = str(program.runtime.output_types[i - len(program.runtime.input_types)])
                else:
                    type_ref = 'vector_ty'

                param_elem.set('type', type_ref)
                param_elem.tail = '\n'

        # Body
        body_elem = SubElement(func_elem, 'body')
        body_elem.text = '\n'
        body_elem.tail = '\n'

        use_df_elem = SubElement(body_elem, 'UseDataFlow')
        use_df_elem.tail = '\n'

        ret_elem = SubElement(body_elem, 'Return')
        ret_elem.text = 'program'
        ret_elem.tail = '\n'

    def _add_entrypoint(self, parent: Element, program: Program):
        """Add main function and entry point."""
        # Main function
        main_func = SubElement(parent, 'Function')
        main_func.set('name', 'main_function')
        main_func.set('entry', 'main')
        main_func.text = '\n'
        main_func.tail = '\n'

        body_elem = SubElement(main_func, 'body')
        body_elem.text = '\n'
        body_elem.tail = '\n'

        # Add N assignment if we have it
        if 'N' in program.symbols:
            assign_elem = SubElement(body_elem, 'Assign')
            assign_elem.set('name', 'N')
            assign_elem.set('value', str(program.symbols['N'].value))
            assign_elem.tail = '\n'

        # Add tensor initializations
        if program.runtime and program.runtime.param_names:
            for i, param_name in enumerate(program.runtime.param_names):
                tensor_elem = SubElement(body_elem, 'Tensor')
                tensor_elem.set('name', param_name)
                tensor_elem.text = '\n'
                tensor_elem.tail = '\n'

                init_elem = SubElement(tensor_elem, 'init')
                # Determine if input or output
                if i < len(program.runtime.input_types):
                    init_elem.text = f'iron.arange(N, dtype=np.int32, device="npu")'
                else:
                    init_elem.text = f'iron.zeros(N, dtype=np.int32, device="npu")'
                init_elem.tail = '\n'

        # Call JIT function
        call_elem = SubElement(body_elem, 'Call')
        call_elem.set('function', 'jit_function')
        if program.runtime and program.runtime.param_names:
            call_elem.set('args', ', '.join(program.runtime.param_names))
        call_elem.tail = '\n'

        # Entry point
        entry_elem = SubElement(parent, 'EntryPoint')
        entry_elem.text = '\n'
        entry_elem.tail = '\n'

        if_elem = SubElement(entry_elem, 'If')
        if_elem.set('condition', '__name__ == \'__main__\'')
        if_elem.text = '\n'
        if_elem.tail = '\n'

        call_main = SubElement(if_elem, 'Call')
        call_main.set('function', 'main_function')
        call_main.tail = '\n'
