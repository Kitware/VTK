// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

struct EnvironmentParameters
{
  right: vec4<f32>,
  up: vec4<f32>,
  front: vec4<f32>,
  size: u32,
  convert_to_linear: u32,
  pad0: u32,
  pad1: u32,
}

@group(0) @binding(0) var<uniform> parameters: EnvironmentParameters;
@group(0) @binding(1) var source_sampler: sampler;
@group(0) @binding(2) var source_equirect: texture_2d<f32>;
@group(0) @binding(3) var source_cube: texture_cube<f32>;
@group(0) @binding(4) var output_faces: texture_storage_2d_array<rgba16float, write>;
@group(0) @binding(5) var input_faces: texture_2d_array<f32>;

fn toLinear(color: vec3<f32>) -> vec3<f32>
{
  if (parameters.convert_to_linear != 0u)
  {
    return pow(max(color, vec3<f32>(0.0)), vec3<f32>(2.2));
  }
  return color;
}

fn sampleEquirect(dir: vec3<f32>) -> vec3<f32>
{
  let theta = atan2(dir.z, dir.x);
  let phi = asin(clamp(dir.y, -1.0, 1.0));
  let p = vec2<f32>(theta * (0.5 / PI) + 0.5, phi * (1.0 / PI) + 0.5);
  return textureSampleLevel(source_equirect, source_sampler, p, 0.0).rgb;
}

fn sampleCube(dir: vec3<f32>) -> vec3<f32>
{
  let dirv = vec3<f32>(dot(dir, parameters.right.xyz), dot(dir, parameters.up.xyz),
    dot(dir, parameters.front.xyz));
  return textureSampleLevel(source_cube, source_sampler, dirv, 0.0).rgb;
}

const SUBSAMPLES: array<vec2<f32>, 4> = array<vec2<f32>, 4>(
  vec2<f32>(0.25, 0.25), vec2<f32>(0.75, 0.25), vec2<f32>(0.25, 0.75), vec2<f32>(0.75, 0.75));

@compute @workgroup_size(8, 8, 1)
fn equirectangularToCube(@builtin(global_invocation_id) id: vec3<u32>)
{
  if (id.x >= parameters.size || id.y >= parameters.size)
  {
    return;
  }
  var color = vec3<f32>(0.0);
  for (var i = 0u; i < 4u; i++)
  {
    let uv = (vec2<f32>(id.xy) + SUBSAMPLES[i]) / f32(parameters.size);
    color += toLinear(sampleEquirect(cubeDirection(id.z, uv)));
  }
  textureStore(output_faces, id.xy, id.z, vec4<f32>(0.25 * color, 1.0));
}

@compute @workgroup_size(8, 8, 1)
fn cubeToCube(@builtin(global_invocation_id) id: vec3<u32>)
{
  if (id.x >= parameters.size || id.y >= parameters.size)
  {
    return;
  }
  let uv = (vec2<f32>(id.xy) + vec2<f32>(0.5)) / f32(parameters.size);
  let color = toLinear(sampleCube(cubeDirection(id.z, uv)));
  textureStore(output_faces, id.xy, id.z, vec4<f32>(color, 1.0));
}

@compute @workgroup_size(8, 8, 1)
fn downsample(@builtin(global_invocation_id) id: vec3<u32>)
{
  if (id.x >= parameters.size || id.y >= parameters.size)
  {
    return;
  }
  let src = vec2<i32>(id.xy) * 2;
  let layer = i32(id.z);
  let color = textureLoad(input_faces, src, layer, 0) +
    textureLoad(input_faces, src + vec2<i32>(1, 0), layer, 0) +
    textureLoad(input_faces, src + vec2<i32>(0, 1), layer, 0) +
    textureLoad(input_faces, src + vec2<i32>(1, 1), layer, 0);
  textureStore(output_faces, id.xy, id.z, 0.25 * color);
}
