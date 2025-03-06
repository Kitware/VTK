// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

@group(0) @binding(0) var inMipmap: texture_2d<f32>;
@group(0) @binding(1) var outMipmap: texture_storage_2d<r32float, write>;

@compute
@workgroup_size(8, 8, 1)
fn computeMain(@builtin(global_invocation_id) id: vec3<u32>)
{
  let inputMipLevelDims = textureDimensions(inMipmap);
  let targetMipLevelDims = textureDimensions(outMipmap);
  if (id.x >= targetMipLevelDims.x || id.y >= targetMipLevelDims.y)
  {
    return;
  }

  let depth1 = textureLoad(inMipmap, id.xy * 2 + vec2u(0, 0), 0).r;
  let depth2 = textureLoad(inMipmap, id.xy * 2 + vec2u(1, 0), 0).r;
  let depth3 = textureLoad(inMipmap, id.xy * 2 + vec2u(0, 1), 0).r;
  let depth4 = textureLoad(inMipmap, id.xy * 2 + vec2u(1, 1), 0).r;

  var outDepth = max(depth1, max(depth2, max(depth3, depth4)));

  // Handling non-power of two dimensions by incorporating the depth
  // of additional pixels if the width or height (or both) is odd
  let widthOdd: bool = (inputMipLevelDims.x & 1) != 0;
  let heightOdd: bool = (inputMipLevelDims.y & 1) != 0;

  if (widthOdd)
  {
   let additionalTexel1 = textureLoad(inMipmap, id.xy * 2 + vec2u(2, 0), 0).r;
   let additionalTexel2 = textureLoad(inMipmap, id.xy * 2 + vec2u(2, 1), 0).r;
    outDepth = max(outDepth, max(additionalTexel1, additionalTexel2));

    if (heightOdd)
    {
        // If the height is also odd, we need to take the "corner" pixel into account
        let additionalTexel3  = textureLoad(inMipmap, id.xy * 2 + vec2u(2, 2), 0).r;
        outDepth = max(outDepth, additionalTexel3);
    }
  }

  if (heightOdd)
  {
    let additionalTexel1 = textureLoad(inMipmap, id.xy * 2 + vec2u(0, 2), 0).r;
    let additionalTexel2 = textureLoad(inMipmap, id.xy * 2 + vec2u(1, 2), 0).r;
    outDepth = max(outDepth, max(additionalTexel1, additionalTexel2));
  }

  textureStore(outMipmap, id.xy, vec4f(vec3f(outDepth), 1.0f));
}
