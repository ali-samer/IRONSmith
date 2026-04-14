#
# This file is licensed under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
# (c) Copyright 2025 Advanced Micro Devices, Inc. or its affiliates

# mlp.py – 4-layer Linear MLP on 16 AIE compute tiles
#
# ┌─────────────────────────────────────────────────────────────────────────────┐
# │  Network:  4 fully-connected linear layers, each 64→64 (int16)              │
# │  Device:   NPU1  (4 columns × 4 compute rows = 16 compute tiles)            │
# │  Pipeline: each column processes one layer; all four run in parallel on      │
# │            successive batches (wave-pipelined).                              │
# └─────────────────────────────────────────────────────────────────────────────┘
#
# Within each column (one MLP layer):
#   Input activation (M=64, K=64) – broadcast to all 4 tiles in the column
#   Weight slice     (K=64, n=16) – unique per tile (1/4 of the weight matrix)
#   Output activation (M=64,N=64) – joined from 4 tile outputs in the MemTile
#
# Inter-layer data flow (columns connected left-to-right):
#   host → col0 → col1 → col2 → col3 → host

import os
import numpy as np

from aie.iron import Kernel, ObjectFifo, Program, Runtime, Worker
from aie.iron.placers import SequentialPlacer
from aie.iron.device import Tile
import aie.iron as iron

from aie.helpers.taplib import TensorAccessPattern


@iron.jit(is_placed=False)
def mlp_jit(X, W0, W1, W2, W3, Y):
    # Constants
    batch_size      = 64   # M
    input_features  = 64   # K
    output_features = 64   # N
    features_per_tile = 16  # output features per tile (output_features // 4 rows)

    # i16 micro-kernel tile dimensions: aie::mmul<r=4, s=4, t=4>
    micro_r, micro_s, micro_t = 4, 4, 4

    # 4D DMA access-pattern descriptors for MemTile layout reformatting
    activation_layout_dims = [
        (batch_size // micro_r,    micro_r * input_features),
        (input_features // micro_s, micro_s),
        (micro_r,                  input_features),
        (micro_s,                  1),
    ]
    weight_layout_dims = [
        (input_features // micro_s,      micro_s * features_per_tile),
        (features_per_tile // micro_t,   micro_t),
        (micro_s,                        features_per_tile),
        (micro_t,                        1),
    ]
    output_layout_dims = [
        (batch_size // micro_r,          micro_r * features_per_tile),
        (micro_r,                        micro_t),
        (features_per_tile // micro_t,   micro_r * micro_t),
        (micro_t,                        1),
    ]

    # Tensor Types
    activation_flat_type  = np.ndarray[(batch_size * input_features,),    np.dtype[np.int16]]
    weight_flat_type      = np.ndarray[(input_features * output_features,), np.dtype[np.int16]]
    activation_full_type  = np.ndarray[(batch_size, input_features),       np.dtype[np.int16]]
    activation_tile_type  = np.ndarray[(batch_size, features_per_tile),    np.dtype[np.int16]]
    weight_tile_type      = np.ndarray[(input_features, features_per_tile), np.dtype[np.int16]]

    # Compute Kernels
    build_dir  = os.path.join(os.path.dirname(os.path.abspath(__file__)), "build")
    mm_archive = os.path.join(build_dir, "mm.a")
    zero_kernel   = Kernel("zero_i16",       mm_archive, [activation_tile_type])
    matmul_kernel = Kernel("matmul_i16_i16", mm_archive, [activation_full_type, weight_tile_type, activation_tile_type])
    relu_kernel   = Kernel("relu_i16",       mm_archive, [activation_tile_type])

    # Data Movement
    # Object Fifos

    # Layer 0 input: host → shim(0,0) → MemTile(0,1) [activation_layout_dims applied] → col0 tiles
    input_activation_fifo      = ObjectFifo(activation_full_type, depth=2, name="input_activation_fifo")
    input_activation_col0_fifo = input_activation_fifo.cons().forward(
        name="input_activation_col0_fifo", dims_to_stream=activation_layout_dims, placement=Tile(0, 1)
    )

    # Inter-layer activation FIFOs (join target for col c, broadcast input for col c+1)
    input_activation_col1_fifo = ObjectFifo(activation_full_type, depth=2, name="input_activation_col1_fifo")
    input_activation_col2_fifo = ObjectFifo(activation_full_type, depth=2, name="input_activation_col2_fifo")
    input_activation_col3_fifo = ObjectFifo(activation_full_type, depth=2, name="input_activation_col3_fifo")

    # Final output: col3 join → host (output_layout_dims converts tiled → row-major)
    output_activation_fifo = ObjectFifo(activation_full_type, depth=2, name="output_activation_fifo", dims_to_stream=output_layout_dims)

    # Weight FIFOs: host → shim(c,0) → MemTile(c,1) → split into 4 per-tile slices
    weights_col0_fifo = ObjectFifo(weight_flat_type, depth=1, name="weights_col0_fifo")
    weights_col1_fifo = ObjectFifo(weight_flat_type, depth=1, name="weights_col1_fifo")
    weights_col2_fifo = ObjectFifo(weight_flat_type, depth=1, name="weights_col2_fifo")
    weights_col3_fifo = ObjectFifo(weight_flat_type, depth=1, name="weights_col3_fifo")

    # Splits
    weight_tile_split_col0 = weights_col0_fifo.cons().split(
        offsets=[0, input_features * features_per_tile, 2 * input_features * features_per_tile, 3 * input_features * features_per_tile],
        obj_types=[weight_tile_type, weight_tile_type, weight_tile_type, weight_tile_type],
        names=["weight_tile_col0_row0", "weight_tile_col0_row1", "weight_tile_col0_row2", "weight_tile_col0_row3"],
        dims_to_stream=[weight_layout_dims, weight_layout_dims, weight_layout_dims, weight_layout_dims],
        placement=Tile(0, 1),
    )
    weight_tile_split_col1 = weights_col1_fifo.cons().split(
        offsets=[0, input_features * features_per_tile, 2 * input_features * features_per_tile, 3 * input_features * features_per_tile],
        obj_types=[weight_tile_type, weight_tile_type, weight_tile_type, weight_tile_type],
        names=["weight_tile_col1_row0", "weight_tile_col1_row1", "weight_tile_col1_row2", "weight_tile_col1_row3"],
        dims_to_stream=[weight_layout_dims, weight_layout_dims, weight_layout_dims, weight_layout_dims],
        placement=Tile(1, 1),
    )
    weight_tile_split_col2 = weights_col2_fifo.cons().split(
        offsets=[0, input_features * features_per_tile, 2 * input_features * features_per_tile, 3 * input_features * features_per_tile],
        obj_types=[weight_tile_type, weight_tile_type, weight_tile_type, weight_tile_type],
        names=["weight_tile_col2_row0", "weight_tile_col2_row1", "weight_tile_col2_row2", "weight_tile_col2_row3"],
        dims_to_stream=[weight_layout_dims, weight_layout_dims, weight_layout_dims, weight_layout_dims],
        placement=Tile(2, 1),
    )
    weight_tile_split_col3 = weights_col3_fifo.cons().split(
        offsets=[0, input_features * features_per_tile, 2 * input_features * features_per_tile, 3 * input_features * features_per_tile],
        obj_types=[weight_tile_type, weight_tile_type, weight_tile_type, weight_tile_type],
        names=["weight_tile_col3_row0", "weight_tile_col3_row1", "weight_tile_col3_row2", "weight_tile_col3_row3"],
        dims_to_stream=[weight_layout_dims, weight_layout_dims, weight_layout_dims, weight_layout_dims],
        placement=Tile(3, 1),
    )

    # Joins
    activation_tile_join_col0 = input_activation_col1_fifo.prod().join(
        offsets=[0, batch_size * features_per_tile, 2 * batch_size * features_per_tile, 3 * batch_size * features_per_tile],
        obj_types=[activation_tile_type, activation_tile_type, activation_tile_type, activation_tile_type],
        names=["activation_tile_col0_row0", "activation_tile_col0_row1", "activation_tile_col0_row2", "activation_tile_col0_row3"],
        placement=Tile(0, 1),
    )
    activation_tile_join_col1 = input_activation_col2_fifo.prod().join(
        offsets=[0, batch_size * features_per_tile, 2 * batch_size * features_per_tile, 3 * batch_size * features_per_tile],
        obj_types=[activation_tile_type, activation_tile_type, activation_tile_type, activation_tile_type],
        names=["activation_tile_col1_row0", "activation_tile_col1_row1", "activation_tile_col1_row2", "activation_tile_col1_row3"],
        placement=Tile(1, 1),
    )
    activation_tile_join_col2 = input_activation_col3_fifo.prod().join(
        offsets=[0, batch_size * features_per_tile, 2 * batch_size * features_per_tile, 3 * batch_size * features_per_tile],
        obj_types=[activation_tile_type, activation_tile_type, activation_tile_type, activation_tile_type],
        names=["activation_tile_col2_row0", "activation_tile_col2_row1", "activation_tile_col2_row2", "activation_tile_col2_row3"],
        placement=Tile(2, 1),
    )
    activation_tile_join_col3 = output_activation_fifo.prod().join(
        offsets=[0, batch_size * features_per_tile, 2 * batch_size * features_per_tile, 3 * batch_size * features_per_tile],
        obj_types=[activation_tile_type, activation_tile_type, activation_tile_type, activation_tile_type],
        names=["activation_tile_col3_row0", "activation_tile_col3_row1", "activation_tile_col3_row2", "activation_tile_col3_row3"],
        placement=Tile(3, 1),
    )

    # Core Body Functions
    def core_fn_with_relu(zero, matmul, relu, activation_in, weights, activation_out):
        """Intermediate layers: zero → matmul → relu in-place."""
        activation_buffer_in  = activation_in.acquire(1)
        weight_buffer         = weights.acquire(1)
        activation_buffer_out = activation_out.acquire(1)
        zero(activation_buffer_out)
        matmul(activation_buffer_in, weight_buffer, activation_buffer_out)
        relu(activation_buffer_out)
        activation_in.release(1)
        weights.release(1)
        activation_out.release(1)

    def core_fn_final(zero, matmul, activation_in, weights, activation_out):
        """Final layer: zero → matmul, no relu."""
        activation_buffer_in  = activation_in.acquire(1)
        weight_buffer         = weights.acquire(1)
        activation_buffer_out = activation_out.acquire(1)
        zero(activation_buffer_out)
        matmul(activation_buffer_in, weight_buffer, activation_buffer_out)
        activation_in.release(1)
        weights.release(1)
        activation_out.release(1)

    # Workers
    # Layers 0–2: zero → matmul → relu
    worker_col0_row2 = Worker(core_fn=core_fn_with_relu, fn_args=[zero_kernel, matmul_kernel, relu_kernel, input_activation_col0_fifo.cons(), weight_tile_split_col0[0].cons(), activation_tile_join_col0[0].prod()], placement=Tile(0, 2), stack_size=0x2000)
    worker_col0_row3 = Worker(core_fn=core_fn_with_relu, fn_args=[zero_kernel, matmul_kernel, relu_kernel, input_activation_col0_fifo.cons(), weight_tile_split_col0[1].cons(), activation_tile_join_col0[1].prod()], placement=Tile(0, 3), stack_size=0x2000)
    worker_col0_row4 = Worker(core_fn=core_fn_with_relu, fn_args=[zero_kernel, matmul_kernel, relu_kernel, input_activation_col0_fifo.cons(), weight_tile_split_col0[2].cons(), activation_tile_join_col0[2].prod()], placement=Tile(0, 4), stack_size=0x2000)
    worker_col0_row5 = Worker(core_fn=core_fn_with_relu, fn_args=[zero_kernel, matmul_kernel, relu_kernel, input_activation_col0_fifo.cons(), weight_tile_split_col0[3].cons(), activation_tile_join_col0[3].prod()], placement=Tile(0, 5), stack_size=0x2000)
    worker_col1_row2 = Worker(core_fn=core_fn_with_relu, fn_args=[zero_kernel, matmul_kernel, relu_kernel, input_activation_col1_fifo.cons(), weight_tile_split_col1[0].cons(), activation_tile_join_col1[0].prod()], placement=Tile(1, 2), stack_size=0x2000)
    worker_col1_row3 = Worker(core_fn=core_fn_with_relu, fn_args=[zero_kernel, matmul_kernel, relu_kernel, input_activation_col1_fifo.cons(), weight_tile_split_col1[1].cons(), activation_tile_join_col1[1].prod()], placement=Tile(1, 3), stack_size=0x2000)
    worker_col1_row4 = Worker(core_fn=core_fn_with_relu, fn_args=[zero_kernel, matmul_kernel, relu_kernel, input_activation_col1_fifo.cons(), weight_tile_split_col1[2].cons(), activation_tile_join_col1[2].prod()], placement=Tile(1, 4), stack_size=0x2000)
    worker_col1_row5 = Worker(core_fn=core_fn_with_relu, fn_args=[zero_kernel, matmul_kernel, relu_kernel, input_activation_col1_fifo.cons(), weight_tile_split_col1[3].cons(), activation_tile_join_col1[3].prod()], placement=Tile(1, 5), stack_size=0x2000)
    worker_col2_row2 = Worker(core_fn=core_fn_with_relu, fn_args=[zero_kernel, matmul_kernel, relu_kernel, input_activation_col2_fifo.cons(), weight_tile_split_col2[0].cons(), activation_tile_join_col2[0].prod()], placement=Tile(2, 2), stack_size=0x2000)
    worker_col2_row3 = Worker(core_fn=core_fn_with_relu, fn_args=[zero_kernel, matmul_kernel, relu_kernel, input_activation_col2_fifo.cons(), weight_tile_split_col2[1].cons(), activation_tile_join_col2[1].prod()], placement=Tile(2, 3), stack_size=0x2000)
    worker_col2_row4 = Worker(core_fn=core_fn_with_relu, fn_args=[zero_kernel, matmul_kernel, relu_kernel, input_activation_col2_fifo.cons(), weight_tile_split_col2[2].cons(), activation_tile_join_col2[2].prod()], placement=Tile(2, 4), stack_size=0x2000)
    worker_col2_row5 = Worker(core_fn=core_fn_with_relu, fn_args=[zero_kernel, matmul_kernel, relu_kernel, input_activation_col2_fifo.cons(), weight_tile_split_col2[3].cons(), activation_tile_join_col2[3].prod()], placement=Tile(2, 5), stack_size=0x2000)
    # Layer 3 (final): zero → matmul, no relu
    worker_col3_row2 = Worker(core_fn=core_fn_final, fn_args=[zero_kernel, matmul_kernel, input_activation_col3_fifo.cons(), weight_tile_split_col3[0].cons(), activation_tile_join_col3[0].prod()], placement=Tile(3, 2), stack_size=0xD00)
    worker_col3_row3 = Worker(core_fn=core_fn_final, fn_args=[zero_kernel, matmul_kernel, input_activation_col3_fifo.cons(), weight_tile_split_col3[1].cons(), activation_tile_join_col3[1].prod()], placement=Tile(3, 3), stack_size=0xD00)
    worker_col3_row4 = Worker(core_fn=core_fn_final, fn_args=[zero_kernel, matmul_kernel, input_activation_col3_fifo.cons(), weight_tile_split_col3[2].cons(), activation_tile_join_col3[2].prod()], placement=Tile(3, 4), stack_size=0xD00)
    worker_col3_row5 = Worker(core_fn=core_fn_final, fn_args=[zero_kernel, matmul_kernel, input_activation_col3_fifo.cons(), weight_tile_split_col3[3].cons(), activation_tile_join_col3[3].prod()], placement=Tile(3, 5), stack_size=0xD00)

    Workers = [
        worker_col0_row2, worker_col0_row3, worker_col0_row4, worker_col0_row5,
        worker_col1_row2, worker_col1_row3, worker_col1_row4, worker_col1_row5,
        worker_col2_row2, worker_col2_row3, worker_col2_row4, worker_col2_row5,
        worker_col3_row2, worker_col3_row3, worker_col3_row4, worker_col3_row5,
    ]

    # Tensor Access Patterns (TAPs)
    tap_input_activation = TensorAccessPattern(tensor_dims=[X.numel()],  offset=0, sizes=[1, X.numel()],  strides=[X.numel(),  1])
    tap_weights_layer0   = TensorAccessPattern(tensor_dims=[W0.numel()], offset=0, sizes=[1, W0.numel()], strides=[W0.numel(), 1])
    tap_weights_layer1   = TensorAccessPattern(tensor_dims=[W1.numel()], offset=0, sizes=[1, W1.numel()], strides=[W1.numel(), 1])
    tap_weights_layer2   = TensorAccessPattern(tensor_dims=[W2.numel()], offset=0, sizes=[1, W2.numel()], strides=[W2.numel(), 1])
    tap_weights_layer3   = TensorAccessPattern(tensor_dims=[W3.numel()], offset=0, sizes=[1, W3.numel()], strides=[W3.numel(), 1])
    tap_output_activation = TensorAccessPattern(tensor_dims=[Y.numel()], offset=0, sizes=[1, Y.numel()],  strides=[Y.numel(),  1])

    # Runtime
    runtime = Runtime()
    with runtime.sequence(activation_flat_type, weight_flat_type, weight_flat_type, weight_flat_type, weight_flat_type, activation_flat_type) as (
        input_activation_buffer, weight_layer0_buffer, weight_layer1_buffer, weight_layer2_buffer, weight_layer3_buffer, output_activation_buffer
    ):
        runtime.start(*Workers)
        # Fills
        runtime.fill(placement=Tile(0, 0), in_fifo=input_activation_fifo.prod(), source=input_activation_buffer,  tap=tap_input_activation)
        runtime.fill(placement=Tile(0, 0), in_fifo=weights_col0_fifo.prod(),     source=weight_layer0_buffer,     tap=tap_weights_layer0)
        runtime.fill(placement=Tile(1, 0), in_fifo=weights_col1_fifo.prod(),     source=weight_layer1_buffer,     tap=tap_weights_layer1)
        runtime.fill(placement=Tile(2, 0), in_fifo=weights_col2_fifo.prod(),     source=weight_layer2_buffer,     tap=tap_weights_layer2)
        runtime.fill(placement=Tile(3, 0), in_fifo=weights_col3_fifo.prod(),     source=weight_layer3_buffer,     tap=tap_weights_layer3)
        # Drains
        runtime.drain(placement=Tile(3, 0), out_fifo=output_activation_fifo.cons(), dest=output_activation_buffer, wait=True, tap=tap_output_activation)

    # Program
    my_program = Program(iron.get_current_device(), runtime)
    return my_program.resolve_program(SequentialPlacer())


def main():
    M, K, N = 64, 64, 64

    X  = iron.randint(-128, 127, (M * K,), dtype=np.int16, device="npu")
    W0 = iron.randint(-128, 127, (K * N,), dtype=np.int16, device="npu")
    W1 = iron.randint(-128, 127, (K * N,), dtype=np.int16, device="npu")
    W2 = iron.randint(-128, 127, (K * N,), dtype=np.int16, device="npu")
    W3 = iron.randint(-128, 127, (K * N,), dtype=np.int16, device="npu")
    Y  = iron.zeros(M * N, dtype=np.int16, device="npu")

    mlp_jit(X, W0, W1, W2, W3, Y)

    print(Y)

    # Validation: compute reference output in numpy
    x_np  = np.array(X, dtype=np.float32).reshape(M, K)
    w0_np = np.array(W0, dtype=np.float32).reshape(K, N)
    w1_np = np.array(W1, dtype=np.float32).reshape(K, N)
    w2_np = np.array(W2, dtype=np.float32).reshape(K, N)
    w3_np = np.array(W3, dtype=np.float32).reshape(K, N)

    # Simulate int16 overflow between layers to match NPU arithmetic.
    # Each layer: matmul in float32, truncate to int16, apply relu, promote back.
    def i16_relu(x): return np.maximum(x.astype(np.int16).astype(np.float32), 0)
    def i16(x):      return x.astype(np.int16).astype(np.float32)

    h1 = i16_relu(x_np @ w0_np)
    h2 = i16_relu(h1   @ w1_np)
    h3 = i16_relu(h2   @ w2_np)
    h4 = i16     (h3   @ w3_np)   # no relu on final layer
    expected = h4.astype(np.int16).flatten()

    actual = np.array(Y, dtype=np.int16)

    print("Element-by-element comparison (first 8):")
    for i in range(min(8, M * N)):
        print(f"  [{i}] expected={expected[i]:6d}  actual={actual[i]:6d}")

    mismatches = np.where(actual != expected)[0]
    if len(mismatches) == 0:
        print(f"Validation passed: all {M * N} elements match")
    else:
        print(f"Validation failed: {len(mismatches)} mismatches")
        for idx in mismatches[:5]:
            print(f"  [{idx}] expected={expected[idx]}, actual={actual[idx]}")
        if len(mismatches) > 5:
            print(f"  ... and {len(mismatches) - 5} more")


if __name__ == "__main__":
    main()
