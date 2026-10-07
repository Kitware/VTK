// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

const PBR_PI: f32 = 3.14159265359;
const PBR_RECIPROCAL_PI: f32 = 0.31830988618;

struct PBRSurface
{
  normal: vec3<f32>,
  view: vec3<f32>,
  NdV: f32,
  tangent: vec3<f32>,
  bitangent: vec3<f32>,
  TdV: f32,
  BdV: f32,
  coat_normal: vec3<f32>,
  coat_NdV: f32,
}

struct PBRMaterial
{
  albedo: vec3<f32>,
  metallic: f32,
  roughness: f32,
  F0: vec3<f32>,
  F90: vec3<f32>,
  anisotropy: f32,
  alpha_t: f32,
  alpha_b: f32,
  coat_strength: f32,
  coat_roughness: f32,
  coat_F0: vec3<f32>,
  coat_F90: vec3<f32>,
  coat_color_factor: vec3<f32>,
}

fn pbrDistributionGGX(NdH: f32, roughness: f32) -> f32
{
  let a = roughness * roughness;
  let a2 = a * a;
  let d = (NdH * a2 - NdH) * NdH + 1.0;
  return a2 / (PBR_PI * d * d);
}

fn pbrVisibilitySmithCorrelated(NdV: f32, NdL: f32, roughness: f32) -> f32
{
  let a2 = roughness * roughness;
  let ggxV = NdL * sqrt(a2 + NdV * (NdV - a2 * NdV));
  let ggxL = NdV * sqrt(a2 + NdL * (NdL - a2 * NdL));
  return 0.5 / (ggxV + ggxL);
}

fn pbrFresnelSchlick(F0: vec3<f32>, F90: vec3<f32>, HdL: f32) -> vec3<f32>
{
  return F0 + (F90 - F0) * pow(clamp(1.0 - HdL, 0.0, 1.0), 5.0);
}

fn pbrDiffuseLambert(albedo: vec3<f32>) -> vec3<f32>
{
  return albedo * PBR_RECIPROCAL_PI;
}

fn pbrDistributionGGXAnisotropic(at: f32, ab: f32, TdH: f32, BdH: f32, NdH: f32) -> f32
{
  let a2 = at * ab;
  let d = vec3<f32>(ab * TdH, at * BdH, a2 * NdH);
  let b2 = a2 / dot(d, d);
  return a2 * b2 * b2 * PBR_RECIPROCAL_PI;
}

fn pbrVisibilitySmithCorrelatedAnisotropic(at: f32, ab: f32, TdV: f32, BdV: f32, TdL: f32,
  BdL: f32, NdV: f32, NdL: f32) -> f32
{
  let lambdaV = NdL * length(vec3<f32>(at * TdV, ab * BdV, NdV));
  let lambdaL = NdV * length(vec3<f32>(at * TdL, ab * BdL, NdL));
  return 0.5 / (lambdaV + lambdaL);
}

fn pbrEvaluateSphericalHarmonics(n: vec3<f32>, sh: ptr<function, array<vec3<f32>, 9>>) -> vec3<f32>
{
  var v = (*sh)[0];
  v += (*sh)[1] * n.y;
  v += (*sh)[2] * n.z;
  v += (*sh)[3] * n.x;
  v += (*sh)[4] * n.x * n.y;
  v += (*sh)[5] * n.y * n.z;
  v += (*sh)[6] * (3.0 * n.z * n.z - 1.0);
  v += (*sh)[7] * n.x * n.z;
  v += (*sh)[8] * (n.x * n.x - n.y * n.y);
  return max(v, vec3<f32>(0.0));
}

fn pbrEvaluateLight(surface: PBRSurface, material: PBRMaterial, L: vec3<f32>,
  incoming: vec3<f32>, hasAnisotropy: bool, hasCoat: bool) -> vec3<f32>
{
  let halfSum = surface.view + L;
  let H = halfSum * inverseSqrt(max(dot(halfSum, halfSum), 1e-12));
  let NdV = surface.NdV;
  let HdL = clamp(dot(H, L), 1e-5, 1.0);
  let NdL = clamp(dot(surface.normal, L), 1e-5, 1.0);
  let NdH = clamp(dot(surface.normal, H), 1e-5, 1.0);
  let F = pbrFresnelSchlick(material.F0, material.F90, HdL);
  var specular: vec3<f32>;
  if (hasAnisotropy)
  {
    let TdH = dot(surface.tangent, H);
    let BdH = dot(surface.bitangent, H);
    let TdL = dot(surface.tangent, L);
    let BdL = dot(surface.bitangent, L);
    let D = pbrDistributionGGXAnisotropic(material.alpha_t, material.alpha_b, TdH, BdH, NdH);
    let V = pbrVisibilitySmithCorrelatedAnisotropic(
      material.alpha_t, material.alpha_b, surface.TdV, surface.BdV, TdL, BdL, NdV, NdL);
    specular = (D * V) * F;
  }
  else
  {
    let D = pbrDistributionGGX(NdH, material.roughness);
    let V = pbrVisibilitySmithCorrelated(NdV, NdL, material.roughness);
    specular = (D * V) * F;
  }
  var diffuse = (1.0 - material.metallic) * (1.0 - F) * pbrDiffuseLambert(material.albedo);
  var radiance = incoming;
  var Lo = vec3<f32>(0.0);
  if (hasCoat)
  {
    let coatNdL = clamp(dot(surface.coat_normal, L), 1e-5, 1.0);
    let coatNdH = clamp(dot(surface.coat_normal, H), 1e-5, 1.0);
    let D = pbrDistributionGGX(coatNdH, material.coat_roughness);
    let V = pbrVisibilitySmithCorrelated(surface.coat_NdV, coatNdL, material.coat_roughness);
    var Fc = pbrFresnelSchlick(material.coat_F0, material.coat_F90, HdL);
    Lo += (D * V) * Fc * radiance * coatNdL * material.coat_strength;
    Fc *= material.coat_strength;
    specular *= (1.0 - Fc) * (1.0 - Fc);
    diffuse *= (1.0 - Fc);
    radiance *= material.coat_color_factor;
  }
  return Lo + radiance * (diffuse + specular) * NdL;
}
