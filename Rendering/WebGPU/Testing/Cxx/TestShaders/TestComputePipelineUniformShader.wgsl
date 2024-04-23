// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

@group(0) @binding(0) var<storage, read> inputVector1: array<i32>;
@group(0) @binding(1) var<storage, read> inputVector2: array<f32>;
@group(0) @binding(2) var<uniform> uniformMultiplier: f32;
@group(0) @binding(3) var<storage, read_write> outputData: array<f32>;

@compute @workgroup_size(32, 1, 1)
fn computeFunction(@builtin(global_invocation_id) id: vec3<u32>)
{
    if (id.x >= arrayLength(&inputVector1))
    {
        return;
    }

    outputData[id.x] = f32(inputVector1[id.x]) * inputVector2[id.x] * uniformMultiplier;
}
