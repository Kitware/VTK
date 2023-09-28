// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkGLSLModLight.h"
#include "vtkActor.h"
#include "vtkCellGridMapper.h"
#include "vtkInformation.h"
#include "vtkInformationObjectBaseKey.h"
#include "vtkLightingMapPass.h"
#include "vtkLogger.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLRenderer.h"
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
bool vtkGLSLModLight::ReplaceShaderValues(vtkOpenGLRenderer* renderer,
  std::string& vtkNotUsed(vertexShader), std::string& /*geometryShader*/,
  std::string& fragmentShader, vtkAbstractMapper* vtkNotUsed(mapper), vtkActor* actor)
{
  // Generate code to handle different types of lights.
  auto info = actor->GetPropertyKeys();
  if (info && info->Has(vtkLightingMapPass::RENDER_NORMALS()))
  {
    vtkShaderProgram::Substitute(fragmentShader, "//VTK::Light::Impl",
      "  vec3 n = (vertexNormalVCVS + 1.0) * 0.5;\n"
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

  int LastlightComplexity = this->LastLightComplexity;
  int LastlightCount = this->LastLightCount;

  if (actor->GetProperty()->GetInterpolation() != VTK_PBR && LastlightCount == 0)
  {
    LastlightComplexity = 0;
  }

  vtkShaderProgram::Substitute(fragmentShader, "//VTK::Normal::Impl",
    "\n"
    "  vec4 vertexPositionVC = vertexPositionVCVS;\n"
    "  vec3 vertexNormalVC = normalize(vertexNormalVCVS);\n"
    "  if (gl_FrontFacing == false) vertexNormalVC = -vertexNormalVC;",
    false);

  std::ostringstream toString;
  // get standard lighting declarations.
  vtkShaderProgram::Substitute(
    fragmentShader, "//VTK::Light::Dec", renderer->GetLightingUniforms());
  switch (LastlightComplexity)
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
        vtkErrorMacro(<< "Headlights are not implemented for PBR interpolation");
        break;
      }
      else
      {
        toString << "float df = max(0.0f, vertexNormalVC.z);\n"
                    "  float sf = pow(df, specularPower);\n"
                    "  vec3 diffuse = df * diffuseColor * lightColor0;\n"
                    "  vec3 specular = sf * specularColor * lightColor0;\n"
                    "  gl_FragData[0] = vec4(ambientColor + diffuse + specular, opacity);\n"
                    "  //VTK::Light::Impl\n";
      }
      vtkShaderProgram::Substitute(fragmentShader, "//VTK::Light::Impl", toString.str(), false);
      break;
    case 2: // light kit
      // This is a hack to get at least a headlight in ParaView.
      // TODO: Handle all the lights in the light kit.

      if (actor->GetProperty()->GetInterpolation() == VTK_PBR)
      {
        // TODO: PBR
        vtkErrorMacro(<< "light kit is not implemented for PBR interpolation");
        break;
      }
      else
      {
        toString << "  vec3 diffuse = vec3(0,0,0);\n"
                    "  vec3 specular = vec3(0,0,0);\n"
                    "  float df;\n"
                    "  float sf;\n";
        for (int i = 0; i < LastlightCount; ++i)
        {
          toString << "    df = max(0.0, dot(vertexNormalVC, -lightDirectionVC" << i
                   << "));\n"
                      // if you change the next line also change vtkShadowMapPass
                      "  diffuse += (df * lightColor"
                   << i << ");\n"
                   << "  sf = sign(df)*pow(max(1e-5, dot( reflect(lightDirectionVC" << i
                   << ", vertexNormalVC), normalize(-vertexPositionVC.xyz))), specularPower);\n"
                      // if you change the next line also change vtkShadowMapPass
                      "  specular += (sf * lightColor"
                   << i << ");\n";
        }
        toString << "  diffuse = diffuse * diffuseColor;\n"
                    "  specular = specular * specularColor;\n"
                    "  gl_FragData[0] = vec4(ambientColor + diffuse + specular, opacity);"
                    "  //VTK::Light::Impl";
      }
      vtkShaderProgram::Substitute(fragmentShader, "//VTK::Light::Impl", toString.str(), false);

      break;
    case 3: // positional
      toString.clear();
      toString.str("");

      if (actor->GetProperty()->GetInterpolation() == VTK_PBR)
      {
        // TODO: PBR
        vtkErrorMacro(<< "positional light is not implemented for PBR interpolation");
        break;
      }
      else
      {
        toString << "  vec3 diffuse = vec3(0,0,0);\n"
                    "  vec3 specular = vec3(0,0,0);\n"
                    "  vec3 vertLightDirectionVC;\n"
                    "  float attenuation;\n"
                    "  float df;\n"
                    "  float sf;\n";
        for (int i = 0; i < LastlightCount; ++i)
        {
          toString
            << "    attenuation = 1.0;\n"
               "    if (lightPositional"
            << i
            << " == 0) {\n"
               "      vertLightDirectionVC = lightDirectionVC"
            << i
            << "; }\n"
               "    else {\n"
               "      vertLightDirectionVC = vertexPositionVC.xyz - lightPositionVC"
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
            << "    df = max(0.0,attenuation*dot(vertexNormalVC, -vertLightDirectionVC));\n"
               // if you change the next line also change vtkShadowMapPass
               "    diffuse += (df * lightColor"
            << i
            << ");\n"
               "    sf = sign(df)*attenuation*pow( max(1e-5, dot( reflect(vertLightDirectionVC, "
               "vertexNormalVC), normalize(-vertexPositionVC.xyz))), specularPower);\n"
               // if you change the next line also change vtkShadowMapPass
               "      specular += (sf * lightColor"
            << i << ");\n";
        }
        toString << "  diffuse = diffuse * diffuseColor;\n"
                    "  specular = specular * specularColor;\n"
                    "  gl_FragData[0] = vec4(ambientColor + diffuse + specular, opacity);"
                    "  //VTK::Light::Impl";
      }
      vtkShaderProgram::Substitute(fragmentShader, "//VTK::Light::Impl", toString.str(), false);
      break;
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
  if (actor->GetProperty()->GetLighting())
  {
    renderer->UpdateLightingUniforms(program);
  }

  // apply vtkProperty attributes
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
