// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

const PI: f32 = 3.14159265359;

fn cubeDirection(face: u32, uv: vec2<f32>) -> vec3<f32>
{
  let s = 2.0 * uv.x - 1.0;
  let t = 2.0 * uv.y - 1.0;
  switch face
  {
    case 0u: { return normalize(vec3<f32>(1.0, -t, -s)); }
    case 1u: { return normalize(vec3<f32>(-1.0, -t, s)); }
    case 2u: { return normalize(vec3<f32>(s, 1.0, t)); }
    case 3u: { return normalize(vec3<f32>(s, -1.0, -t)); }
    case 4u: { return normalize(vec3<f32>(s, -t, 1.0)); }
    default: { return normalize(vec3<f32>(-s, -t, -1.0)); }
  }
}

fn radicalInverseVdC(inBits: u32) -> f32
{
  var bits = (inBits << 16u) | (inBits >> 16u);
  bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
  bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
  bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
  bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
  return f32(bits) * 2.3283064365386963e-10;
}

fn hammersley(i: u32, n: u32) -> vec2<f32>
{
  return vec2<f32>(f32(i) / f32(n), radicalInverseVdC(i));
}

fn importanceSampleGGX(rd: vec2<f32>, n: vec3<f32>, roughness: f32) -> vec3<f32>
{
  let a = max(roughness * roughness, 1e-4);
  let phi = 2.0 * PI * rd.x;
  let cosTheta = sqrt((1.0 - rd.y) / (1.0 + (a * a - 1.0) * rd.y));
  let sinTheta = sqrt(1.0 - cosTheta * cosTheta);
  let h = vec3<f32>(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
  let up = select(vec3<f32>(1.0, 0.0, 0.0), vec3<f32>(0.0, 0.0, 1.0), abs(n.z) < 0.999);
  let tangent = normalize(cross(up, n));
  let bitangent = cross(n, tangent);
  return normalize(tangent * h.x + bitangent * h.y + n * h.z);
}

fn distributionGGX(NdH: f32, roughness: f32) -> f32
{
  let alpha = max(roughness * roughness, 1e-4);
  let alpha2 = alpha * alpha;
  let denom = NdH * NdH * (alpha2 - 1.0) + 1.0;
  return alpha2 / (PI * max(denom * denom, 1e-8));
}
