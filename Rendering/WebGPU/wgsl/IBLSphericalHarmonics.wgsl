// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

struct HarmonicsParameters
{
  size: u32,
  pad0: u32,
  pad1: u32,
  pad2: u32,
}

const WORKGROUP_SIZE: u32 = 64u;

@group(0) @binding(0) var<uniform> parameters: HarmonicsParameters;
@group(0) @binding(1) var environment_faces: texture_2d_array<f32>;
@group(0) @binding(2) var<storage, read_write> coefficients: array<vec4<f32>, 9>;

var<workgroup> partial_sums: array<array<vec3<f32>, 9>, WORKGROUP_SIZE>;
var<workgroup> partial_weights: array<f32, WORKGROUP_SIZE>;

fn texelSolidAngle(uv: vec2<f32>, invSize: f32) -> f32
{
  let x0 = 2.0 * (uv.x - 0.5 * invSize) - 1.0;
  let y0 = 2.0 * (uv.y - 0.5 * invSize) - 1.0;
  let x1 = x0 + 2.0 * invSize;
  let y1 = y0 + 2.0 * invSize;
  let a00 = atan2(x0 * y0, sqrt(x0 * x0 + y0 * y0 + 1.0));
  let a01 = atan2(x0 * y1, sqrt(x0 * x0 + y1 * y1 + 1.0));
  let a10 = atan2(x1 * y0, sqrt(x1 * x1 + y0 * y0 + 1.0));
  let a11 = atan2(x1 * y1, sqrt(x1 * x1 + y1 * y1 + 1.0));
  return a00 - a01 - a10 + a11;
}

@compute @workgroup_size(64, 1, 1)
fn sphericalHarmonics(@builtin(local_invocation_index) local: u32)
{
  var sums: array<vec3<f32>, 9>;
  for (var k = 0u; k < 9u; k++)
  {
    sums[k] = vec3<f32>(0.0);
  }
  var weightSum = 0.0;
  let size = parameters.size;
  let invSize = 1.0 / f32(size);
  let faceSize = size * size;
  let texelCount = 6u * faceSize;
  for (var index = local; index < texelCount; index += WORKGROUP_SIZE)
  {
    let face = index / faceSize;
    let texel = index % faceSize;
    let xy = vec2<u32>(texel % size, texel / size);
    let uv = (vec2<f32>(xy) + vec2<f32>(0.5)) * invSize;
    let n = cubeDirection(face, uv);
    let weight = texelSolidAngle(uv, invSize);
    let color = textureLoad(environment_faces, vec2<i32>(xy), i32(face), 0).rgb * weight;
    sums[0] += color * 0.282095;
    sums[1] += color * (-0.488603 * n.y);
    sums[2] += color * (0.488603 * n.z);
    sums[3] += color * (-0.488603 * n.x);
    sums[4] += color * (1.092548 * n.x * n.y);
    sums[5] += color * (-1.092548 * n.y * n.z);
    sums[6] += color * (0.315392 * (3.0 * n.z * n.z - 1.0));
    sums[7] += color * (-1.092548 * n.x * n.z);
    sums[8] += color * (0.546274 * (n.x * n.x - n.y * n.y));
    weightSum += weight;
  }
  partial_sums[local] = sums;
  partial_weights[local] = weightSum;
  workgroupBarrier();
  for (var stride = WORKGROUP_SIZE / 2u; stride > 0u; stride = stride / 2u)
  {
    if (local < stride)
    {
      for (var k = 0u; k < 9u; k++)
      {
        partial_sums[local][k] += partial_sums[local + stride][k];
      }
      partial_weights[local] += partial_weights[local + stride];
    }
    workgroupBarrier();
  }
  if (local == 0u)
  {
    let totalWeight = partial_weights[0];
    let normalization = select(0.0, 4.0 * PI / totalWeight, totalWeight > 0.0);
    let lambert = array<f32, 9>(0.282095, -0.488603 * (2.0 / 3.0), 0.488603 * (2.0 / 3.0),
      -0.488603 * (2.0 / 3.0), 1.092548 * 0.25, -1.092548 * 0.25, 0.315392 * 0.25,
      -1.092548 * 0.25, 0.546274 * 0.25);
    for (var k = 0u; k < 9u; k++)
    {
      coefficients[k] = vec4<f32>(partial_sums[0][k] * normalization * lambert[k], 0.0);
    }
  }
}
