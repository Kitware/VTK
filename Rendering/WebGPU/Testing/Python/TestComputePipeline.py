# SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
# SPDX-License-Identifier: BSD-3-Clause

from vtkmodules.vtkCommonCore import vtkFloatArray
from vtkmodules.vtkRenderingWebGPU import (
    vtkWebGPUComputeBuffer,
    vtkWebGPUComputePipeline,
)

import os
from vtkmodules.util.misc import vtkGetDataRoot

VTK_DATA_ROOT = vtkGetDataRoot()
shader_path = os.path.join(VTK_DATA_ROOT, "Data", "wgsl", "TestMultiply.wgsl")

pipeline = vtkWebGPUComputePipeline()
pass_obj = pipeline.CreateComputePass()

pass_obj.SetShaderSourceFromPath(shader_path)
pass_obj.SetShaderEntryPoint("main")

inputs = [1.0, 2.0, 3.0, 4.0]
expected = [2.0, 4.0, 6.0, 8.0]

input_array = vtkFloatArray()
for val in inputs:
    input_array.InsertNextValue(val)

input_buf = vtkWebGPUComputeBuffer()
input_buf.SetGroup(0)
input_buf.SetBinding(0)
input_buf.SetMode(vtkWebGPUComputeBuffer.READ_ONLY_COMPUTE_STORAGE)
input_buf.SetData(input_array)
input_buf.SetDataType(vtkWebGPUComputeBuffer.VTK_DATA_ARRAY)

output_buf = vtkWebGPUComputeBuffer()
output_buf.SetGroup(0)
output_buf.SetBinding(1)
output_buf.SetMode(vtkWebGPUComputeBuffer.READ_WRITE_MAP_COMPUTE_STORAGE)
output_buf.SetByteSize(len(inputs) * 4)

idx_in = pass_obj.AddBuffer(input_buf)
idx_out = pass_obj.AddBuffer(output_buf)

pass_obj.SetWorkgroups(len(inputs), 1, 1)
pass_obj.Dispatch()

output_array = vtkFloatArray()
pass_obj.ReadBufferFromGPU(idx_out, output_array)

pipeline.Update()

assert output_array.GetNumberOfTuples() == len(
    expected
), f"Expected {len(expected)} tuples, got {output_array.GetNumberOfTuples()}"

for i, exp in enumerate(expected):
    val = output_array.GetValue(i)
    assert (
        abs(val - exp) < 1e-5
    ), f"Mismatch at index {i}: expected {exp}, got {val}"
