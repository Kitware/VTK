// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

@group(0) @binding(0) var depthTexture: texture_depth_2d;
@group(0) @binding(1) var<storage, read_write> pointDepthBuffer: array<u32>;

@compute
@workgroup_size(8, 8, 1)
fn computeMain(@builtin(global_invocation_id) id: vec3<u32>)
{
  let dims = textureDimensions(depthTexture);
  if (id.x >= dims.x || id.y >= dims.y)
  {
      return;
  }

  let pixelIndex = id.x + id.y * dims.x;

  let loadCoords = vec2u(id.x, id.y);
  pointDepthBuffer[pixelIndex] = u32(textureLoad(depthTexture, loadCoords, 0) * (pow(2.0, 32) - 1));
}
