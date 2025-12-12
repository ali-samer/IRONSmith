# add_activatejit.py -*- Python -*-


import numpy as np
from ml_dtypes import bfloat16

from aie.iron import Program, Runtime, Worker, ObjectFifo
from aie.iron.placers import SequentialPlacer
from aie.iron.device.tile import AnyComputeTile
from aie.iron import ExternalFunction, jit
from aie.iron.dataflow import ObjectFifoLink
from aie.iron.device import Tile
from aie.iron.device import NPU1Col1, NPU2Col1, XCVC1902
import aie.iron as iron

from aie.helpers.taplib import TensorAccessPattern


@iron.jit(is_placed=False)
def base_aaa(inputA, inputB, outputD):
    # Define tensor types
    data_ty = np.ndarray[(inputA.numel(),), np.dtype[bfloat16]]
    chunk_ty = np.ndarray[(inputA.numel() // 4,), np.dtype[bfloat16]]
    worker_chunk_ty = np.ndarray[(inputA.numel() // 8,), np.dtype[bfloat16]]
    data_a_ty = np.ndarray[(inputA.numel(),), np.dtype[bfloat16]]
    chunk_a = np.ndarray[(inputA.numel() // 4,), np.dtype[bfloat16]]
    chunk_a_worker = np.ndarray[(inputA.numel() // 8,), np.dtype[bfloat16]]
    data_b_ty = np.ndarray[(inputB.numel(),), np.dtype[bfloat16]]
    chunk_b = np.ndarray[(inputB.numel() // 4,), np.dtype[bfloat16]]
    chunk_b_worker = np.ndarray[(inputB.numel() // 8,), np.dtype[bfloat16]]
    data_d_ty = np.ndarray[(outputD.numel(),), np.dtype[bfloat16]]
    chunk_d = np.ndarray[(outputD.numel() // 4,), np.dtype[bfloat16]]
    chunk_d_worker = np.ndarray[(outputD.numel() // 8,), np.dtype[bfloat16]]

    # Data movement with ObjectFifos
    SHIM_L3_L2_A1A2_col0 = ObjectFifo(obj_type=chunk_a, depth=2, name="SHIM_L3_L2_A1A2_col0")
    SHIM_L3_L2_A3A4_col1 = ObjectFifo(obj_type=chunk_a, depth=2, name="SHIM_L3_L2_A3A4_col1")
    SHIM_L3_L2_A5A6_col2 = ObjectFifo(obj_type=chunk_a, depth=2, name="SHIM_L3_L2_A5A6_col2")
    SHIM_L3_L2_A7A8_col3 = ObjectFifo(obj_type=chunk_a, depth=2, name="SHIM_L3_L2_A7A8_col3")
    SHIM_L3_L2_B1B2_col0 = ObjectFifo(obj_type=chunk_b, depth=2, name="SHIM_L3_L2_B1B2_col0")
    SHIM_L3_L2_B3B4_col1 = ObjectFifo(obj_type=chunk_b, depth=2, name="SHIM_L3_L2_B3B4_col1")
    SHIM_L3_L2_B5B6_col2 = ObjectFifo(obj_type=chunk_b, depth=2, name="SHIM_L3_L2_B5B6_col2")
    SHIM_L3_L2_B7B8_col3 = ObjectFifo(obj_type=chunk_b, depth=2, name="SHIM_L3_L2_B7B8_col3")
    MEM_L2_L1_A1A2_col0 = SHIM_L3_L2_A1A2_col0.cons().split(obj_types=[chunk_a_worker, chunk_a_worker], offsets=[((inputA.numel() // 8) * 0), ((inputA.numel() // 8) * 1)], names=["MEM_L2_L1_A1_col0", "MEM_L2_L1_A2_col0"], placement=Tile(0, 1))
    MEM_L2_L1_A3A4_col1 = SHIM_L3_L2_A3A4_col1.cons().split(obj_types=[chunk_a_worker, chunk_a_worker], offsets=[((inputA.numel() // 8) * 0), ((inputA.numel() // 8) * 1)], names=["MEM_L2_L1_A3_col1", "MEM_L2_L1_A4_col1"], placement=Tile(1, 1))
    MEM_L2_L1_A5A6_col2 = SHIM_L3_L2_A5A6_col2.cons().split(obj_types=[chunk_a_worker, chunk_a_worker], offsets=[((inputA.numel() // 8) * 0), ((inputA.numel() // 8) * 1)], names=["MEM_L2_L1_A5_col2", "MEM_L2_L1_A6_col2"], placement=Tile(2, 1))
    MEM_L2_L1_A7A8_col3 = SHIM_L3_L2_A7A8_col3.cons().split(obj_types=[chunk_a_worker, chunk_a_worker], offsets=[((inputA.numel() // 8) * 0), ((inputA.numel() // 8) * 1)], names=["MEM_L2_L1_A7_col3", "MEM_L2_L1_A8_col3"], placement=Tile(3, 1))
    MEM_L2_L1_B1B2_col0 = SHIM_L3_L2_B1B2_col0.cons().split(obj_types=[chunk_b_worker, chunk_b_worker], offsets=[((inputB.numel() // 8) * 0), ((inputB.numel() // 8) * 1)], names=["MEM_L2_L1_B1_col0", "MEM_L2_L1_B2_col0"], placement=Tile(0, 1))
    MEM_L2_L1_B3B4_col1 = SHIM_L3_L2_B3B4_col1.cons().split(obj_types=[chunk_b_worker, chunk_b_worker], offsets=[((inputB.numel() // 8) * 0), ((inputB.numel() // 8) * 1)], names=["MEM_L2_L1_B3_col1", "MEM_L2_L1_B4_col1"], placement=Tile(1, 1))
    MEM_L2_L1_B5B6_col2 = SHIM_L3_L2_B5B6_col2.cons().split(obj_types=[chunk_b_worker, chunk_b_worker], offsets=[((inputB.numel() // 8) * 0), ((inputB.numel() // 8) * 1)], names=["MEM_L2_L1_B5_col2", "MEM_L2_L1_B6_col2"], placement=Tile(2, 1))
    MEM_L2_L1_B7B8_col3 = SHIM_L3_L2_B7B8_col3.cons().split(obj_types=[chunk_b_worker, chunk_b_worker], offsets=[((inputB.numel() // 8) * 0), ((inputB.numel() // 8) * 1)], names=["MEM_L2_L1_B7_col3", "MEM_L2_L1_B8_col3"], placement=Tile(3, 1))
    L1_L1_add_to_relu_1 = ObjectFifo(obj_type=worker_chunk_ty, depth=2, name="L1_L1_add_to_relu_1")
    L1_L1_add_to_relu_2 = ObjectFifo(obj_type=worker_chunk_ty, depth=2, name="L1_L1_add_to_relu_2")
    L1_L1_add_to_relu_3 = ObjectFifo(obj_type=worker_chunk_ty, depth=2, name="L1_L1_add_to_relu_3")
    L1_L1_add_to_relu_4 = ObjectFifo(obj_type=worker_chunk_ty, depth=2, name="L1_L1_add_to_relu_4")
    L1_L1_add_to_relu_5 = ObjectFifo(obj_type=worker_chunk_ty, depth=2, name="L1_L1_add_to_relu_5")
    L1_L1_add_to_relu_6 = ObjectFifo(obj_type=worker_chunk_ty, depth=2, name="L1_L1_add_to_relu_6")
    L1_L1_add_to_relu_7 = ObjectFifo(obj_type=worker_chunk_ty, depth=2, name="L1_L1_add_to_relu_7")
    L1_L1_add_to_relu_8 = ObjectFifo(obj_type=worker_chunk_ty, depth=2, name="L1_L1_add_to_relu_8")
    SHIM_L2_L3_D1D2_col0 = ObjectFifo(obj_type=chunk_d, depth=2, name="SHIM_L2_L3_D1D2_col0")
    SHIM_L2_L3_D3D4_col1 = ObjectFifo(obj_type=chunk_d, depth=2, name="SHIM_L2_L3_D3D4_col1")
    SHIM_L2_L3_D5D6_col2 = ObjectFifo(obj_type=chunk_d, depth=2, name="SHIM_L2_L3_D5D6_col2")
    SHIM_L2_L3_D7D8_col3 = ObjectFifo(obj_type=chunk_d, depth=2, name="SHIM_L2_L3_D7D8_col3")
    MEM_L1_L2_D1D2_col0 = SHIM_L2_L3_D1D2_col0.prod().join(obj_types=[chunk_d_worker, chunk_d_worker], names=["MEM_L1_L2_D1_col0", "MEM_L1_L2_D2_col0"], placement=Tile(0, 1), offsets=[((outputD.numel() // 8) * 0), ((outputD.numel() // 8) * 1)])
    MEM_L1_L2_D3D4_col1 = SHIM_L2_L3_D3D4_col1.prod().join(obj_types=[chunk_d_worker, chunk_d_worker], names=["MEM_L1_L2_D3_col1", "MEM_L1_L2_D4_col1"], placement=Tile(1, 1), offsets=[((outputD.numel() // 8) * 0), ((outputD.numel() // 8) * 1)])
    MEM_L1_L2_D5D6_col2 = SHIM_L2_L3_D5D6_col2.prod().join(obj_types=[chunk_d_worker, chunk_d_worker], names=["MEM_L1_L2_D5_col2", "MEM_L1_L2_D6_col2"], placement=Tile(2, 1), offsets=[((outputD.numel() // 8) * 0), ((outputD.numel() // 8) * 1)])
    MEM_L1_L2_D7D8_col3 = SHIM_L2_L3_D7D8_col3.prod().join(obj_types=[chunk_d_worker, chunk_d_worker], names=["MEM_L1_L2_D7_col3", "MEM_L1_L2_D8_col3"], placement=Tile(3, 1), offsets=[((outputD.numel() // 8) * 0), ((outputD.numel() // 8) * 1)])

    #Define kernels here... ------------------------------------------------\/
    externalfunc1 = ExternalFunction(
        name="eltwise_add_bf16_scalar", source_file="../../../aie_kernels/aie2/add.cc", arg_types=[worker_chunk_ty, worker_chunk_ty, worker_chunk_ty], include_dirs=["/scratch/andrewa/mlir-aie/aie_kernels/"]
    )

    externalfunc2 = ExternalFunction(
        name="bf16_relu", source_file="../../../aie_kernels/aie2/relu.cc", arg_types=[worker_chunk_ty, worker_chunk_ty], include_dirs=["/scratch/andrewa/mlir-aie/aie_kernels/"]
    )

    # core_fn here:
    def corefunc1(kernel, inputA, inputB, outputC):
            elementA = inputA.acquire(1)
            elementB = inputB.acquire(1)
            elementC = outputC.acquire(1)
            kernel(elementA, elementB, elementC)
            inputA.release(1)
            inputB.release(1)
            outputC.release(1)

    def corefunc2(kernel, inputC, outputD):
            elementC = inputC.acquire(1)
            elementD = outputD.acquire(1)
            kernel(elementC, elementD)
            inputC.release(1)
            outputD.release(1)

    #Workers defined here:
    Workers = []
    worker_add_col0_w0 = Worker(core_fn=corefunc1, fn_args=[externalfunc1, MEM_L2_L1_A1A2_col0[0].cons(), MEM_L2_L1_B1B2_col0[0].cons(), L1_L1_add_to_relu_1.prod()], placement=Tile(0, 5))
    worker_add_col0_w1 = Worker(core_fn=corefunc1, fn_args=[externalfunc1, MEM_L2_L1_A1A2_col0[1].cons(), MEM_L2_L1_B1B2_col0[1].cons(), L1_L1_add_to_relu_2.prod()], placement=Tile(0, 3))
    worker_add_col1_w0 = Worker(core_fn=corefunc1, fn_args=[externalfunc1, MEM_L2_L1_A3A4_col1[0].cons(), MEM_L2_L1_B3B4_col1[0].cons(), L1_L1_add_to_relu_3.prod()], placement=Tile(1, 5))
    worker_add_col1_w1 = Worker(core_fn=corefunc1, fn_args=[externalfunc1, MEM_L2_L1_A3A4_col1[1].cons(), MEM_L2_L1_B3B4_col1[1].cons(), L1_L1_add_to_relu_4.prod()], placement=Tile(1, 3))
    worker_add_col2_w0 = Worker(core_fn=corefunc1, fn_args=[externalfunc1, MEM_L2_L1_A5A6_col2[0].cons(), MEM_L2_L1_B5B6_col2[0].cons(), L1_L1_add_to_relu_5.prod()], placement=Tile(2, 5))
    worker_add_col2_w1 = Worker(core_fn=corefunc1, fn_args=[externalfunc1, MEM_L2_L1_A5A6_col2[1].cons(), MEM_L2_L1_B5B6_col2[1].cons(), L1_L1_add_to_relu_6.prod()], placement=Tile(2, 3))
    worker_add_col3_w0 = Worker(core_fn=corefunc1, fn_args=[externalfunc1, MEM_L2_L1_A7A8_col3[0].cons(), MEM_L2_L1_B7B8_col3[0].cons(), L1_L1_add_to_relu_7.prod()], placement=Tile(3, 5))
    worker_add_col3_w1 = Worker(core_fn=corefunc1, fn_args=[externalfunc1, MEM_L2_L1_A7A8_col3[1].cons(), MEM_L2_L1_B7B8_col3[1].cons(), L1_L1_add_to_relu_8.prod()], placement=Tile(3, 3))
    worker_relu_col0_w0 = Worker(core_fn=corefunc2, fn_args=[externalfunc2, L1_L1_add_to_relu_1.cons(), MEM_L1_L2_D1D2_col0[0].prod()], placement=Tile(0, 4))
    worker_relu_col0_w1 = Worker(core_fn=corefunc2, fn_args=[externalfunc2, L1_L1_add_to_relu_2.cons(), MEM_L1_L2_D1D2_col0[1].prod()], placement=Tile(0, 2))
    worker_relu_col1_w0 = Worker(core_fn=corefunc2, fn_args=[externalfunc2, L1_L1_add_to_relu_3.cons(), MEM_L1_L2_D3D4_col1[0].prod()], placement=Tile(1, 4))
    worker_relu_col1_w1 = Worker(core_fn=corefunc2, fn_args=[externalfunc2, L1_L1_add_to_relu_4.cons(), MEM_L1_L2_D3D4_col1[1].prod()], placement=Tile(1, 2))
    worker_relu_col2_w0 = Worker(core_fn=corefunc2, fn_args=[externalfunc2, L1_L1_add_to_relu_5.cons(), MEM_L1_L2_D5D6_col2[0].prod()], placement=Tile(2, 4))
    worker_relu_col2_w1 = Worker(core_fn=corefunc2, fn_args=[externalfunc2, L1_L1_add_to_relu_6.cons(), MEM_L1_L2_D5D6_col2[1].prod()], placement=Tile(2, 2))
    worker_relu_col3_w0 = Worker(core_fn=corefunc2, fn_args=[externalfunc2, L1_L1_add_to_relu_7.cons(), MEM_L1_L2_D7D8_col3[0].prod()], placement=Tile(3, 4))
    worker_relu_col3_w1 = Worker(core_fn=corefunc2, fn_args=[externalfunc2, L1_L1_add_to_relu_8.cons(), MEM_L1_L2_D7D8_col3[1].prod()], placement=Tile(3, 2))

    Workers = [worker_add_col0_w0, worker_add_col0_w1, worker_add_col1_w0, worker_add_col1_w1, worker_add_col2_w0, worker_add_col2_w1, worker_add_col3_w0, worker_add_col3_w1, worker_relu_col0_w0, worker_relu_col0_w1, worker_relu_col1_w0, worker_relu_col1_w1, worker_relu_col2_w0, worker_relu_col2_w1, worker_relu_col3_w0, worker_relu_col3_w1]

    # Runtime operations to move data to/from the AIE-array
    rt = Runtime()
    with rt.sequence(chunk_ty, chunk_ty, chunk_ty) as (A, B, D):
        rt.start(*Workers)
        rt.fill(placement=Tile(0, 0), in_fifo=SHIM_L3_L2_A1A2_col0.prod(), source=A, tap=TensorAccessPattern(tensor_dims=[inputA.numel()], offset=((inputA.numel() // 4) * 0), sizes=[((inputA.numel() // 4) // (inputA.numel() // 8)), (inputA.numel() // 8)], strides=[(inputA.numel() // 8), 1]))
        rt.fill(placement=Tile(1, 0), in_fifo=SHIM_L3_L2_A3A4_col1.prod(), source=A, tap=TensorAccessPattern(tensor_dims=[inputA.numel()], offset=((inputA.numel() // 4) * 1), sizes=[((inputA.numel() // 4) // (inputA.numel() // 8)), (inputA.numel() // 8)], strides=[(inputA.numel() // 8), 1]))
        rt.fill(placement=Tile(2, 0), in_fifo=SHIM_L3_L2_A5A6_col2.prod(), source=A, tap=TensorAccessPattern(tensor_dims=[inputA.numel()], offset=((inputA.numel() // 4) * 2), sizes=[((inputA.numel() // 4) // (inputA.numel() // 8)), (inputA.numel() // 8)], strides=[(inputA.numel() // 8), 1]))
        rt.fill(placement=Tile(3, 0), in_fifo=SHIM_L3_L2_A7A8_col3.prod(), source=A, tap=TensorAccessPattern(tensor_dims=[inputA.numel()], offset=((inputA.numel() // 4) * 3), sizes=[((inputA.numel() // 4) // (inputA.numel() // 8)), (inputA.numel() // 8)], strides=[(inputA.numel() // 8), 1]))
        rt.fill(placement=Tile(0, 0), in_fifo=SHIM_L3_L2_B1B2_col0.prod(), source=B, tap=TensorAccessPattern(tensor_dims=[inputB.numel()], offset=((inputB.numel() // 4) * 0), sizes=[((inputB.numel() // 4) // (inputB.numel() // 8)), (inputB.numel() // 8)], strides=[(inputB.numel() // 8), 1]))
        rt.fill(placement=Tile(1, 0), in_fifo=SHIM_L3_L2_B3B4_col1.prod(), source=B, tap=TensorAccessPattern(tensor_dims=[inputB.numel()], offset=((inputB.numel() // 4) * 1), sizes=[((inputB.numel() // 4) // (inputB.numel() // 8)), (inputB.numel() // 8)], strides=[(inputB.numel() // 8), 1]))
        rt.fill(placement=Tile(2, 0), in_fifo=SHIM_L3_L2_B5B6_col2.prod(), source=B, tap=TensorAccessPattern(tensor_dims=[inputB.numel()], offset=((inputB.numel() // 4) * 2), sizes=[((inputB.numel() // 4) // (inputB.numel() // 8)), (inputB.numel() // 8)], strides=[(inputB.numel() // 8), 1]))
        rt.fill(placement=Tile(3, 0), in_fifo=SHIM_L3_L2_B7B8_col3.prod(), source=B, tap=TensorAccessPattern(tensor_dims=[inputB.numel()], offset=((inputB.numel() // 4) * 3), sizes=[((inputB.numel() // 4) // (inputB.numel() // 8)), (inputB.numel() // 8)], strides=[(inputB.numel() // 8), 1]))
        rt.drain(placement=Tile(0, 0), out_fifo=SHIM_L2_L3_D1D2_col0.cons(), dest=D, wait=True, tap=TensorAccessPattern(tensor_dims=[outputD.numel()], offset=((outputD.numel() // 4) * 0), sizes=[((outputD.numel() // 4) // (outputD.numel() // 8)), (outputD.numel() // 8)], strides=[(outputD.numel() // 8), 1]))
        rt.drain(placement=Tile(1, 0), out_fifo=SHIM_L2_L3_D3D4_col1.cons(), dest=D, wait=True, tap=TensorAccessPattern(tensor_dims=[outputD.numel()], offset=((outputD.numel() // 4) * 1), sizes=[((outputD.numel() // 4) // (outputD.numel() // 8)), (outputD.numel() // 8)], strides=[(outputD.numel() // 8), 1]))
        rt.drain(placement=Tile(2, 0), out_fifo=SHIM_L2_L3_D5D6_col2.cons(), dest=D, wait=True, tap=TensorAccessPattern(tensor_dims=[outputD.numel()], offset=((outputD.numel() // 4) * 2), sizes=[((outputD.numel() // 4) // (outputD.numel() // 8)), (outputD.numel() // 8)], strides=[(outputD.numel() // 8), 1]))
        rt.drain(placement=Tile(3, 0), out_fifo=SHIM_L2_L3_D7D8_col3.cons(), dest=D, wait=True, tap=TensorAccessPattern(tensor_dims=[outputD.numel()], offset=((outputD.numel() // 4) * 3), sizes=[((outputD.numel() // 4) // (outputD.numel() // 8)), (outputD.numel() // 8)], strides=[(outputD.numel() // 8), 1]))

    # Create the program from the current device and runtime
    my_program = Program(iron.get_current_device(), rt)

    # Place components and resolve program (generate MLIR + compile)
    return my_program.resolve_program(SequentialPlacer())


def main():
    datatype = bfloat16
    data_size = 128
    inputA = iron.arange(data_size, dtype=datatype, device="npu")
    inputB = iron.arange(data_size, dtype=datatype, device="npu")
    outputD = iron.zeros(data_size, dtype=datatype, device="npu")
    base_aaa(inputA, inputB, outputD)



if __name__ == "__main__":
    main()