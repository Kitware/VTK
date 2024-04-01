// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkGLSLModLight.h"
#include "vtkActor.h"
#include "vtkCellGridMapper.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationObjectBaseKey.h"
#include "vtkLightingMapPass.h"
#include "vtkLogger.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLRenderer.h"
#include "vtkPBRFunctions.h"
#include "vtkPBRIrradianceTexture.h"
#include "vtkPBRLUTTexture.h"
#include "vtkPBRPrefilterTexture.h"
#include "vtkProperty.h"
#include "vtkShaderProgram.h"
#include <sstream>

VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkGLSLModLight);

//------------------------------------------------------------------------------
vtkGLSLModLight::vtkGLSLModLight() = default;

//------------------------------------------------------------------------------
vtkGLSLModLight::~vtkGLSLModLight() = default;

//------------------------------------------------------------------------------
void vtkGLSLModLight::PrintSelf(ostream& os, vtkIndent indent)
{
  os << "LastLightComplexity: " << this->LastLightComplexity << "\n";
  os << "LastLightCount: " << this->LastLightCount << "\n";
  os << "UsePBRTextures: " << this->UsePBRTextures << "\n";
  os << "UseAnisotropy: " << this->UseAnisotropy << "\n";
  os << "UseClearCoat: " << this->UseClearCoat << "\n";
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
vtkGLSLModLight::LightStatsBasic vtkGLSLModLight::GetBasicLightStats(
  vtkOpenGLRenderer* renderer, vtkActor* actor)
{
  auto property = actor->GetProperty();
  const bool lightsEnabled = property->GetLighting();
  // if lighting is enabled, get the lighting information from opengl renderer.
  LightStatsBasic stats = {};
  if (lightsEnabled)
  {
    stats.Complexity = renderer->GetLightingComplexity();
    stats.Count = renderer->GetLightingCount();
  }
  return stats;
}

//------------------------------------------------------------------------------
bool vtkGLSLModLight::ReplaceShaderValues(vtkOpenGLRenderer* renderer, std::string& vertexShader,
  std::string& /*geometryShader*/, std::string& fragmentShader,
  vtkAbstractMapper* vtkNotUsed(mapper), vtkActor* actor)
{
  vtkShaderProgram::Substitute(
    vertexShader, "//VTK::PositionVC::Dec", "smooth out vec4 vertexVCVSOutput;");
  vtkShaderProgram::Substitute(
    fragmentShader, "//VTK::PositionVC::Dec", "smooth in vec4 vertexVCVSOutput;");
  vtkShaderProgram::Substitute(
    fragmentShader, "//VTK::PositionVC::Impl", "vec4 vertexVC = vertexVCVSOutput;");

  // Only if normal was not already declared as an output in vertex shader
  if (!vtkShaderProgram::Substitute(
        vertexShader, "out vec3 normalVCVSOutput;", "out vec3 normalVCVSOutput;"))
  {
    vtkShaderProgram::Substitute(
      vertexShader, "//VTK::Normal::Dec", "smooth out vec3 normalVCVSOutput;");
  }
  // Only if normal was not already declared as an input in fragment shader
  if (!vtkShaderProgram::Substitute(
        fragmentShader, "in vec3 normalVCVSOutput;", "in vec3 normalVCVSOutput;"))
  {
    vtkShaderProgram::Substitute(
      fragmentShader, "//VTK::Normal::Dec", "smooth in vec3 normalVCVSOutput;");
  }

  // Generate code to handle different types of lights.
  auto info = actor->GetPropertyKeys();
  if (info && info->Has(vtkLightingMapPass::RENDER_NORMALS()))
  {
    vtkShaderProgram::Substitute(fragmentShader, "//VTK::Light::Impl",
      "  vec3 n = (normalVCVSOutput + 1.0) * 0.5;\n"
      "  gl_FragData[0] = vec4(n.x, n.y, n.z, 1.0);");
    return true;
  }
  if (info && info->Has(vtkLightingMapPass::RENDER_LUMINANCE()))
  {
    vtkShaderProgram::Substitute(fragmentShader, "//VTK::Light::Impl",
      "  diffuseColor = vec3(1, 1, 1);\n"
      "  specularColor = vec3(1, 1, 1);\n"
      "  //VTK::Light::Impl\n",
      false);
  }

  auto stats = vtkGLSLModLight::GetBasicLightStats(renderer, actor);
  this->LastLightComplexity = stats.Complexity;
  this->LastLightCount = stats.Count;

  int lastLightComplexity = this->LastLightComplexity;
  int lastLightCount = this->LastLightCount;

  if (actor->GetProperty()->GetInterpolation() != VTK_PBR && lastLightCount == 0)
  {
    lastLightComplexity = 0;
  }
  // Only if vertexNormalVCVS was not already declared in fragment shader
  if (!vtkShaderProgram::Substitute(
        fragmentShader, "vec3 vertexNormalVCVS", "vec3 vertexNormalVCVS"))
  {
    vtkShaderProgram::Substitute(fragmentShader, "//VTK::Normal::Impl",
      "vec3 vertexNormalVCVS = normalVCVSOutput;\n"
      "if (gl_FrontFacing == false) vertexNormalVCVS.z = -vertexNormalVCVS.z;\n"
      "//VTK::Normal::Impl");
  }

  vtkShaderProgram::Substitute(fragmentShader, "//VTK::Normal::Impl",
    "  vec3 normalizedNormalVCVSOutput = normalize(vertexNormalVCVS);", false);

  bool hasIBL = false;
  std::ostringstream oss;
  if (actor->GetProperty()->GetInterpolation() == VTK_PBR && lastLightComplexity > 0)
  {
    // PBR functions
    vtkShaderProgram::Substitute(fragmentShader, "//VTK::Light::Dec", vtkPBRFunctions);

    // disable default behavior with textures
    vtkShaderProgram::Substitute(fragmentShader, "//VTK::TCoord::Impl", "");

    // get color and material from textures
    auto textures = actor->GetProperty()->GetAllTextures();
    bool albedo = false;
    bool material = false;
    bool emissive = false;

    if (this->UsePBRTextures)
    {
      for (auto& t : textures)
      {
        if (t.first == "albedoTex")
        {
          albedo = true;
          oss << "vec4 albedoSample = texture(albedoTex, tcoordVCVSOutput);\n"
                 "  vec3 albedo = albedoSample.rgb * diffuseColor;\n"
                 "  opacity = intensity_opacity * albedoSample.a;\n";
        }
        else if (t.first == "materialTex")
        {
          // we are using GLTF specification here with a combined texture holding values for AO,
          // roughness and metallic on R,G,B channels respectively
          material = true;
          oss << "  vec4 material = texture(materialTex, tcoordVCVSOutput);\n"
                 "  float roughness = material.g * roughnessUniform;\n"
                 "  float metallic = material.b * metallicUniform;\n"
                 "  float ao = material.r;\n";
        }
        else if (t.first == "emissiveTex")
        {
          emissive = true;
          oss << "  vec3 emissiveColor = texture(emissiveTex, tcoordVCVSOutput).rgb;\n"
                 "  emissiveColor = emissiveColor * emissiveFactorUniform;\n";
        }
        // Anisotropy texture is sampled by mappers.
      }
    }
    vtkOpenGLRenderer* oglRen = vtkOpenGLRenderer::SafeDownCast(renderer);

    // IBL
    if (oglRen && renderer->GetUseImageBasedLighting())
    {
      hasIBL = true;
      oss << "  const float prefilterMaxLevel = float("
          << (oglRen->GetEnvMapPrefiltered()->GetPrefilterLevels() - 1) << ");\n";
    }

    if (!albedo)
    {
      // VTK colors are expressed in linear color space
      oss << "vec3 albedo = diffuseColor;\n";
    }
    if (!material)
    {
      oss << "  float roughness = roughnessUniform;\n";
      oss << "  float metallic = metallicUniform;\n";
      oss << "  float ao = 1.0;\n";
    }
    if (!emissive)
    {
      oss << "  vec3 emissiveColor = vec3(0.0);\n";
    }

    oss << "  vec3 N = normalizedNormalVCVSOutput;\n"
           "  vec3 V = normalize(-vertexVC.xyz);\n"
           "  float NdV = clamp(dot(N, V), 1e-5, 1.0);\n";

    if (this->UseAnisotropy)
    {
      // Load anisotropic functions
      vtkShaderProgram::Substitute(fragmentShader, "//VTK::Define::Dec",
        "#define ANISOTROPY\n"
        "//VTK::Define::Dec");

      // Precompute anisotropic parameters
      // at and ab are the roughness along the tangent and bitangent
      // Disney, as in OSPray
      oss << "  float r2 = roughness * roughness;\n"
             "  float aspect = sqrt(1.0 - 0.9 * anisotropy);\n";
      oss << "  float at = max(r2 / aspect, 0.001);\n"
             "  float ab = max(r2 * aspect, 0.001);\n";

      oss << "  float TdV = dot(tangentVC, V);\n"
             "  float BdV = dot(bitangentVC, V);\n";
    }

    if (this->UseClearCoat)
    {
      // Load clear coat uniforms
      vtkShaderProgram::Substitute(fragmentShader, "//VTK::Define::Dec",
        "#define CLEAR_COAT\n"
        "//VTK::Define::Dec");

      // Clear coat parameters
      oss << "  vec3 coatN = coatNormalVCVSOutput;\n";
      oss << "  float coatRoughness = coatRoughnessUniform;\n";
      oss << "  float coatStrength = coatStrengthUniform;\n";
      oss << "  float coatNdV = clamp(dot(coatN, V), 1e-5, 1.0);\n";
    }

    if (hasIBL)
    {
      if (!oglRen->GetUseSphericalHarmonics())
      {
        oss << "  vec3 irradiance = texture(irradianceTex, envMatrix*N).rgb;\n";
      }
      else
      {
        oss << "  vec3 rotN = envMatrix * N;\n";
        oss << "  vec3 irradiance = vec3(ComputeSH(rotN, shRed), ComputeSH(rotN, shGreen), "
               "ComputeSH(rotN, shBlue));\n";
      }

      if (this->UseAnisotropy)
      {
        oss << "  vec3 anisotropicTangent = cross(bitangentVC, V);\n"
               "  vec3 anisotropicNormal = cross(anisotropicTangent, bitangentVC);\n"
               "  vec3 bentNormal = normalize(mix(N, anisotropicNormal, anisotropy));\n"
               "  vec3 worldReflect = normalize(envMatrix*reflect(-V, bentNormal));\n";
      }
      else
      {
        oss << "  vec3 worldReflect = normalize(envMatrix*reflect(-V, N));\n";
      }

      oss << "  vec3 prefilteredSpecularColor = textureLod(prefilterTex, worldReflect,"
             " roughness * prefilterMaxLevel).rgb;\n";
      oss << "  vec2 brdf = texture(brdfTex, vec2(NdV, roughness)).rg;\n";

      // Use the same prefilter texture for clear coat but with the clear coat roughness and normal

      if (this->UseClearCoat)
      {
        oss << "  vec3 coatWorldReflect = normalize(envMatrix*reflect(-V,coatN));\n"
               "  vec3 prefilteredSpecularCoatColor = textureLod(prefilterTex, coatWorldReflect,"
               " coatRoughness * prefilterMaxLevel).rgb;\n"
               "  vec2 coatBrdf = texture(brdfTex, vec2(coatNdV, coatRoughness)).rg;\n";
      }
    }
    else
    {
      oss << "  vec3 irradiance = vec3(0.0);\n";
      oss << "  vec3 prefilteredSpecularColor = vec3(0.0);\n";
      oss << "  vec2 brdf = vec2(0.0, 0.0);\n";

      if (this->UseClearCoat)
      {
        oss << "  vec3 prefilteredSpecularCoatColor = vec3(0.0);\n";
        oss << "  vec2 coatBrdf = vec2(0.0);\n";
      }
    }

    oss << "  vec3 Lo = vec3(0.0);\n";

    if (lastLightComplexity != 0)
    {
      oss << "  vec3 F0 = mix(vec3(baseF0Uniform), albedo, metallic);\n"
             // specular occlusion, it affects only material with an f0 < 0.02,
             // else f90 is 1.0
             "  float f90 = clamp(dot(F0, vec3(50.0 * 0.33)), 0.0, 1.0);\n"
             "  vec3 F90 = mix(vec3(f90), edgeTintUniform, metallic);\n"
             "  vec3 L, H, radiance, F, specular, diffuse;\n"
             "  float NdL, NdH, HdL, distanceVC, attenuation, D, Vis;\n\n";
      if (this->UseClearCoat)
      {
        // Coat layer is dielectric so F0 and F90 are achromatic
        oss << "  vec3 coatF0 = vec3(coatF0Uniform);\n"
               "  vec3 coatF90 = vec3(1.0);\n"
               "  vec3 coatLayer, Fc;\n"
               "  float coatNdL, coatNdH;\n"
               "  vec3 coatColorFactor = mix(vec3(1.0), coatColorUniform, coatStrength);\n";
      }
    }

    oss << "//VTK::Light::Impl\n";

    vtkShaderProgram::Substitute(fragmentShader, "//VTK::Light::Impl", oss.str(), false);
    oss.clear();
    oss.str("");

    if (hasIBL)
    {
      oss << "//VTK::Light::Dec\n"
             "uniform mat3 envMatrix;\n"
             "uniform sampler2D brdfTex;\n"
             "uniform samplerCube prefilterTex;\n";

      if (oglRen->GetUseSphericalHarmonics())
      {
        oss << "uniform float shRed[9];\n"
               "uniform float shGreen[9];\n"
               "uniform float shBlue[9];\n"
               "float ComputeSH(vec3 n, float sh[9])\n"
               "{\n"
               "  float v = 0.0;\n"
               "  v += sh[0];\n"
               "  v += sh[1] * n.y;\n"
               "  v += sh[2] * n.z;\n"
               "  v += sh[3] * n.x;\n"
               "  v += sh[4] * n.x * n.y;\n"
               "  v += sh[5] * n.y * n.z;\n"
               "  v += sh[6] * (3.0 * n.z * n.z - 1.0);\n"
               "  v += sh[7] * n.x * n.z;\n"
               "  v += sh[8] * (n.x * n.x - n.y * n.y);\n"
               "  return max(v, 0.0);\n"
               "}\n";
      }
      else
      {
        oss << "uniform samplerCube irradianceTex;\n";
      }

      // add uniforms
      vtkShaderProgram::Substitute(fragmentShader, "//VTK::Light::Dec", oss.str());
      oss.clear();
      oss.str("");
    }
  }

  // get standard lighting declarations.
  vtkShaderProgram::Substitute(
    fragmentShader, "//VTK::Light::Dec", renderer->GetLightingUniforms());
  oss.str("");
  switch (lastLightComplexity)
  {
    case 0: // no lighting
      vtkShaderProgram::Substitute(fragmentShader, "//VTK::Light::Impl",
        "gl_FragData[0] = vec4(ambientColor + diffuseColor, opacity);\n"
        "  //VTK::Light::Impl\n",
        false);
      break;
    case 1: // headlight
      if (actor->GetProperty()->GetInterpolation() == VTK_PBR)
      {
        // L = V = H for headlights
        if (this->UseAnisotropy)
        {
          // When V=H, maybe can be optimised
          oss << "specular = SpecularAnisotropic(at, ab, V, tangentVC, bitangentVC, V, TdV, "
                 "BdV, NdV, NdV, NdV,\n"
                 "1.0, roughness, anisotropy, F0, F90, F);\n";
        }
        else
        {
          oss << "specular = SpecularIsotropic(NdV, NdV, NdV, 1.0, roughness, F0, F90, F);\n";
        }
        oss << "  diffuse = (1.0 - metallic) * (1.0 - F) * DiffuseLambert(albedo);\n"
               "  radiance = lightColor0;\n";

        if (this->UseClearCoat)
        {
          oss << "  // Clear coat is isotropic\n"
                 "  coatLayer = SpecularIsotropic(coatNdV, coatNdV, coatNdV, 1.0,"
                 " coatRoughness, coatF0, coatF90, Fc) * radiance * coatNdV * coatStrength;\n"
                 "  Fc *= coatStrength;\n"
                 "  radiance *= coatColorFactor;\n"
                 "  specular *= (1.0 - Fc) * (1.0 - Fc);\n"
                 "  diffuse *= (1.0 - Fc);\n"
                 "  Lo += coatLayer;\n";
        }
        oss << "  Lo += radiance * (diffuse + specular) * NdV;\n\n"
               "//VTK::Light::Impl\n";
      }
      else
      {
        oss << "float df = max(0.0f, normalizedNormalVCVSOutput.z);\n"
               "  float sf = pow(df, power_specular);\n"
               "  vec3 diffuse = df * diffuseColor * lightColor0;\n"
               "  vec3 specular = sf * specularColor * lightColor0;\n"
               "  gl_FragData[0] = vec4(ambientColor + diffuse + specular, opacity);\n"
               "  //VTK::Light::Impl\n";
      }
      vtkShaderProgram::Substitute(fragmentShader, "//VTK::Light::Impl", oss.str(), false);
      break;
    case 2: // light kit
      if (actor->GetProperty()->GetInterpolation() == VTK_PBR)
      {
        for (int i = 0; i < lastLightCount; ++i)
        {
          oss << "  L = normalize(-lightDirectionVC" << i
              << ");\n"
                 "  H = normalize(V + L);\n"
                 "  HdL = clamp(dot(H, L), 1e-5, 1.0);\n"
                 "  NdL = clamp(dot(N, L), 1e-5, 1.0);\n"
                 "  NdH = clamp(dot(N, H), 1e-5, 1.0);\n"
                 "  radiance = lightColor"
              << i << ";\n";

          if (this->UseAnisotropy)
          {
            oss << "  specular = SpecularAnisotropic(at, ab, L, tangentVC, bitangentVC, H, "
                   "TdV, BdV, NdH, NdV, NdL, HdL, roughness, anisotropy, F0, F90, F);\n";
          }
          else
          {
            oss << "  specular = SpecularIsotropic(NdH, NdV, NdL, HdL, roughness, F0, F90, F);\n";
          }

          oss << "  diffuse = (1.0 - metallic) * (1.0 - F) * DiffuseLambert(albedo);\n";

          if (this->UseClearCoat)
          {
            oss << "  coatNdL = clamp(dot(coatN, L), 1e-5, 1.0);\n"
                   "  coatNdH = clamp(dot(coatN, H), 1e-5, 1.0);\n"
                   "  // Clear coat is isotropic\n"
                   "  coatLayer = SpecularIsotropic(coatNdH, coatNdV, coatNdL, HdL,"
                   " coatRoughness, coatF0, coatF90, Fc) * radiance * coatNdL * coatStrength;\n"
                   "  // Energy compensation depending on how much light is reflected by the "
                   "coat layer\n"
                   "  Fc *= coatStrength;\n"
                   "  specular *= (1.0 - Fc) * (1.0 - Fc);\n"
                   "  diffuse *= (1.0 - Fc);\n"
                   "  radiance *= coatColorFactor;\n"
                   "  Lo += coatLayer;\n";
          }

          oss << "  Lo += radiance * (diffuse + specular) * NdL;\n";
        }
        oss << "//VTK::Light::Impl\n";
      }
      else
      {
        oss << "  vec3 diffuse = vec3(0,0,0);\n"
               "  vec3 specular = vec3(0,0,0);\n"
               "  float df;\n"
               "  float sf;\n";
        for (int i = 0; i < lastLightCount; ++i)
        {
          oss << "    df = max(0.0, dot(normalizedNormalVCVSOutput, -lightDirectionVC" << i
              << "));\n"
                 // if you change the next line also change vtkShadowMapPass
                 "  diffuse += (df * lightColor"
              << i << ");\n"
              << "  sf = sign(df)*pow(max(1e-5, dot( reflect(lightDirectionVC" << i
              << ", normalizedNormalVCVSOutput), normalize(-vertexVC.xyz))), "
                 "power_specular);\n"
                 // if you change the next line also change vtkShadowMapPass
                 "  specular += (sf * lightColor"
              << i << ");\n";
        }
        oss << "  diffuse = diffuse * diffuseColor;\n"
               "  specular = specular * specularColor;\n"
               "  gl_FragData[0] = vec4(ambientColor + diffuse + specular, opacity);"
               "  //VTK::Light::Impl";
      }
      vtkShaderProgram::Substitute(fragmentShader, "//VTK::Light::Impl", oss.str(), false);

      break;
    case 3: // positional
      if (actor->GetProperty()->GetInterpolation() == VTK_PBR)
      {
        for (int i = 0; i < lastLightCount; ++i)
        {
          oss << "  L = lightPositionVC" << i
              << " - vertexVC.xyz;\n"
                 "  distanceVC = length(L);\n"
                 "  L = normalize(L);\n"
                 "  H = normalize(V + L);\n"
                 "  NdL = clamp(dot(N, L), 1e-5, 1.0);\n"
                 "  NdH = clamp(dot(N, H), 1e-5, 1.0);\n"
                 "  HdL = clamp(dot(H, L), 1e-5, 1.0);\n"
                 "  if (lightPositional"
              << i
              << " == 0)\n"
                 "  {\n"
                 "    attenuation = 1.0;\n"
                 "  }\n"
                 "  else\n"
                 "  {\n"
                 "    attenuation = 1.0 / (lightAttenuation"
              << i
              << ".x\n"
                 "      + lightAttenuation"
              << i
              << ".y * distanceVC\n"
                 "      + lightAttenuation"
              << i
              << ".z * distanceVC * distanceVC);\n"
                 "    // cone angle is less than 90 for a spot light\n"
                 "    if (lightConeAngle"
              << i
              << " < 90.0) {\n"
                 "      float coneDot = dot(-L, lightDirectionVC"
              << i
              << ");\n"
                 "      // if inside the cone\n"
                 "      if (coneDot >= cos(radians(lightConeAngle"
              << i
              << ")))\n"
                 "      {\n"
                 "        attenuation = attenuation * pow(coneDot, lightExponent"
              << i
              << ");\n"
                 "      }\n"
                 "      else\n"
                 "      {\n"
                 "        attenuation = 0.0;\n"
                 "      }\n"
                 "    }\n"
                 "  }\n"
                 "  radiance = lightColor"
              << i << " * attenuation;\n";

          if (this->UseAnisotropy)
          {
            oss << "  specular = SpecularAnisotropic(at, ab, L, tangentVC, bitangentVC, H, "
                   "TdV, BdV, NdH, NdV, NdL, HdL, roughness, anisotropy, F0, F90, F);\n";
          }
          else
          {
            oss << "  specular = SpecularIsotropic(NdH, NdV, NdL, HdL, roughness, F0, F90, F);\n";
          }

          oss << "  diffuse = (1.0 - metallic) * (1.0 - F) * DiffuseLambert(albedo);\n";

          if (this->UseClearCoat)
          {
            oss << "  coatNdL = clamp(dot(coatN, L), 1e-5, 1.0);\n"
                   "  coatNdH = clamp(dot(coatN, H), 1e-5, 1.0);\n"
                   "  // Clear coat is isotropic\n"
                   "  coatLayer = SpecularIsotropic(coatNdH, coatNdV, coatNdL, HdL,"
                   " coatRoughness, coatF0, coatF90, Fc) * radiance * coatNdL * coatStrength;\n"
                   "  // Energy compensation depending on how much light is reflected by the "
                   "coat layer\n"
                   "  Fc *= coatStrength;\n"
                   "  specular *= (1.0 - Fc) * (1.0 - Fc);\n"
                   "  diffuse *= (1.0 - Fc);\n"
                   "  radiance *= coatColorFactor;\n"
                   "  Lo += coatLayer;\n";
          }

          oss << "  Lo += radiance * (diffuse + specular) * NdL;\n";
        }
        oss << "//VTK::Light::Impl\n";
      }
      else
      {
        oss << "  vec3 diffuse = vec3(0,0,0);\n"
               "  vec3 specular = vec3(0,0,0);\n"
               "  vec3 vertLightDirectionVC;\n"
               "  float attenuation;\n"
               "  float df;\n"
               "  float sf;\n";
        for (int i = 0; i < lastLightCount; ++i)
        {
          oss << "    attenuation = 1.0;\n"
                 "    if (lightPositional"
              << i
              << " == 0) {\n"
                 "      vertLightDirectionVC = lightDirectionVC"
              << i
              << "; }\n"
                 "    else {\n"
                 "      vertLightDirectionVC = vertexVC.xyz - lightPositionVC"
              << i
              << ";\n"
                 "      float distanceVC = length(vertLightDirectionVC);\n"
                 "      vertLightDirectionVC = normalize(vertLightDirectionVC);\n"
                 "      attenuation = 1.0 /\n"
                 "        (lightAttenuation"
              << i
              << ".x\n"
                 "         + lightAttenuation"
              << i
              << ".y * distanceVC\n"
                 "         + lightAttenuation"
              << i
              << ".z * distanceVC * distanceVC);\n"
                 "      // cone angle is less than 90 for a spot light\n"
                 "      if (lightConeAngle"
              << i
              << " < 90.0) {\n"
                 "        float coneDot = dot(vertLightDirectionVC, lightDirectionVC"
              << i
              << ");\n"
                 "        // if inside the cone\n"
                 "        if (coneDot >= cos(radians(lightConeAngle"
              << i
              << "))) {\n"
                 "          attenuation = attenuation * pow(coneDot, lightExponent"
              << i
              << "); }\n"
                 "        else {\n"
                 "          attenuation = 0.0; }\n"
                 "        }\n"
                 "      }\n"
              << "    df = max(0.0,attenuation*dot(normalizedNormalVCVSOutput, "
                 "-vertLightDirectionVC));\n"
                 // if you change the next line also change vtkShadowMapPass
                 "    diffuse += (df * lightColor"
              << i
              << ");\n"
                 "    sf = sign(df)*attenuation*pow( max(1e-5, dot( reflect(vertLightDirectionVC, "
                 "normalizedNormalVCVSOutput), normalize(-vertexVC.xyz))), power_specular);\n"
                 // if you change the next line also change vtkShadowMapPass
                 "      specular += (sf * lightColor"
              << i << ");\n";
        }
        oss << "  diffuse = diffuse * diffuseColor;\n"
               "  specular = specular * specularColor;\n"
               "  gl_FragData[0] = vec4(ambientColor + diffuse + specular, opacity);"
               "  //VTK::Light::Impl";
      }
      vtkShaderProgram::Substitute(fragmentShader, "//VTK::Light::Impl", oss.str(), false);
      break;
    default:
      break;
  }

  if (actor->GetProperty()->GetInterpolation() == VTK_PBR && lastLightComplexity > 0)
  {
    oss.str("");

    oss << "  // In IBL, we assume that v=n, so the amount of light reflected is\n"
           "  // the reflectance F0\n"
           "  vec3 specularBrdf = F0 * brdf.r + F90 * brdf.g;\n"
           "  vec3 iblSpecular = prefilteredSpecularColor * specularBrdf;\n"
           // no diffuse for metals
           "  vec3 iblDiffuse = (1.0 - F0) * (1.0 - metallic) * irradiance * albedo;\n"
           "  vec3 color = iblDiffuse + iblSpecular;\n"
           "\n";

    if (this->UseClearCoat)
    {
      oss << "  // Clear coat attenuation\n"
             "  Fc = F_Schlick(coatF0, coatF90, coatNdV) * coatStrength;\n"
             "  iblSpecular *= (1.0 - Fc);\n"
             "  iblDiffuse *= (1.0 - Fc) * (1.0 - Fc);\n"
             "  // Clear coat specular\n"
             "  vec3 iblSpecularClearCoat = prefilteredSpecularCoatColor * (coatF0 * coatBrdf.r + "
             "coatBrdf.g) * Fc;\n"
             // Color absorption by the coat layer
             "  color *= coatColorFactor;\n"
             "  color += iblSpecularClearCoat;\n"
             "\n";
    }

    oss << "  color += Lo;\n"
           "  color = mix(color, color * ao, aoStrengthUniform);\n" // ambient occlusion
           "  color += emissiveColor;\n"                            // emissive
           "  color = pow(color, vec3(1.0/2.2));\n"                 // to sRGB color space
           "  gl_FragData[0] = vec4(color, opacity);\n"
           "  //VTK::Light::Impl";

    vtkShaderProgram::Substitute(fragmentShader, "//VTK::Light::Impl", oss.str(), false);
  }

  // If rendering luminance values, write those values to the fragment
  if (info && info->Has(vtkLightingMapPass::RENDER_LUMINANCE()))
  {
    switch (this->LastLightComplexity)
    {
      case 0: // no lighting
        vtkShaderProgram::Substitute(fragmentShader, "//VTK::Light::Impl",
          "  gl_FragData[0] = vec4(0.0, 0.0, 0.0, 1.0);\n"
          "  //VTK::Light::Impl",
          false);
        break;
      case 1: // headlight
      case 2: // light kit
      case 3: // positional
        vtkShaderProgram::Substitute(fragmentShader, "//VTK::Light::Impl",
          "  float ambientY = dot(vec3(0.2126, 0.7152, 0.0722), ambientColor);\n"
          "  gl_FragData[0] = vec4(ambientY, diffuse.x, specular.x, 1.0);\n"
          "  //VTK::Light::Impl",
          false);
        break;
    }
  }
  return true;
}

//------------------------------------------------------------------------------
bool vtkGLSLModLight::SetShaderParameters(vtkOpenGLRenderer* renderer, vtkShaderProgram* program,
  vtkAbstractMapper* vtkNotUsed(mapper), vtkActor* actor,
  vtkOpenGLVertexArrayObject* vtkNotUsed(VAO) /*=nullptr*/)
{
  // for unlit there are no lighting parameters
  if (this->LastLightComplexity < 1)
  {
    return false;
  }

  vtkOpenGLRenderer* oglRen = vtkOpenGLRenderer::SafeDownCast(renderer);
  if (oglRen)
  {
    vtkFloatArray* sh = oglRen->GetSphericalHarmonics();

    if (oglRen->GetUseSphericalHarmonics() && sh)
    {
      std::string uniforms[3] = { "shRed", "shGreen", "shBlue" };
      for (int i = 0; i < 3; i++)
      {
        float coeffs[9];
        sh->GetTypedTuple(i, coeffs);

        // predivide with pi for Lambertian diffuse
        coeffs[0] *= 0.282095f;
        coeffs[1] *= -0.488603f * (2.f / 3.f);
        coeffs[2] *= 0.488603f * (2.f / 3.f);
        coeffs[3] *= -0.488603f * (2.f / 3.f);
        coeffs[4] *= 1.092548f * 0.25f;
        coeffs[5] *= -1.092548f * 0.25f;
        coeffs[6] *= 0.315392f * 0.25f;
        coeffs[7] *= -1.092548f * 0.25f;
        coeffs[8] *= 0.546274f * 0.25f;

        program->SetUniform1fv(uniforms[i].c_str(), 9, coeffs);
      }
    }
  }
  oglRen->UpdateLightingUniforms(program);
  // Add IBL textures
  if (oglRen->GetUseImageBasedLighting())
  {
    program->SetUniformi("brdfTex", oglRen->GetEnvMapLookupTable()->GetTextureUnit());
    program->SetUniformi("prefilterTex", oglRen->GetEnvMapPrefiltered()->GetTextureUnit());

    if (!oglRen->GetUseSphericalHarmonics())
    {
      program->SetUniformi("irradianceTex", oglRen->GetEnvMapIrradiance()->GetTextureUnit());
    }
  }
  // apply vtkProperty attributes
  // FIXME: Follow a consistent naming convention for shader uniforms.
  vtkProperty* ppty = actor->GetProperty();
  program->SetUniformf("intensity_opacity", ppty->GetOpacity());
  program->SetUniformf("intensity_ambient", ppty->GetAmbient());
  program->SetUniformf("intensity_diffuse", ppty->GetDiffuse());
  program->SetUniformf("intensity_specular", ppty->GetSpecular());
  program->SetUniform3f("color_ambient", ppty->GetAmbientColor());
  program->SetUniform3f("color_diffuse", ppty->GetDiffuseColor());
  program->SetUniform3f("color_specular", ppty->GetSpecularColor());
  program->SetUniformi("enable_specular", ppty->GetLighting());
  program->SetUniformf("power_specular", ppty->GetSpecularPower());
  if (auto bfPpty = actor->GetBackfaceProperty())
  {
    program->SetUniformf("intensity_opacity_bf", bfPpty->GetOpacity());
    program->SetUniformf("intensity_ambient_bf", bfPpty->GetAmbient());
    program->SetUniformf("intensity_diffuse_bf", bfPpty->GetDiffuse());
    program->SetUniformf("intensity_specular_bf", bfPpty->GetSpecular());
    program->SetUniform3f("color_ambient_bf", bfPpty->GetAmbientColor());
    program->SetUniform3f("color_diffuse_bf", bfPpty->GetDiffuseColor());
    program->SetUniform3f("color_specular_bf", bfPpty->GetSpecularColor());
    program->SetUniformi("enable_specular_bf", bfPpty->GetLighting());
    program->SetUniformf("power_specular_bf", bfPpty->GetSpecularPower());
  }

  program->SetUniformf("normalScaleUniform", static_cast<float>(ppty->GetNormalScale()));

  if (actor->GetProperty()->GetInterpolation() == VTK_PBR && this->LastLightComplexity > 0)
  {
    program->SetUniformf("metallicUniform", static_cast<float>(ppty->GetMetallic()));
    program->SetUniformf("roughnessUniform", static_cast<float>(ppty->GetRoughness()));
    program->SetUniformf("aoStrengthUniform", static_cast<float>(ppty->GetOcclusionStrength()));
    program->SetUniform3f("emissiveFactorUniform", ppty->GetEmissiveFactor());
    program->SetUniform3f("edgeTintUniform", ppty->GetEdgeTint());

    if (ppty->GetAnisotropy() > 0.0)
    {
      program->SetUniformf("anisotropyUniform", static_cast<float>(ppty->GetAnisotropy()));
      program->SetUniformf(
        "anisotropyRotationUniform", static_cast<float>(ppty->GetAnisotropyRotation()));
    }

    if (ppty->GetCoatStrength() > 0.0)
    {
      // Compute the reflectance of the coat layer and the exterior
      // Hard coded air environment (ior = 1.0)
      const double environmentIOR = 1.0;
      program->SetUniformf("coatF0Uniform",
        static_cast<float>(
          vtkProperty::ComputeReflectanceFromIOR(ppty->GetCoatIOR(), environmentIOR)));
      program->SetUniform3f("coatColorUniform", ppty->GetCoatColor());
      program->SetUniformf("coatStrengthUniform", static_cast<float>(ppty->GetCoatStrength()));
      program->SetUniformf("coatRoughnessUniform", static_cast<float>(ppty->GetCoatRoughness()));
      program->SetUniformf(
        "coatNormalScaleUniform", static_cast<float>(ppty->GetCoatNormalScale()));
    }
    // Compute the reflectance of the base layer
    program->SetUniformf(
      "baseF0Uniform", static_cast<float>(ppty->ComputeReflectanceOfBaseLayer()));
  }
  return true;
}

//------------------------------------------------------------------------------
bool vtkGLSLModLight::IsUpToDate(
  vtkOpenGLRenderer* renderer, vtkAbstractMapper* vtkNotUsed(mapper), vtkActor* actor)
{
  auto stats =
    vtkGLSLModLight::GetBasicLightStats(static_cast<vtkOpenGLRenderer*>(renderer), actor);
  if (this->LastLightComplexity != stats.Complexity || this->LastLightCount != stats.Count)
  {
    // lighting is not up to date.
    return false;
  }
  // all good
  return true;
}

VTK_ABI_NAMESPACE_END
