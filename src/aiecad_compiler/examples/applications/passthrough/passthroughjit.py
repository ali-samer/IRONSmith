# passthroughjit.py -*- Python -*-


import sys
import numpy as np

from aie.iron import Program, Runtime, ObjectFifo
from aie.iron.placers import SequentialPlacer
from aie.iron.device import NPU1Col1, NPU2Col1, XCVC1902
import aie.iron as iron


@iron.jit(is_placed=False)
def passthrough_dmas_jit(input_tensor, output_tensor):
    N = input_tensor.numel()
    line_size = 1024
    assert N % line_size == 0, "N must be multiple of line_size"

    # Define tensor types
    vector_ty = np.ndarray[(N,), np.dtype[np.int32]]
    line_ty = np.ndarray[(line_size,), np.dtype[np.int32]]

    # Data movement with ObjectFifos
    of_in = ObjectFifo(line_ty, name="in")
    of_out = of_in.cons().forward()

    # Runtime operations to move data to/from the AIE-array
    rt = Runtime()
    with rt.sequence(vector_ty, vector_ty) as (a_in, c_out):
        rt.fill(of_in.prod(), a_in)
        rt.drain(of_out.cons(), c_out, wait=True)

    # Create the program from the current device and runtime
    my_program = Program(iron.get_current_device(), rt)

    # Place components and resolve program (generate MLIR + compile)
    placer = SequentialPlacer()
    return my_program.resolve_program(placer)


def main():
    # Default values
    N = 4096
    line_size = 1024

    # Parse arguments
    if len(sys.argv) > 1:
        N = int(sys.argv[1])
        assert N % line_size == 0, "N must be multiple of line_size"

    if len(sys.argv) > 2:
        device_name = sys.argv[2]
        if device_name == "npu":
            iron.set_current_device(NPU1Col1())
        elif device_name == "npu2":
            iron.set_current_device(NPU2Col1())
        elif device_name == "xcvc1902":
            iron.set_current_device(XCVC1902())
        else:
            raise ValueError(f"[ERROR] Device name {device_name} is unknown")
    else:
        # Default to NPU1Col1 if not specified
        iron.set_current_device(NPU1Col1())

    # Create input and output tensors on the device
    input_tensor = iron.arange(N, dtype=np.int32, device="npu")
    output_tensor = iron.zeros(N, dtype=np.int32, device="npu")

    # JIT-compile and execute
    passthrough_dmas_jit(input_tensor, output_tensor)

    # Bring output back to host for verification
    output_host = output_tensor.numpy()
    input_host = input_tensor.numpy()

    # Verify: output should equal input (passthrough)
    errors = np.count_nonzero(input_host != output_host)

    # Print results
    print(f"{'Index':>6} {'Input':>8} {'Output':>8}")
    print("-" * 24)
    for i in range(min(N, 4096)):  # Print first 32 elements
        print(f"{i:6}: {input_host[i]:8} {output_host[i]:8}")

    if errors == 0:
        print(f"\nPASS! All {N} elements match.\n")
        sys.exit(0)
    else:
        print(f"\nFAIL! {errors} mismatches out of {N} elements.\n")
        sys.exit(1)


if __name__ == "__main__":
    main()