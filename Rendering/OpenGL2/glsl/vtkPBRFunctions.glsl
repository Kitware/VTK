//VTK::Define::Dec
//VTK::Light::Dec
const float PI = 3.14159265359;
const float recPI = 0.31830988618;
uniform float metallicUniform;
uniform float roughnessUniform;
uniform vec3 emissiveFactorUniform;
uniform float aoStrengthUniform;
uniform float baseF0Uniform;
uniform vec3 edgeTintUniform;
#ifdef ANISOTROPY
uniform float anisotropyUniform;
#endif
#ifdef CLEAR_COAT
uniform float coatF0Uniform;
uniform float coatStrengthUniform;
uniform float coatRoughnessUniform;
uniform vec3 coatColorUniform;
#endif
float D_GGX(float NdH, float roughness)
{
  float a = roughness * roughness;
  float a2 = a * a;
  float d = (NdH * a2 - NdH) * NdH + 1.0;
  return a2 / (PI * d * d);
}
float V_SmithCorrelated(float NdV, float NdL, float roughness)
{
  float a2 = roughness * roughness;
  float ggxV = NdL * sqrt(a2 + NdV * (NdV - a2 * NdV));
  float ggxL = NdV * sqrt(a2 + NdL * (NdL - a2 * NdL));
  return 0.5 / (ggxV + ggxL);
}
vec3 F_Schlick(vec3 F0, vec3 F90, float HdL)
{
  return F0 + (F90 - F0) * pow(1.0 - HdL, 5.0);
}
vec3 DiffuseLambert(vec3 albedo)
{
  return albedo * recPI;
}
vec3 SpecularIsotropic(float NdH, float NdV, float NdL, float HdL, float roughness,
  vec3 F0, vec3 F90, out vec3 F)
{
  float D = D_GGX(NdH, roughness);
  float V = V_SmithCorrelated(NdV, NdL, roughness);
  F = F_Schlick(F0, F90, HdL);
  return (D * V) * F;
}
#ifdef ANISOTROPY
// Anisotropy functions
float D_GGX_Anisotropic(float at, float ab, float TdH, float BdH, float NdH)
{
  float a2 = at * ab;
  vec3 d = vec3(ab * TdH, at * BdH, a2 * NdH);
  float d2 = dot(d, d);
  float b2 = a2 / d2;
  return a2 * b2 * b2 * recPI;
}
float V_SmithGGXCorrelated_Anisotropic(float at, float ab, float TdV, float BdV,
  float TdL, float BdL, float NdV, float NdL)
{
  float lambdaV = NdL * length(vec3(at * TdV, ab * BdV, NdV));
  float lambdaL = NdV * length(vec3(at * TdL, ab * BdL, NdL));
  return 0.5 / (lambdaV + lambdaL);
}
vec3 SpecularAnisotropic(float at, float ab, vec3 l, vec3 t, vec3 b, vec3 h, float TdV,
  float BdV, float NdH, float NdV, float NdL, float HdL, float roughness, float anisotropy,
  vec3 F0, vec3 F90, out vec3 F)
{
  float TdL = dot(t, l);
  float BdL = dot(b, l);
  float TdH = dot(t, h);
  float BdH = dot(b, h);
  // specular anisotropic BRDF
  float D = D_GGX_Anisotropic(at, ab, TdH, BdH, NdH);
  float V = V_SmithGGXCorrelated_Anisotropic(at, ab, TdV, BdV, TdL, BdL, NdV, NdL);
  F = F_Schlick(F0, F90, HdL);
  return (D * V) * F;
}
#endif
