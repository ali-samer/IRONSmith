# vector_vector_add/vector_vector_add.py -*- Python -*-
#
# This file is licensed under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
# (c) Copyright 2024-2025 Advanced Micro Devices, Inc. or its affiliates

import argparse
import sys
import time
import numpy as np
import aie.iron as iron

from aie.iron import ObjectFifo, Program, Runtime, Worker, ExternalFunction
from aie.iron.placers import SequentialPlacer
from aie.iron.device import NPU1Col1, NPU2Col1
from aie.iron.controlflow import range_

lane_sz_table = {
    np.dtype(np.int32): 16,
    np.dtype(np.int16): 32,
    np.dtype(np.int8):  64
}
lane_sz = lambda x : lane_sz_table[np.dtype(x)]

valid_dtypes = ['int8', 'int16', 'int32']

size=10000
base=1024
buffer_size = 512

min_loop_iter = base // lane_sz(np.int8)

DEFAULT_DTYPE = np.int32
TRACE_BYTES = base* 8

@iron.jit(is_placed=False)
def vector_vector_add(input0, input1, output):
    if input0.shape != input1.shape:
        raise ValueError(
            f"Input shapes are not the equal ({input0.shape} != {input1.shape})."
        )
    if input0.shape != output.shape:
        raise ValueError(
            f"Input and output shapes are not the equal ({input0.shape} != {output.shape})."
        )

    if input0.dtype != input1.dtype:
        raise ValueError(
            f"Input data types are not the same ({input0.dtype} != {input1.dtype})."
        )
    if input0.dtype != output.dtype:
        raise ValueError(
            f"Input and output data types are not the same ({input0.dtype} != {output.dtype})."
        )
    dtype = input0.dtype

    if len(np.shape(input0)) != 1:
        raise ValueError("Function only supports vectors.")
    num_elements = np.size(input0)

    n = base

    if num_elements % n != 0:
        raise ValueError(
            f"Number of elements ({num_elements}) must be a multiple of {n}."
        )

    # Define tensor types
    tensor_ty = np.ndarray[(num_elements,), np.dtype[dtype]]
    tile_ty = np.ndarray[(base,), np.dtype[dtype]]

    # External Kernel
    '''
    the target flag informs the compiler about the architecture
    we want the cpp program to be compiled for.
    aie2: compile for the aie second gen.
    none: no specific vendor
    elf: produce an ELF binary
    '''
    target = "--target=aie2-none-unknown-elf"
    compile_flags =[target, "-O2", "-Wno-attributes", "-fno-exceptions", "-fno-rtti",
                    f"-DLANE_SIZE={lane_sz(dtype)}", f"-DDTYPE={np.dtype(dtype).name}_t",
                    f"-DMIN_LOOP_ITERATIONS={min_loop_iter}"]

    vec_add_kernel = ExternalFunction (
        name="vec_add_vectorized_6",
        source_file="kernels/vec_add_scalar.cc",
        arg_types=[tile_ty, tile_ty, tile_ty, np.int32],
        compile_flags=compile_flags,
    )

    print("Kernel name:", vec_add_kernel._name)
    print("Source file:", vec_add_kernel._source_file)
    print("Object file:", vec_add_kernel._object_file_name)
    print("Arg types  :", vec_add_kernel.arg_types)
    # print("Include dirs:", getattr(vec_add_kernel, "include_dirs", None))
    print("Compile flags:", compile_flags)

    # AIE-array data movement with object fifos
    of_in1 = ObjectFifo(tile_ty, name="in1")
    of_in2 = ObjectFifo(tile_ty, name="in2")
    of_out = ObjectFifo(tile_ty, name="out")

    # Define a task that will run on a compute tile
    def core_body(of_in1, of_in2, of_out, vec_add_kernel):
        # Number of sub-vector "tile" iterations
        for _ in range_(size):
            elem_in1 = of_in1.acquire(1)
            elem_in2 = of_in2.acquire(1)
            elem_out = of_out.acquire(1)
            vec_add_kernel(elem_in1, elem_in2, elem_out, n)
            of_in1.release(1)
            of_in2.release(1)
            of_out.release(1)

    # Create a worker to run the task on a compute tile
    worker = Worker(core_body,
                    fn_args=[of_in1.cons(), of_in2.cons(), of_out.prod(), vec_add_kernel],
                    trace=True)

    # Runtime operations to move data to/from the AIE-array
    rt = Runtime()
    with rt.sequence(tensor_ty, tensor_ty, tensor_ty) as (A, B, C):
        # rt.enable_trace(TRACE_BYTES)
        rt.start(worker)
        rt.fill(of_in1.prod(), A)
        rt.fill(of_in2.prod(), B)
        rt.drain(of_out.cons(), C, wait=True)

    # Place program components (assign them resources on the device) and generate an MLIR module
    return Program(iron.get_current_device(), rt).resolve_program(SequentialPlacer())

def main():
    device_map = {
        "npu": NPU1Col1(),
        "npu2": NPU2Col1(),
    }

    parser = argparse.ArgumentParser()
    parser.add_argument(
        "-v", "--verbose", action="store_true", help="Enable verbose output"
    )
    parser.add_argument(
        "-d",
        "--device",
        choices=["npu", "npu2"],
        default="npu",
        help="Target device",
    )
    parser.add_argument(
        "-n",
        "--num-elements",
        type=int,
        default=base*size,
        help="Number of elements (default: 32)",
    )
    parser.add_argument(
        "-t",
        "--device-type",
        choices=valid_dtypes,
        type=str,
        default=(DEFAULT_DTYPE(0).dtype.name),
        help="Type of buffer (default: int32)"
    )
    args = parser.parse_args()

    np_dtype = np.dtype(args.device_type)
    # NOTE1: infer mapping from main memory to npu

    # Construct two input random tensors and an output zeroed tensor
    # The three tensor are in memory accessible to the NPU
    input0 = iron.randint(0, 100, (args.num_elements,), dtype=np_dtype, device="npu")
    input1 = iron.randint(0, 100, (args.num_elements,), dtype=np_dtype, device="npu")
    output = iron.zeros_like(input0)

    iron.set_current_device(device_map[args.device])

    # JIT-compile the kernel then launches the kernel with the given arguments. Future calls
    # to the kernel will use the same compiled kernel and loaded code objects
    start = time.perf_counter()
    vector_vector_add(input0, input1, output)
    end = time.perf_counter()

    print(f"Number of elements: {args.num_elements}")
    print(f"Device Type: {np_dtype}")
    print(f"Elapsed: {end - start:.6f} seconds")

    # Check the correctness of the result
    e = np.equal(input0.numpy() + input1.numpy(), output.numpy())
    errors = np.size(e) - np.count_nonzero(e)

    # Optionally, print the results
    if args.verbose:
        print(f"{'input0':>4} + {'input1':>4} = {'output':>4}")
        print("-" * 34)
        count = input0.numel()
        for idx, (a, b, c) in enumerate(
                zip(input0[:count], input1[:count], output[:count])
        ):
            print(f"{idx:2}: {a:4} + {b:4} = {c:4}")

    # If the result is correct, exit with a success code.
    # Otherwise, exit with a failure code
    if not errors:
        print("\nPASS!\n")
        sys.exit(0)
    else:
        print("\nError count: ", errors)
        print("\nFailed.\n")
        sys.exit(-1)


if __name__ == "__main__":
    main()
