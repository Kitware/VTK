// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

struct LookupTableParameters
{
  size: u32,
  sample_count: u32,
  pad0: u32,
  pad1: u32,
}

@group(0) @binding(0) var<uniform> parameters: LookupTableParameters;
@group(0) @binding(1) var output_table: texture_storage_2d<rgba16float, write>;

fn geometrySchlickGGX(NdV: f32, k: f32) -> f32
{
  return NdV / (NdV * (1.0 - k) + k);
}

@compute @workgroup_size(8, 8, 1)
fn brdfLookupTable(@builtin(global_invocation_id) id: vec3<u32>)
{
  if (id.x >= parameters.size || id.y >= parameters.size)
  {
    return;
  }
  let NdV = (f32(id.x) + 0.5) / f32(parameters.size);
  let roughness = (f32(id.y) + 0.5) / f32(parameters.size);
  let v = vec3<f32>(sqrt(1.0 - NdV * NdV), 0.0, NdV);
  let n = vec3<f32>(0.0, 0.0, 1.0);
  let k = 0.5 * roughness * roughness;
  var a = 0.0;
  var b = 0.0;
  for (var i = 0u; i < parameters.sample_count; i++)
  {
    let h = importanceSampleGGX(hammersley(i, parameters.sample_count), n, roughness);
    let l = normalize(2.0 * dot(v, h) * h - v);
    let NdL = max(l.z, 0.0);
    let NdH = max(h.z, 0.0);
    let VdH = max(dot(v, h), 0.0);
    if (NdL > 0.0 && NdH > 0.0)
    {
      let g = geometrySchlickGGX(NdV, k) * geometrySchlickGGX(NdL, k);
      let gVis = (g * VdH) / (NdH * NdV);
      let fc = pow(1.0 - VdH, 5.0);
      a += (1.0 - fc) * gVis;
      b += fc * gVis;
    }
  }
  let count = f32(parameters.sample_count);
  textureStore(output_table, id.xy, vec4<f32>(a / count, b / count, 0.0, 1.0));
}
