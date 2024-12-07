// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

@group(0) @binding(0) var<storage, read_write> inputOutput: array<f32>;

@compute @workgroup_size(32, 1, 1)
fn computeFunctionAdd(@builtin(global_invocation_id) id: vec3<u32>)
{
    if (id.x >= arrayLength(&inputOutput))
    {
        return;
    }

    inputOutput[id.x] = inputOutput[id.x] + 42.0f;
}
