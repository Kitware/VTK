// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

@group(0) @binding(0) var depthTexture: texture_depth_2d;
@group(0) @binding(1) var<storage, read_write> outBuffer: array<f32>;

@compute
@workgroup_size(8, 8, 1)
fn computeMain(@builtin(global_invocation_id) id: vec3<u32>)
{
  let dims = textureDimensions(depthTexture);
  if (id.x >= dims.x || id.y >= dims.y)
  {
    return;
  }
  outBuffer[id.x + dims.x * id.y] = textureLoad(depthTexture, id.xy, 0);
  // textureStore(outTexture, id.xy, vec4f(vec3f(textureLoad(depthTexture, id.xy, 0)), 1.0f));
}
