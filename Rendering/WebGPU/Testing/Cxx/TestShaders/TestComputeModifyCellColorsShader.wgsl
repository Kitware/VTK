// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// The compute pipeline automatically defines 2 uniforms that give the offset into the
// cellDataBuffer that needs to be used to access the relevant part of the buffer as well
// as the length of the relevant part of the buffer for bounds checking.
// We're defining a structure to make accessing the uniforms more explicit. We could
// also have declared a u32 array of length 2, that would have worked the same.
// The structure basically allows to have names on these u32 values.
struct RenderBufferInfos
{
  floatCountOffset: u32,
  elementCount: u32
};

@group(0) @binding(0) var<storage, read_write> cellDataBuffer: array<f32>;
// The following buffer is automatically bound by the compute pipeline
@group(0) @binding(1) var<uniform> renderBufferInfos: RenderBufferInfos;

@compute @workgroup_size(32, 1, 1)
fn changeCellColorCompute(@builtin(global_invocation_id) id: vec3<u32>)
{
    let floatOffset = renderBufferInfos.floatCountOffset;
    let elementCount = renderBufferInfos.elementCount;

    if (id.x >= elementCount)
    {
        return;
    }

    // Turning the cell color white
    // Offsetting and multiplying the index by 4u because colors are vec4f
    cellDataBuffer[floatOffset + id.x * 4u + 0u] = 1.0f;
    cellDataBuffer[floatOffset + id.x * 4u + 1u] = 1.0f;
    cellDataBuffer[floatOffset + id.x * 4u + 2u] = 1.0f;
    cellDataBuffer[floatOffset + id.x * 4u + 3u] = 1.0f;
}
