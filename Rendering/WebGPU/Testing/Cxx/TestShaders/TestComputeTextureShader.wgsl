// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

@group(0) @binding(0) var outputTexture: texture_storage_2d<rgba8unorm, write>;

@compute @workgroup_size(8, 8, 1)
fn computeColor(@builtin(global_invocation_id) id: vec3<u32>)
{
  let dims = textureDimensions(outputTexture);
  if (id.x >= dims.x || id.y >= dims.y)
  {
    return;
  }

  textureStore(outputTexture, id.xy, vec4f(vec3f(42.0f / 255.0f), 1.0f));
}
