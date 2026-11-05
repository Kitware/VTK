// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

struct PrefilterParameters
{
  roughness: f32,
  texel_solid_angle: f32,
  max_source_level: f32,
  size: u32,
  sample_count: u32,
  pad0: u32,
  pad1: u32,
  pad2: u32,
}

@group(0) @binding(0) var<uniform> parameters: PrefilterParameters;
@group(0) @binding(1) var environment_sampler: sampler;
@group(0) @binding(2) var environment: texture_cube<f32>;
@group(0) @binding(3) var output_faces: texture_storage_2d_array<rgba16float, write>;

@compute @workgroup_size(8, 8, 1)
fn prefilter(@builtin(global_invocation_id) id: vec3<u32>)
{
  if (id.x >= parameters.size || id.y >= parameters.size)
  {
    return;
  }
  let uv = (vec2<f32>(id.xy) + vec2<f32>(0.5)) / f32(parameters.size);
  let n = cubeDirection(id.z, uv);
  let roughness = clamp(parameters.roughness, 0.0, 1.0);
  if (roughness == 0.0)
  {
    let color = textureSampleLevel(environment, environment_sampler, n, 0.0).rgb;
    textureStore(output_faces, id.xy, id.z, vec4<f32>(color, 1.0));
    return;
  }
  let sampleCount = f32(parameters.sample_count);
  var color = vec3<f32>(0.0);
  var weight = 0.0;
  for (var i = 0u; i < parameters.sample_count; i++)
  {
    let h = importanceSampleGGX(hammersley(i, parameters.sample_count), n, roughness);
    let NdH = max(dot(n, h), 0.0);
    let l = normalize(2.0 * NdH * h - n);
    let NdL = max(dot(n, l), 0.0);
    if (NdL > 0.0)
    {
      let pdf = distributionGGX(NdH, roughness);
      let sampleSolidAngle = 1.0 / (sampleCount * pdf);
      let level = clamp(0.5 * (2.0 + log2(sampleSolidAngle) - log2(parameters.texel_solid_angle)),
        0.0, parameters.max_source_level);
      color += textureSampleLevel(environment, environment_sampler, l, level).rgb * NdL;
      weight += NdL;
    }
  }
  textureStore(output_faces, id.xy, id.z, vec4<f32>(color / max(weight, 1e-6), 1.0));
}
