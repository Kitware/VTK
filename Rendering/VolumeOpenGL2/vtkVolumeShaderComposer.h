/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumeShaderComposer.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkVolumeShaderComposer_h
#define vtkVolumeShaderComposer_h

#include "vtkVolumeMask.h"

#include <vtkCamera.h>
#include <vtkOpenGLGPUVolumeRayCastMapper.h>
#include <vtkRenderer.h>
#include <vtkVolume.h>
#include <vtkVolumeMapper.h>
#include <vtkVolumeProperty.h>

#include <map>
#include <sstream>
#include <string>

// NOTE:
// In this code, we referred to various spaces described below:
// Object space: Raw coordinates in space defined by volume matrix
// Dataset space: Raw coordinates
// Eye space: Coordinates in eye space (as referred in computer graphics)

namespace vtkvolume
{
  //--------------------------------------------------------------------------
  std::string replace(std::string source, const std::string &search,
                      const std::string &replace, bool all)
  {
    if (replace.empty())
    {
      return source;
    }

    std::string::size_type pos = 0;
    bool first = true;
    while ((pos = source.find(search, 0)) != std::string::npos)
    {
      source.replace(pos, search.length(), replace);
      pos += search.length();
      if (first)
      {
        first = false;
        if (!all)
        {
          return source;
        }
      }
    }
    return source;
  }

  //--------------------------------------------------------------------------
  std::string ComputeClipPositionImplementation(vtkRenderer* vtkNotUsed(ren),
                                                vtkVolumeMapper* vtkNotUsed(mapper),
                                                vtkVolume* vtkNotUsed(vol))
  {
    return std::string("\
      \n  vec4 pos = in_projectionMatrix * in_modelViewMatrix *\
      \n             in_volumeMatrix * vec4(in_vertexPos.xyz, 1.0);\
      \n  gl_Position = pos;"
    );
  }

  //--------------------------------------------------------------------------
  std::string ComputeTextureCoordinates(vtkRenderer* vtkNotUsed(ren),
                                        vtkVolumeMapper* vtkNotUsed(mapper),
                                        vtkVolume* vtkNotUsed(vol))
  {
    return std::string(
      "\n  // For point dataset, we offset the texture coordinate\
       \n  // to account for OpenGL treating voxel at the center of the cell.\
       \n  vec3 uvx = sign(in_cellSpacing) * (in_vertexPos - in_volumeExtentsMin) /\
       \n               (in_volumeExtentsMax - in_volumeExtentsMin);\
       \n\
       \n  if (in_cellFlag)\
       \n    {\
       \n    ip_textureCoords = uvx;\
       \n    ip_inverseTextureDataAdjusted = in_inverseTextureDatasetMatrix;\
       \n    }\
       \n  else\
       \n    {\
       \n    // Transform cell tex-coordinates to point tex-coordinates\
       \n    ip_textureCoords = (in_cellToPoint * vec4(uvx, 1.0)).xyz;\
       \n    ip_inverseTextureDataAdjusted = in_cellToPoint * in_inverseTextureDatasetMatrix;\
       \n    }");
  }

  //--------------------------------------------------------------------------
  std::string BaseDeclarationVertex(vtkRenderer* vtkNotUsed(ren),
                                    vtkVolumeMapper* vtkNotUsed(mapper),
                                    vtkVolume* vtkNotUsed(vol))
  {
    return std::string("\
      \n  uniform bool in_cellFlag;\
      \n  uniform vec3 in_cellSpacing;\
      \n  uniform mat4 in_modelViewMatrix;\
      \n  uniform mat4 in_projectionMatrix;\
      \n  uniform mat4 in_volumeMatrix;\
      \n\
      \n  uniform vec3 in_volumeExtentsMin;\
      \n  uniform vec3 in_volumeExtentsMax;\
      \n\
      \n  uniform mat4 in_inverseTextureDatasetMatrix;\
      \n  uniform mat4 in_cellToPoint;\
      \n  uniform vec3 in_textureExtentsMax;\
      \n  uniform vec3 in_textureExtentsMin;\
      \n\
      \n  //This variable could be 'invariant varying' but it is declared\
      \n  //as 'varying' to avoid compiler compatibility issues.\
      \n  varying mat4 ip_inverseTextureDataAdjusted;");
  }

  //--------------------------------------------------------------------------
  std::string BaseDeclarationFragment(vtkRenderer* vtkNotUsed(ren),
                                      vtkVolumeMapper* mapper,
                                      vtkVolume* vtkNotUsed(vol),
                                      int vtkNotUsed(numberOfLights),
                                      int lightingComplexity,
                                      bool hasGradientOpacity,
                                      int noOfComponents,
                                      int independentComponents)
  {
    std::string shaderStr = std::string("\
      \n// Volume dataset\
      \nuniform sampler3D in_volume;\
      \nuniform int in_noOfComponents;\
      \nuniform int in_independentComponents;\
      \n\
      \nuniform sampler2D in_noiseSampler;\
      \n#ifndef GL_ES\
      \nuniform sampler2D in_depthSampler;\
      \n#endif\
      \n\
      \n// Camera position\
      \nuniform vec3 in_cameraPos;\
      \n\
      \n// view and model matrices\
      \nuniform mat4 in_volumeMatrix;\
      \nuniform mat4 in_inverseVolumeMatrix;\
      \nuniform mat4 in_projectionMatrix;\
      \nuniform mat4 in_inverseProjectionMatrix;\
      \nuniform mat4 in_modelViewMatrix;\
      \nuniform mat4 in_inverseModelViewMatrix;\
      \nuniform mat4 in_textureDatasetMatrix;\
      \nuniform mat4 in_inverseTextureDatasetMatrix;\
      \nvarying mat4 ip_inverseTextureDataAdjusted;\
      \nuniform vec3 in_texMin;\
      \nuniform vec3 in_texMax;\
      \nuniform mat4 in_texureToEyeIt;\
      \n\
      \n// Ray step size\
      \nuniform vec3 in_cellStep;\
      \nuniform vec2 in_scalarsRange[4];\
      \nuniform vec3 in_cellSpacing;\
      \n\
      \n// Sample distance\
      \nuniform float in_sampleDistance;\
      \n\
      \n// Scales\
      \nuniform vec3 in_cellScale;\
      \nuniform vec2 in_windowLowerLeftCorner;\
      \nuniform vec2 in_inverseOriginalWindowSize;\
      \nuniform vec2 in_inverseWindowSize;\
      \nuniform vec3 in_textureExtentsMax;\
      \nuniform vec3 in_textureExtentsMin;\
      \n\
      \n// Material and lighting\
      \nuniform vec3 in_diffuse[4];\
      \nuniform vec3 in_ambient[4];\
      \nuniform vec3 in_specular[4];\
      \nuniform float in_shininess[4];\
      \n\
      \n// Others\
      \nuniform bool in_cellFlag;\
      \n uniform bool in_useJittering;\
      \n uniform bool in_clampDepthToBackface;\
      \n\
      \nuniform vec2 in_averageIPRange;"
      );

    if (lightingComplexity > 0 || hasGradientOpacity)
    {
      shaderStr += std::string("\
        \nuniform bool in_twoSidedLighting;\
        \nvec3 g_xvec;\
        \nvec3 g_yvec;\
        \nvec3 g_zvec;");
    }

    if (hasGradientOpacity)
    {
      shaderStr += std::string("\
        \nvec3 g_aspect;\
        \nvec3 g_cellSpacing;\
        \nfloat g_avgSpacing;");
    }

    if (lightingComplexity == 3)
    {
      shaderStr += std::string("\
        \nvec4 g_fragWorldPos;\
        \nuniform int in_numberOfLights;\
        \nuniform vec3 in_lightAmbientColor[6];\
        \nuniform vec3 in_lightDiffuseColor[6];\
        \nuniform vec3 in_lightSpecularColor[6];\
        \nuniform vec3 in_lightDirection[6];\
        \nuniform vec3 in_lightPosition[6];\
        \nuniform vec3 in_lightAttenuation[6];\
        \nuniform float in_lightConeAngle[6];\
        \nuniform float in_lightExponent[6];\
        \nuniform int in_lightPositional[6];\
      ");
    }
    else if (lightingComplexity == 2)
    {
      shaderStr += std::string("\
        \nvec4 g_fragWorldPos;\
        \nuniform int in_numberOfLights;\
        \nuniform vec3 in_lightAmbientColor[6];\
        \nuniform vec3 in_lightDiffuseColor[6];\
        \nuniform vec3 in_lightSpecularColor[6];\
        \nuniform vec3 in_lightDirection[6];\
      ");
    }
    else
    {
      shaderStr += std::string("\
        \nuniform vec3 in_lightAmbientColor[1];\
        \nuniform vec3 in_lightDiffuseColor[1];\
        \nuniform vec3 in_lightSpecularColor[1];\
        \nvec4 g_lightPosObj;\
        \nvec3 g_ldir;\
        \nvec3 g_vdir;\
        \nvec3 g_h;");
    }

    if (noOfComponents > 1 && independentComponents)
    {
      shaderStr += std::string("\
        \nuniform vec4 in_componentWeight;");
    }

    vtkOpenGLGPUVolumeRayCastMapper* glMapper
      = vtkOpenGLGPUVolumeRayCastMapper::SafeDownCast(mapper);
    if (glMapper->GetCurrentPass() != vtkOpenGLGPUVolumeRayCastMapper::DepthPass &&
        glMapper->GetUseDepthPass())
    {
      shaderStr += std::string("\
        \nuniform sampler2D in_depthPassSampler;");
    }

    return shaderStr;
  }

  //--------------------------------------------------------------------------
  std::string BaseInit(vtkRenderer* vtkNotUsed(ren),
                       vtkVolumeMapper* mapper,
                       vtkVolume* vol,
                       int lightingComplexity)
  {
    vtkOpenGLGPUVolumeRayCastMapper* glMapper
      = vtkOpenGLGPUVolumeRayCastMapper::SafeDownCast(mapper);

    std::string shaderStr = std::string("\
      \n  bool l_adjustTextureExtents =  !in_cellFlag;"
    );

    if (glMapper->GetCurrentPass() != vtkOpenGLGPUVolumeRayCastMapper::DepthPass &&
        glMapper->GetUseDepthPass() && glMapper->GetBlendMode() ==
        vtkVolumeMapper::COMPOSITE_BLEND)
    {
      shaderStr += std::string("\
        \n  //\
        \n  vec2 fragTexCoord2 = (gl_FragCoord.xy - in_windowLowerLeftCorner) *\
        \n                        in_inverseWindowSize;\
        \n  vec4 depthValue = texture2D(in_depthPassSampler, fragTexCoord2);\
        \n  vec4 dataPos;\
        \n  dataPos.x = (gl_FragCoord.x - in_windowLowerLeftCorner.x) * 2.0 *\
        \n                     in_inverseWindowSize.x - 1.0;\
        \n  dataPos.y = (gl_FragCoord.y - in_windowLowerLeftCorner.y) * 2.0 *\
        \n                     in_inverseWindowSize.y - 1.0;\
        \n  dataPos.z = (2.0 * depthValue.x - (gl_DepthRange.near +\
        \n                     gl_DepthRange.far)) / gl_DepthRange.diff;\
        \n  dataPos.w = 1.0;\
        \n\
        \n  // From normalized device coordinates to eye coordinates.\
        \n  // in_projectionMatrix is inversed because of way VT\
        \n  // From eye coordinates to texture coordinates\
        \n  dataPos = in_inverseTextureDatasetMatrix *\
        \n            in_inverseVolumeMatrix *\
        \n            in_inverseModelViewMatrix *\
        \n            in_inverseProjectionMatrix *\
        \n            dataPos;\
        \n  dataPos /= dataPos.w;\
        \n  g_dataPos = dataPos.xyz;\
        \n  l_adjustTextureExtents = true;"
      );
    }
    else
    {
      shaderStr += std::string("\
        \n  // Get the 3D texture coordinates for lookup into the in_volume dataset\
        \n  g_dataPos = ip_textureCoords.xyz;"
      );
    }

      shaderStr += std::string("\
        \n\
        \n  // Eye position in dataset space\
        \n  g_eyePosObj = (in_inverseVolumeMatrix * vec4(in_cameraPos, 1.0));\
        \n  if (g_eyePosObj.w != 0.0)\
        \n    {\
        \n    g_eyePosObj.x /= g_eyePosObj.w;\
        \n    g_eyePosObj.y /= g_eyePosObj.w;\
        \n    g_eyePosObj.z /= g_eyePosObj.w;\
        \n    g_eyePosObj.w = 1.0;\
        \n    }\
        \n\
        \n  // Getting the ray marching direction (in dataset space);\
        \n  vec3 rayDir = computeRayDirection();\
        \n\
        \n  // Multiply the raymarching direction with the step size to get the\
        \n  // sub-step size we need to take at each raymarching step\
        \n  g_dirStep = (ip_inverseTextureDataAdjusted *\
        \n              vec4(rayDir, 0.0)).xyz * in_sampleDistance;\
        \n\
        \n  float jitterValue = (texture2D(in_noiseSampler, g_dataPos.xy).x);\
        \n  if (in_useJittering)\
        \n    {\
        \n    g_dataPos += g_dirStep * jitterValue;\
        \n    }\
        \n  else\
        \n    {\
        \n    g_dataPos += g_dirStep;\
        \n    }\
        \n\
        \n  // Flag to deternmine if voxel should be considered for the rendering\
        \n  bool l_skip = false;");

    if (vol->GetProperty()->GetShade() && lightingComplexity == 1)
    {
        shaderStr += std::string("\
          \n  // Light position in dataset space\
          \n  g_lightPosObj = (in_inverseVolumeMatrix *\
          \n                      vec4(in_cameraPos, 1.0));\
          \n  if (g_lightPosObj.w != 0.0)\
          \n    {\
          \n    g_lightPosObj.x /= g_lightPosObj.w;\
          \n    g_lightPosObj.y /= g_lightPosObj.w;\
          \n    g_lightPosObj.z /= g_lightPosObj.w;\
          \n    g_lightPosObj.w = 1.0;\
          \n    }\
          \n  g_ldir = normalize(g_lightPosObj.xyz - ip_vertexPos);\
          \n  g_vdir = normalize(g_eyePosObj.xyz - ip_vertexPos);\
          \n  g_h = normalize(g_ldir + g_vdir);"
        );
    }
    if ( (vol->GetProperty()->GetShade() ||
          vol->GetProperty()->HasGradientOpacity()) &&
         glMapper->GetCurrentPass() != vtkOpenGLGPUVolumeRayCastMapper::DepthPass)
    {
      shaderStr += std::string("\
        \n  g_xvec = vec3(in_cellStep[0], 0.0, 0.0);\
        \n  g_yvec = vec3(0.0, in_cellStep[1], 0.0);\
        \n  g_zvec = vec3(0.0, 0.0, in_cellStep[2]);"
      );
    }

    if (vol->GetProperty()->HasGradientOpacity())
    {
      shaderStr += std::string("\
        \n  g_cellSpacing = vec3(in_cellSpacing[0],\
        \n                       in_cellSpacing[1],\
        \n                       in_cellSpacing[2]);\
        \n  g_avgSpacing = (g_cellSpacing[0] +\
        \n                  g_cellSpacing[1] +\
        \n                  g_cellSpacing[2])/3.0;\
        \n  // Adjust the aspect\
        \n  g_aspect.x = g_cellSpacing[0] * 2.0 / g_avgSpacing;\
        \n  g_aspect.y = g_cellSpacing[1] * 2.0 / g_avgSpacing;\
        \n  g_aspect.z = g_cellSpacing[2] * 2.0 / g_avgSpacing;"
      );
    }

      return shaderStr;
  }

  //--------------------------------------------------------------------------
  std::string BaseImplementation(vtkRenderer* vtkNotUsed(ren),
                                 vtkVolumeMapper* vtkNotUsed(mapper),
                                 vtkVolume* vtkNotUsed(vol))
  {
    return std::string("\
      \n    l_skip = false;"
    );
  }

  //--------------------------------------------------------------------------
  std::string BaseExit(vtkRenderer* vtkNotUsed(ren),
                       vtkVolumeMapper* vtkNotUsed(mapper),
                       vtkVolume* vtkNotUsed(vol))
  {
    return std::string();
  }

  //--------------------------------------------------------------------------
  std::string ComputeGradientDeclaration(vtkRenderer* vtkNotUsed(ren),
                                         vtkVolumeMapper* vtkNotUsed(mapper),
                                         vtkVolume* vol,
                                         int noOfComponents,
                                         int independentComponents,
                                         std::map<int, std::string>
                                           gradientTableMap)
  {
    std::string shaderStr;
    if (vol->GetProperty()->HasGradientOpacity() &&
        (noOfComponents == 1 || !independentComponents))
    {
      shaderStr += std::string("\
        \nuniform sampler2D in_gradientTransferFunc;\
        \nfloat computeGradientOpacity(vec4 grad)\
        \n  {\
        \n  return texture2D("+gradientTableMap[0]+", vec2(grad.w, 0.0)).r;\
        \n  }"
      );
    }
    else if (noOfComponents > 1 && independentComponents &&
             vol->GetProperty()->HasGradientOpacity())
    {
      std::ostringstream toString;
      for (int i = 0; i < noOfComponents; ++i)
      {
        shaderStr += std::string("\n uniform sampler2D ") +
                     gradientTableMap[i] + std::string(";");
      }

      shaderStr += std::string("\
        \nfloat computeGradientOpacity(vec4 grad, int component)\
        \n  {");

      for (int i = 0; i < noOfComponents; ++i)
      {
        toString << i;
        shaderStr += std::string("\
          \n  if (component == " + toString.str() + ")");

        shaderStr += std::string("\
          \n    {\
          \n    return texture2D("+ gradientTableMap[i] + ", vec2(grad.w, 0.0)).r;\
          \n    }"
        );

        // Reset
        toString.str("");
        toString.clear();
      }

      shaderStr += std::string("\
        \n  }");
    }

    if (vol->GetProperty()->GetShade() &&
        !vol->GetProperty()->HasGradientOpacity())
    {
      shaderStr += std::string("\
        \n// c is short for component\
        \nvec4 computeGradient(int c)\
        \n  {\
        \n  vec3 g1;\
        \n  vec3 g2;\
        \n  g1.x = texture3D(in_volume, vec3(g_dataPos + g_xvec)).x;\
        \n  g1.y = texture3D(in_volume, vec3(g_dataPos + g_yvec)).x;\
        \n  g1.z = texture3D(in_volume, vec3(g_dataPos + g_zvec)).x;\
        \n  g2.x = texture3D(in_volume, vec3(g_dataPos - g_xvec)).x;\
        \n  g2.y = texture3D(in_volume, vec3(g_dataPos - g_yvec)).x;\
        \n  g2.z = texture3D(in_volume, vec3(g_dataPos - g_zvec)).x;\
        \n  g1 = g1 * in_volume_scale.r + in_volume_bias.r;\
        \n  g2 = g2 * in_volume_scale.r + in_volume_bias.r;\
        \n  return vec4((g1 - g2), -1.0);\
        \n  }"
      );
    }
    else if (vol->GetProperty()->HasGradientOpacity())
    {
      shaderStr += std::string("\
        \n// c is short for component\
        \nvec4 computeGradient(int c)\
        \n  {\
        \n  vec3 g1;\
        \n  vec4 g2;\
        \n  g1.x = texture3D(in_volume, vec3(g_dataPos + g_xvec)).x;\
        \n  g1.y = texture3D(in_volume, vec3(g_dataPos + g_yvec)).x;\
        \n  g1.z = texture3D(in_volume, vec3(g_dataPos + g_zvec)).x;\
        \n  g2.x = texture3D(in_volume, vec3(g_dataPos - g_xvec)).x;\
        \n  g2.y = texture3D(in_volume, vec3(g_dataPos - g_yvec)).x;\
        \n  g2.z = texture3D(in_volume, vec3(g_dataPos - g_zvec)).x;\
        \n  g1 = g1 * in_volume_scale.r + in_volume_bias.r;\
        \n  g2 = g2 * in_volume_scale.r + in_volume_bias.r;\
        \n  g1.x = in_scalarsRange[c][0] + (\
        \n         in_scalarsRange[c][1] - in_scalarsRange[c][0]) * g1.x;\
        \n  g1.y = in_scalarsRange[c][0] + (\
        \n         in_scalarsRange[c][1] - in_scalarsRange[c][0]) * g1.y;\
        \n  g1.z = in_scalarsRange[c][0] + (\
        \n         in_scalarsRange[c][1] - in_scalarsRange[c][0]) * g1.z;\
        \n  g2.x = in_scalarsRange[c][0] + (\
        \n         in_scalarsRange[c][1] - in_scalarsRange[c][0]) * g2.x;\
        \n  g2.y = in_scalarsRange[c][0] + (\
        \n         in_scalarsRange[c][1] - in_scalarsRange[c][0]) * g2.y;\
        \n  g2.z = in_scalarsRange[c][0] + (\
        \n         in_scalarsRange[c][1] - in_scalarsRange[c][0]) * g2.z;\
        \n  g2.xyz = g1 - g2.xyz;\
        \n  g2.x /= g_aspect.x;\
        \n  g2.y /= g_aspect.y;\
        \n  g2.z /= g_aspect.z;\
        \n  g2.w = 0.0;\
        \n  float grad_mag = length(g2);\
        \n  if (grad_mag > 0.0)\
        \n    {\
        \n    g2.x /= grad_mag;\
        \n    g2.y /= grad_mag;\
        \n    g2.z /= grad_mag;\
        \n    }\
        \n  else\
        \n    {\
        \n    g2.xyz = vec3(0.0, 0.0, 0.0);\
        \n    }\
        \n  grad_mag = grad_mag * 1.0 / (0.25 * (in_scalarsRange[c][1] -\
        \n                                      (in_scalarsRange[c][0])));\
        \n  grad_mag = clamp(grad_mag, 0.0, 1.0);\
        \n  g2.w = grad_mag;\
        \n  return g2;\
        \n  }"
      );
    }
    else
    {
      shaderStr += std::string("\
        \nvec4 computeGradient(int component)\
        \n  {\
        \n  return vec4(0.0);\
        \n  }");
    }

    return shaderStr;
  }

  //--------------------------------------------------------------------------
  std::string ComputeLightingDeclaration(vtkRenderer* vtkNotUsed(ren),
                                         vtkVolumeMapper* mapper,
                                         vtkVolume* vol,
                                         int noOfComponents,
                                         int independentComponents,
                                         int vtkNotUsed(numberOfLights),
                                         int lightingComplexity)
  {
    vtkVolumeProperty* volProperty = vol->GetProperty();
    std::string shaderStr = std::string("\
      \nvec4 computeLighting(vec4 color, int component)\
      \n  {\
      \n  vec4 finalColor = vec4(0.0);"
    );

    // Shading for composite blending only
    int shadeReqd = volProperty->GetShade() &&
                    (mapper->GetBlendMode() ==
                     vtkVolumeMapper::COMPOSITE_BLEND);

    if (shadeReqd || volProperty->HasGradientOpacity())
    {
      shaderStr += std::string("\
        \n  // Compute gradient function only once\
        \n  vec4 gradient = computeGradient(component);"
      );
    }

    if (shadeReqd)
    {
      if (lightingComplexity == 1)
      {
        shaderStr += std::string("\
          \n  vec3 diffuse = vec3(0.0);\
          \n  vec3 specular = vec3(0.0);\
          \n  vec3 normal = gradient.xyz / in_cellSpacing;\
          \n  float normalLength = length(normal);\
          \n  if (normalLength > 0.0)\
          \n    {\
          \n    normal = normalize(normal);\
          \n    }\
          \n  else\
          \n    {\
          \n    normal = vec3(0.0, 0.0, 0.0);\
          \n    }\
          \n   float nDotL = dot(normal, g_ldir);\
          \n   float nDotH = dot(normal, g_h);\
          \n   if (nDotL < 0.0 && in_twoSidedLighting)\
          \n     {\
          \n     nDotL = -nDotL;\
          \n     }\
          \n   if (nDotH < 0.0 && in_twoSidedLighting)\
          \n     {\
          \n     nDotH = -nDotH;\
          \n     }\
          \n   if (nDotL > 0.0)\
          \n     {\
          \n     diffuse = nDotL * in_diffuse[component] *\
          \n               in_lightDiffuseColor[0] * color.rgb;\
          \n     }\
          \n    specular = pow(nDotH, in_shininess[component]) *\
          \n                 in_specular[component] *\
          \n                 in_lightSpecularColor[0];\
          \n  // For the headlight, ignore the light's ambient color\
          \n  // for now as it is causing the old mapper tests to fail\
          \n  finalColor.xyz = in_ambient[component] * color.rgb +\
          \n                   diffuse + specular;"
          );
      }
      else if (lightingComplexity == 2)
      {
        shaderStr += std::string("\
          \n  g_fragWorldPos = in_modelViewMatrix * in_volumeMatrix *\
          \n                      in_textureDatasetMatrix * vec4(-g_dataPos, 1.0);\
          \n  if (g_fragWorldPos.w != 0.0)\
          \n   {\
          \n   g_fragWorldPos /= g_fragWorldPos.w;\
          \n   }\
          \n  vec3 vdir = normalize(g_fragWorldPos.xyz);\
          \n  vec3 normal = gradient.xyz;\
          \n  vec3 ambient = vec3(0.0);\
          \n  vec3 diffuse = vec3(0.0);\
          \n  vec3 specular = vec3(0.0);\
          \n  float normalLength = length(normal);\
          \n  if (normalLength > 0.0)\
          \n    {\
          \n    normal = normalize((in_texureToEyeIt * vec4(normal, 0.0)).xyz);\
          \n    }\
          \n  else\
          \n    {\
          \n    normal = vec3(0.0, 0.0, 0.0);\
          \n    }\
          \n  for (int lightNum = 0; lightNum < in_numberOfLights; lightNum++)\
          \n    {\
          \n    vec3 ldir = in_lightDirection[lightNum].xyz;\
          \n    vec3 h = normalize(ldir + vdir);\
          \n    float nDotH = dot(normal, h);\
          \n    if (nDotH < 0.0 && in_twoSidedLighting)\
          \n     {\
          \n     nDotH = -nDotH;\
          \n     }\
          \n  float nDotL = dot(normal, ldir);\
          \n  if (nDotL < 0.0 && in_twoSidedLighting)\
          \n    {\
          \n    nDotL = -nDotL;\
          \n    }\
          \n  if (nDotL > 0.0)\
          \n    {\
          \n    diffuse += in_lightDiffuseColor[lightNum] * nDotL;\
          \n    }\
          \n  if (nDotH > 0.0)\
          \n    {\
          \n    specular = in_lightSpecularColor[lightNum] *\
          \n               pow(nDotH, in_shininess[component]);\
          \n    }\
          \n  ambient += in_lightAmbientColor[lightNum];\
          \n  }\
          \n  finalColor.xyz = in_ambient[component] * ambient +\
          \n                   in_diffuse[component] * diffuse * color.rgb +\
          \n                   in_specular[component] * specular;"
          );
      }
      else if (lightingComplexity == 3)
      {
        shaderStr += std::string("\
          \n  g_fragWorldPos = in_modelViewMatrix * in_volumeMatrix *\
          \n                      in_textureDatasetMatrix * vec4(g_dataPos, 1.0);\
          \n  if (g_fragWorldPos.w != 0.0)\
          \n    {\
          \n    g_fragWorldPos /= g_fragWorldPos.w;\
          \n    }\
          \n  vec3 viewDirection = normalize(-g_fragWorldPos.xyz);\
          \n  vec3 ambient = vec3(0,0,0);\
          \n  vec3 diffuse = vec3(0,0,0);\
          \n  vec3 specular = vec3(0,0,0);\
          \n  vec3 vertLightDirection;\
          \n  vec3 normal = normalize((in_texureToEyeIt * vec4(gradient.xyz, 0.0)).xyz);\
          \n  vec3 lightDir;\
          \n  for (int lightNum = 0; lightNum < in_numberOfLights; lightNum++)\
          \n    {\
          \n    float attenuation = 1.0;\
          \n    // directional\
          \n    lightDir = in_lightDirection[lightNum];\
          \n    if (in_lightPositional[lightNum] == 0)\
          \n      {\
          \n      vertLightDirection = lightDir;\
          \n      }\
          \n    else\
          \n      {\
          \n      vertLightDirection = (g_fragWorldPos.xyz - in_lightPosition[lightNum]);\
          \n      float distance = length(vertLightDirection);\
          \n      vertLightDirection = normalize(vertLightDirection);\
          \n      attenuation = 1.0 /\
          \n                    (in_lightAttenuation[lightNum].x\
          \n                    + in_lightAttenuation[lightNum].y * distance\
          \n                    + in_lightAttenuation[lightNum].z * distance * distance);\
          \n      // per OpenGL standard cone angle is 90 or less for a spot light\
          \n      if (in_lightConeAngle[lightNum] <= 90.0)\
          \n        {\
          \n        float coneDot = dot(vertLightDirection, lightDir);\
          \n        // if inside the cone\
          \n        if (coneDot >= cos(radians(in_lightConeAngle[lightNum])))\
          \n          {\
          \n          attenuation = attenuation * pow(coneDot, in_lightExponent[lightNum]);\
          \n          }\
          \n        else\
          \n          {\
          \n          attenuation = 0.0;\
          \n          }\
          \n        }\
          \n      }\
          \n  // diffuse and specular lighting\
          \n  float nDotL = dot(normal, vertLightDirection);\
          \n  if (nDotL < 0.0 && in_twoSidedLighting)\
          \n    {\
          \n    nDotL = -nDotL;\
          \n    }\
          \n  if (nDotL > 0.0)\
          \n    {\
          \n    float df = max(0.0, attenuation * nDotL);\
          \n    diffuse += (df * in_lightDiffuseColor[lightNum]);\
          \n    }\
          \n  vec3 h = normalize(vertLightDirection + viewDirection);\
          \n  float nDotH = dot(normal, h);\
          \n  if (nDotH < 0.0 && in_twoSidedLighting)\
          \n    {\
          \n    nDotH = -nDotH;\
          \n    }\
          \n  if (nDotH > 0.0)\
          \n    {\
          \n    float sf = attenuation * pow(nDotH, in_shininess[component]);\
          \n    specular += (sf * in_lightSpecularColor[lightNum]);\
          \n    }\
          \n    ambient += in_lightAmbientColor[lightNum];\
          \n  }\
          \n  finalColor.xyz = in_ambient[component] * ambient +\
          \n                   in_diffuse[component] * diffuse * color.rgb +\
          \n                   in_specular[component] * specular;\
        ");
      }
    }
    else
    {
      shaderStr += std::string(
        "\n  finalColor = vec4(color.rgb, 0.0);"
      );
    }

    if (volProperty->HasGradientOpacity() &&
        (noOfComponents == 1 || !independentComponents))
    {
      shaderStr += std::string("\
        \n  if (gradient.w >= 0.0)\
        \n    {\
        \n    color.a = color.a *\
        \n              computeGradientOpacity(gradient);\
        \n    }"
      );
    }
     else if (noOfComponents > 1 && independentComponents &&
             volProperty->HasGradientOpacity())
     {
      shaderStr += std::string("\
      \n  if (gradient.w >= 0.0)\
      \n    {\
      \n    for (int i = 0; i < in_noOfComponents; ++i)\
      \n      {\
      \n      color.a = color.a *\
      \n      computeGradientOpacity(gradient, i) * in_componentWeight[i];\
      \n      }\
      \n    }"
      );
     }

    shaderStr += std::string("\
      \n  finalColor.a = color.a;\
      \n  return finalColor;\
      \n  }"
    );

    return shaderStr;
  }

  //--------------------------------------------------------------------------
  std::string ComputeRayDirectionDeclaration(vtkRenderer* ren,
                                             vtkVolumeMapper* vtkNotUsed(mapper),
                                             vtkVolume* vtkNotUsed(vol),
                                             int vtkNotUsed(noOfComponents))
  {
    if (!ren->GetActiveCamera()->GetParallelProjection())
    {
      return std::string("\
        \nvec3 computeRayDirection()\
        \n  {\
        \n  return normalize(ip_vertexPos.xyz - g_eyePosObj.xyz);\
        \n  }");
    }
    else
    {
      return std::string("\
        \nuniform vec3 in_projectionDirection;\
        \nvec3 computeRayDirection()\
        \n  {\
        \n  return normalize((in_inverseVolumeMatrix *\
        \n                   vec4(in_projectionDirection, 0.0)).xyz);\
        \n  }");
    }
  }

  //--------------------------------------------------------------------------
  std::string ComputeColorDeclaration(vtkRenderer* vtkNotUsed(ren),
                                      vtkVolumeMapper* vtkNotUsed(mapper),
                                      vtkVolume* vtkNotUsed(vol),
                                      int noOfComponents,
                                      int independentComponents,
                                      std::map<int, std::string> colorTableMap)
  {
      if (noOfComponents == 1)
      {
        return std::string("\
          \nuniform sampler2D in_colorTransferFunc;\
          \nvec4 computeColor(vec4 scalar, float opacity)\
          \n  {\
          \n  return computeLighting(vec4(texture2D(in_colorTransferFunc,\
          \n                         vec2(scalar.w, 0.0)).xyz, opacity), 0);\
          \n  }");
      }
      else if (noOfComponents > 1 && independentComponents)
      {
        std::string shaderStr;
        std::ostringstream toString;
        for (int i = 0; i < noOfComponents; ++i)
        {
          shaderStr += std::string("\n uniform sampler2D ") +
                       colorTableMap[i] + std::string(";");
        }

        shaderStr += std::string("\
          \nvec4 computeColor(vec4 scalar, float opacity, int component)\
          \n  {");

        for (int i = 0; i < noOfComponents; ++i)
        {
          toString << i;
          shaderStr += std::string("\
            \n  if (component == " + toString.str() + ")");

          shaderStr += std::string("\
            \n    {\
            \n    return computeLighting(vec4(texture2D(\
            \n      "+colorTableMap[i]);
          shaderStr += std::string(", vec2(\
            \n      scalar[" + toString.str() + "],0.0)).xyz,\
            \n      opacity),"+toString.str()+");\
            \n    }");

          // Reset
          toString.str("");
          toString.clear();
        }

          shaderStr += std::string("\n  }");
          return shaderStr;
      }
      else if (noOfComponents == 2 && !independentComponents)
      {
        return std::string("\
          \nuniform sampler2D in_colorTransferFunc;\
          \nvec4 computeColor(vec4 scalar, float opacity)\
          \n  {\
          \n  return computeLighting(vec4(texture2D(in_colorTransferFunc,\
          \n                                        vec2(scalar.x, 0.0)).xyz,\
          \n                              opacity), 0);\
          \n  }");
      }
      else
      {
        return std::string("\
          \nvec4 computeColor(vec4 scalar, float opacity)\
          \n  {\
          \n  return computeLighting(vec4(scalar.xyz, opacity), 0);\
          \n  }");
      }
  }

  //--------------------------------------------------------------------------
  std::string ComputeOpacityDeclaration(vtkRenderer* vtkNotUsed(ren),
                                        vtkVolumeMapper* vtkNotUsed(mapper),
                                        vtkVolume* vtkNotUsed(vol),
                                        int noOfComponents,
                                        int independentComponents,
                                        std::map<int, std::string> opacityTableMap)
  {
    if (noOfComponents > 1 && independentComponents)
    {
      std::string shaderStr;
      std::ostringstream toString;

      for (int i = 0; i < noOfComponents; ++i)
      {
        shaderStr += std::string("\n uniform sampler2D ") +
                     opacityTableMap[i] + std::string(";");

      }

        shaderStr += std::string("\
          \nfloat computeOpacity(vec4 scalar, int component)\
          \n  {");

        for (int i = 0; i < noOfComponents; ++i)
        {
          toString << i;
          shaderStr += std::string("\
            \n  if (component == " + toString.str() + ")");

          shaderStr += std::string("\
            \n    {\
            \n    return texture2D(in_opacityTransferFunc");
          shaderStr += (i == 0 ? "" : toString.str());
          shaderStr += std::string(",vec2(scalar[" + toString.str() + "],0)).r;\
            \n    }");

           // Reset
           toString.str("");
           toString.clear();
        }

        shaderStr += std::string("\n  }");
        return shaderStr;
    }
    else if (noOfComponents == 2 && !independentComponents)
    {
      return std::string("\
        \nuniform sampler2D in_opacityTransferFunc;\
        \nfloat computeOpacity(vec4 scalar)\
        \n  {\
        \n  return texture2D(in_opacityTransferFunc, vec2(scalar.y, 0)).r;\
        \n  }");
    }
    else
    {
      return std::string("\
        \nuniform sampler2D in_opacityTransferFunc;\
        \nfloat computeOpacity(vec4 scalar)\
        \n  {\
        \n  return texture2D(in_opacityTransferFunc, vec2(scalar.w, 0)).r;\
        \n  }");
    }
  }

  //--------------------------------------------------------------------------
  std::string ShadingDeclarationVertex(vtkRenderer* vtkNotUsed(ren),
                                       vtkVolumeMapper* vtkNotUsed(mapper),
                                       vtkVolume* vtkNotUsed(vol))
  {
    return std::string();
  }

  //--------------------------------------------------------------------------
  std::string ShadingDeclarationFragment(vtkRenderer* vtkNotUsed(ren),
                                         vtkVolumeMapper* vtkNotUsed(mapper),
                                         vtkVolume* vtkNotUsed(vol))
  {
    return std::string();
  }

  //--------------------------------------------------------------------------
  std::string ShadingInit(vtkRenderer* vtkNotUsed(ren),
                          vtkVolumeMapper* mapper,
                          vtkVolume* vtkNotUsed(vol))
  {
    if (mapper->GetBlendMode() == vtkVolumeMapper::MAXIMUM_INTENSITY_BLEND)
    {
      return std::string("\
        \n  // We get data between 0.0 - 1.0 range\
        \n  bool l_firstValue = true;\
        \n  vec4 l_maxValue = vec4(0.0);"
      );
    }
    else if (mapper->GetBlendMode() ==
             vtkVolumeMapper::MINIMUM_INTENSITY_BLEND)
    {
      return std::string("\
        \n  //We get data between 0.0 - 1.0 range\
        \n  bool l_firstValue = true;\
        \n  vec4 l_minValue = vec4(1.0);"
      );
    }
    else if (mapper->GetBlendMode() == vtkVolumeMapper::AVERAGE_INTENSITY_BLEND)
    {
      return std::string("\
        \n  //We get data between 0.0 - 1.0 range\
        \n  vec4 l_avgValue = vec4(0.0);\
        \n  // Keep track of number of samples\
        \n  uvec4 l_numSamples = uvec4(0);"
      );
    }
    else if (mapper->GetBlendMode() == vtkVolumeMapper::ADDITIVE_BLEND)
    {
      return std::string("\
        \n  //We get data between 0.0 - 1.0 range\
        \n  vec4 l_sumValue = vec4(0.0);"
      );
    }
    else
    {
      return std::string();
    }
  }

  //--------------------------------------------------------------------------
  std::string ShadingImplementation(vtkRenderer* vtkNotUsed(ren),
                                    vtkVolumeMapper* mapper,
                                    vtkVolume* vtkNotUsed(vol),
                                    vtkImageData* maskInput,
                                    vtkVolumeMask* mask, int maskType,
                                    int noOfComponents,
                                    int independentComponents = 0)
  {
    vtkOpenGLGPUVolumeRayCastMapper* glMapper =
      vtkOpenGLGPUVolumeRayCastMapper::SafeDownCast(mapper);
    std::string shaderStr = std::string("\
      \n    if (!l_skip)\
      \n      {\
      \n      vec4 scalar = texture3D(in_volume, g_dataPos);"
    );

    // simulate old intensity textures
    if (noOfComponents == 1)
    {
      shaderStr += std::string("\
        \n      scalar.r = scalar.r*in_volume_scale.r + in_volume_bias.r;\
        \n      scalar = vec4(scalar.r,scalar.r,scalar.r,scalar.r);"
        );
    }
    else
    {
      // handle bias and scale
      shaderStr += std::string("\
        \n      scalar = scalar*in_volume_scale + in_volume_bias;"
        );
    }

    if (mapper->GetBlendMode() == vtkVolumeMapper::MAXIMUM_INTENSITY_BLEND)
    {
      if (noOfComponents > 1)
      {
        if (!independentComponents)
        {
          shaderStr += std::string("\
            \n      if (l_maxValue.w < scalar.w || l_firstValue)\
            \n        {\
            \n        l_maxValue = scalar;\
            \n        }\
            \n\
            \n     if (l_firstValue)\
            \n        {\
            \n        l_firstValue = false;\
            \n        }"
          );
        }
        else
        {
          shaderStr += std::string("\
           \n      for (int i = 0; i < in_noOfComponents; ++i)\
           \n        {\
           \n        if (l_maxValue[i] < scalar[i] || l_firstValue)\
           \n          {\
           \n          l_maxValue[i] = scalar[i];\
           \n          }\
           \n        }\
           \n     if (l_firstValue)\
           \n        {\
           \n        l_firstValue = false;\
           \n        }"
          );
        }
      }
      else
      {
        shaderStr += std::string("\
          \n      if (l_maxValue.w < scalar.x || l_firstValue)\
          \n        {\
          \n        l_maxValue.w = scalar.x;\
          \n        }\
          \n\
          \n     if (l_firstValue)\
          \n        {\
          \n        l_firstValue = false;\
          \n        }"
        );
      }
    }
    else if (mapper->GetBlendMode() == vtkVolumeMapper::MINIMUM_INTENSITY_BLEND)
    {
      if (noOfComponents > 1)
      {
        if (!independentComponents)
        {
          shaderStr += std::string("\
            \n      if (l_minValue.w > scalar.w || l_firstValue)\
            \n        {\
            \n        l_minValue = scalar;\
            \n        }\
            \n\
            \n     if (l_firstValue)\
            \n        {\
            \n        l_firstValue = false;\
            \n        }"
          );
        }
        else
        {
          shaderStr += std::string("\
          \n      for (int i = 0; i < in_noOfComponents; ++i)\
          \n        {\
          \n        if (l_minValue[i] < scalar[i] || l_firstValue)\
          \n          {\
          \n          l_minValue[i] = scalar[i];\
          \n          }\
          \n        }\
          \n     if (l_firstValue)\
          \n        {\
          \n        l_firstValue = false;\
          \n        }"
          );
        }
      }
      else
      {
        shaderStr += std::string("\
          \n      if (l_minValue.w > scalar.x || l_firstValue)\
          \n        {\
          \n        l_minValue.w = scalar.x;\
          \n        }\
          \n\
          \n     if (l_firstValue)\
          \n        {\
          \n        l_firstValue = false;\
          \n        }"
        );
      }
    }
    else if (mapper->GetBlendMode() == vtkVolumeMapper::AVERAGE_INTENSITY_BLEND)
    {
      if (noOfComponents > 1  && independentComponents)
      {
        shaderStr += std::string("\
        \n       for (int i = 0; i < in_noOfComponents; ++i)\
        \n         {\
        \n         // Get the intensity in volume scalar range\
        \n         float intensity = in_scalarsRange[i][0] +\
        \n                           (in_scalarsRange[i][1] -\
        \n                            in_scalarsRange[i][0]) * scalar[i];\
        \n         if (in_averageIPRange.x <= intensity &&\
        \n             intensity <= in_averageIPRange.y)\
        \n           {\
        \n           float opacity = computeOpacity(scalar, i);\
        \n           l_avgValue[i] += scalar[i];\
        \n           ++l_numSamples[i];\
        \n           }\
        \n         }"
        );
      }
      else
      {
        shaderStr += std::string("\
        \n      // Get the intensity in volume scalar range\
        \n      float intensity = in_scalarsRange[0][0] +\
        \n                        (in_scalarsRange[0][1] -\
        \n                         in_scalarsRange[0][0]) * scalar.x;\
        \n      if (in_averageIPRange.x <= intensity &&\
        \n          intensity <= in_averageIPRange.y)\
        \n        {\
        \n        l_avgValue.w += scalar.x;\
        \n        ++l_numSamples.w;\
        \n        }"
        );
      }
    }
    else if (mapper->GetBlendMode() == vtkVolumeMapper::ADDITIVE_BLEND)
    {
      if (noOfComponents > 1)
      {
       if (!independentComponents)
       {
         shaderStr += std::string("\
           \n      float opacity = computeOpacity(scalar);\
           \n      l_sumValue.x = l_sumValue.x + opacity * scalar.x;"
         );
       }
       else
       {
         shaderStr += std::string("\
         \n       for (int i = 0; i < in_noOfComponents; ++i)\
         \n         {\
         \n         float opacity = computeOpacity(scalar, i);\
         \n         l_sumValue[i] = l_sumValue[i] + opacity * scalar[i];\
         \n         }"
         );
       }
      }
      else
      {
        shaderStr += std::string("\
          \n      float opacity = computeOpacity(scalar);\
          \n      l_sumValue.x = l_sumValue.x + opacity * scalar.x;"
        );
      }
    }
    else if (mapper->GetBlendMode() == vtkVolumeMapper::COMPOSITE_BLEND)
    {
      if (noOfComponents > 1 && independentComponents)
      {
        shaderStr += std::string("\
          \n      vec4 color[4]; vec4 tmp = vec4(0.0);\
          \n      float totalAlpha = 0.0;\
          \n      for (int i = 0; i < in_noOfComponents; ++i)\
          \n        {\
        ");
        if (glMapper->GetUseDepthPass() && glMapper->GetCurrentPass() ==
            vtkOpenGLGPUVolumeRayCastMapper::DepthPass)
        {
          shaderStr += std::string("\
            \n        // Data fetching from the red channel of volume texture\
            \n        float opacity = computeOpacity(scalar, i);\
            \n        if (opacity > 0.0)\
            \n          {\
            \n          g_srcColor.a = opacity;\
            \n          }\
            \n       }"
          );
        }
        else if (!mask || !maskInput ||
            maskType != vtkGPUVolumeRayCastMapper::LabelMapMaskType)
        {
          shaderStr += std::string("\
          \n        // Data fetching from the red channel of volume texture\
          \n        color[i][3] = computeOpacity(scalar, i);\
          \n        color[i] = computeColor(scalar, color[i][3], i);\
          \n        totalAlpha += color[i][3] * in_componentWeight[i];\
          \n        }\
          \n      if (totalAlpha > 0.0)\
          \n        {\
          \n        for (int i = 0; i < in_noOfComponents; ++i)\
          \n          {\
          \n          // Only let visible components contribute to the final color\
          \n          if (in_componentWeight[i] <= 0) continue;\
          \n\
          \n          tmp.x += color[i].x * color[i].w * in_componentWeight[i];\
          \n          tmp.y += color[i].y * color[i].w * in_componentWeight[i];\
          \n          tmp.z += color[i].z * color[i].w * in_componentWeight[i];\
          \n          tmp.w += ((color[i].w * color[i].w)/totalAlpha);\
          \n          }\
          \n        }\
          \n      g_fragColor = (1.0f - g_fragColor.a) * tmp + g_fragColor;"
          );
        }
      }
      else if (glMapper->GetUseDepthPass() && glMapper->GetCurrentPass() ==
               vtkOpenGLGPUVolumeRayCastMapper::DepthPass)
      {
        shaderStr += std::string("\
          \n      g_srcColor = vec4(0.0);\
          \n      g_srcColor.a = computeOpacity(scalar);"
        );
      }
      else
      {
         if (!mask || !maskInput ||
             maskType != vtkGPUVolumeRayCastMapper::LabelMapMaskType)
         {
           shaderStr += std::string("\
             \n      g_srcColor = vec4(0.0);\
             \n      g_srcColor.a = computeOpacity(scalar);\
             \n      if (g_srcColor.a > 0.0)\
             \n        {\
             \n        g_srcColor = computeColor(scalar, g_srcColor.a);"
           );
         }

         shaderStr += std::string("\
           \n        // Opacity calculation using compositing:\
           \n        // Here we use front to back compositing scheme whereby\
           \n        // the current sample value is multiplied to the\
           \n        // currently accumulated alpha and then this product\
           \n        // is subtracted from the sample value to get the\
           \n        // alpha from the previous steps. Next, this alpha is\
           \n        // multiplied with the current sample colour\
           \n        // and accumulated to the composited colour. The alpha\
           \n        // value from the previous steps is then accumulated\
           \n        // to the composited colour alpha.\
           \n        g_srcColor.rgb *= g_srcColor.a;\
           \n        g_fragColor = (1.0f - g_fragColor.a) * g_srcColor + g_fragColor;"
         );

         if (!mask || !maskInput ||
           maskType != vtkGPUVolumeRayCastMapper::LabelMapMaskType)
         {
           shaderStr += std::string("\
             \n        }"
           );
         }
      }
    }
     else
     {
        shaderStr += std::string();
     }

      shaderStr += std::string("\
        \n      }"
      );
      return shaderStr;
  }

  //--------------------------------------------------------------------------
  std::string PickingActorPassExit(vtkRenderer* vtkNotUsed(ren),
    vtkVolumeMapper* vtkNotUsed(mapper), vtkVolume* vtkNotUsed(vol))
  {
    return std::string("\
    \n  // Special coloring mode which renders the Prop Id in fragments that\
    \n  // have accumulated certain level of opacity. Used during the selection\
    \n  // pass vtkHardwareSelection::ACTOR_PASS.\
    \n  if (g_fragColor.a > 3.0/ 255.0)\
    \n    {\
    \n    gl_FragData[0] = vec4(in_propId, 1.0);\
    \n    }\
    \n  else\
    \n    {\
    \n    gl_FragData[0] = vec4(0.0);\
    \n    }\
    \n  return;");
  };

  //--------------------------------------------------------------------------
  std::string PickingIdLow24PassExit(vtkRenderer* vtkNotUsed(ren),
    vtkVolumeMapper* vtkNotUsed(mapper), vtkVolume* vtkNotUsed(vol))
  {
  return std::string("\
  \n  // Special coloring mode which renders the voxel index in fragments that\
  \n  // have accumulated certain level of opacity. Used during the selection\
  \n  // pass vtkHardwareSelection::ID_LOW24.\
  \n  if (g_fragColor.a > 3.0/ 255.0)\
  \n    {\
  \n    uvec3 volumeDim = uvec3(in_textureExtentsMax - in_textureExtentsMin);\
  \n    uvec3 voxelCoords = uvec3(volumeDim * g_dataPos);\
  \n    // vtkHardwareSelector assumes index 0 to be empty space, so add uint(1).\
  \n    uint idx = volumeDim.x * volumeDim.y * voxelCoords.z +\
  \n      volumeDim.x * voxelCoords.y + voxelCoords.x + uint(1);\
  \n    gl_FragData[0] = vec4(float(idx % uint(256)) / 255.0,\
  \n      float((idx / uint(256)) % uint(256)) / 255.0,\
  \n      float((idx / uint(65536)) % uint(256)) / 255.0, 1.0);\
  \n    }\
  \n  else\
  \n    {\
  \n    gl_FragData[0] = vec4(0.0);\
  \n    }\
  \n  return;");
  };

  //--------------------------------------------------------------------------
  std::string PickingIdMid24PassExit(vtkRenderer* vtkNotUsed(ren),
    vtkVolumeMapper* vtkNotUsed(mapper), vtkVolume* vtkNotUsed(vol))
  {
  return std::string("\
  \n  // Special coloring mode which renders the voxel index in fragments that\
  \n  // have accumulated certain level of opacity. Used during the selection\
  \n  // pass vtkHardwareSelection::ID_MID24.\
  \n  if (g_fragColor.a > 3.0/ 255.0)\
  \n    {\
  \n    uvec3 volumeDim = uvec3(in_textureExtentsMax - in_textureExtentsMin);\
  \n    uvec3 voxelCoords = uvec3(volumeDim * g_dataPos);\
  \n    // vtkHardwareSelector assumes index 0 to be empty space, so add uint(1).\
  \n    uint idx = volumeDim.x * volumeDim.y * voxelCoords.z +\
  \n      volumeDim.x * voxelCoords.y + voxelCoords.x + uint(1);\
  \n    idx = ((idx & 0xff000000) >> 24);\
  \n    gl_FragData[0] = vec4(float(idx % uint(256)) / 255.0,\
  \n      float((idx / uint(256)) % uint(256)) / 255.0,\
  \n      float(idx / uint(65536)) / 255.0, 1.0);\
  \n    }\
  \n  else\
  \n    {\
  \n    gl_FragData[0] = vec4(0.0);\
  \n    }\
  \n  return;");
  };

  //--------------------------------------------------------------------------
  std::string ShadingExit(vtkRenderer* vtkNotUsed(ren),
                          vtkVolumeMapper* mapper,
                          vtkVolume* vtkNotUsed(vol),
                          int noOfComponents,
                          int independentComponents = 0)
  {
    vtkOpenGLGPUVolumeRayCastMapper* glMapper =
      vtkOpenGLGPUVolumeRayCastMapper::SafeDownCast(mapper);

    if (glMapper->GetUseDepthPass() && glMapper->GetCurrentPass() ==
        vtkOpenGLGPUVolumeRayCastMapper::DepthPass &&
        mapper->GetBlendMode() == vtkVolumeMapper::COMPOSITE_BLEND)
    {
      return std::string();
    }
    else if (mapper->GetBlendMode() == vtkVolumeMapper::MAXIMUM_INTENSITY_BLEND)
    {
      if (noOfComponents > 1 && independentComponents)
      {
        return std::string("\
          \n   g_srcColor = vec4(0);\
          \n   for (int i = 0; i < in_noOfComponents; ++i)\
          \n     {\
          \n     vec4 tmp = computeColor(l_maxValue, computeOpacity(l_maxValue, i), i);\
          \n     g_srcColor[0] += tmp[0] * tmp[3] * in_componentWeight[i];\
          \n     g_srcColor[1] += tmp[1] * tmp[3] * in_componentWeight[i];\
          \n     g_srcColor[2] += tmp[2] * tmp[3] * in_componentWeight[i];\
          \n     g_srcColor[3] += tmp[3] * in_componentWeight[i];\
          \n     }\
          \n   g_fragColor = g_srcColor;"
        );
      }
      else
      {
        return std::string("\
         \n  g_srcColor = computeColor(l_maxValue,\
         \n                            computeOpacity(l_maxValue));\
         \n  g_fragColor.rgb = g_srcColor.rgb * g_srcColor.a;\
         \n  g_fragColor.a = g_srcColor.a;"
        );
      }
    }
    else if (mapper->GetBlendMode() == vtkVolumeMapper::MINIMUM_INTENSITY_BLEND)
    {
      if (noOfComponents > 1 && independentComponents)
      {
        return std::string("\
          \n  g_srcColor = vec4(0);\
          \n  for (int i = 0; i < in_noOfComponents; ++i)\
          \n    {\
          \n    vec4 tmp = computeColor(l_minValue, computeOpacity(l_minValue, i), i);\
          \n    g_srcColor[0] += tmp[0] * tmp[3] * in_componentWeight[i];\
          \n    g_srcColor[1] += tmp[1] * tmp[3] * in_componentWeight[i];\
          \n    g_srcColor[2] += tmp[2] * tmp[3] * in_componentWeight[i];\
          \n    g_srcColor[2] += tmp[3] * tmp[3] * in_componentWeight[i];\
          \n    }\
          \n  g_fragColor = g_srcColor;"
        );
      }
      else
      {
        return std::string("\
          \n  g_srcColor = computeColor(l_minValue,\
          \n                            computeOpacity(l_minValue));\
          \n  g_fragColor.rgb = g_srcColor.rgb * g_srcColor.a;\
          \n  g_fragColor.a = g_srcColor.a;"
        );
      }
    }
    else if (mapper->GetBlendMode() == vtkVolumeMapper::AVERAGE_INTENSITY_BLEND)
    {
      if (noOfComponents > 1 && independentComponents)
      {
        return std::string("\
          \n  g_srcColor = vec4(0);\
          \n  for (int i = 0; i < in_noOfComponents; ++i)\
          \n    {\
          \n    if (l_numSamples[i] == uint(0))\
          \n      {\
          \n      continue;\
          \n      }\
          \n    l_avgValue[i] /= l_numSamples[i];\
          \n    vec4 tmp = computeColor(l_avgValue,\
          \n                            computeOpacity(l_avgValue, i), i);\
          \n    g_srcColor[0] += tmp[0] * tmp[3] * in_componentWeight[i];\
          \n    g_srcColor[1] += tmp[1] * tmp[3] * in_componentWeight[i];\
          \n    g_srcColor[2] += tmp[2] * tmp[3] * in_componentWeight[i];\
          \n    g_srcColor[3] += tmp[3] * in_componentWeight[i];\
          \n    }\
          \n  g_fragColor = g_srcColor;"
        );
      }
      else
      {
        return std::string("\
         \n  if (l_numSamples.w == uint(0))\
         \n    {\
         \n    g_fragColor = vec4(0);\
         \n    }\
         \n  else\
         \n    {\
         \n    l_avgValue.w /= l_numSamples.w;\
         \n    g_srcColor = computeColor(l_avgValue,\
         \n                              computeOpacity(l_avgValue));\
         \n    g_fragColor.rgb = g_srcColor.rgb * g_srcColor.a;\
         \n    g_fragColor.a = g_srcColor.a;\
         \n    }"
        );
      }
    }
    else if (mapper->GetBlendMode() == vtkVolumeMapper::ADDITIVE_BLEND)
    {
      if (noOfComponents > 1 && independentComponents)
      {
        // Add all the components to get final color
        return std::string("\
          \n  l_sumValue.x *= in_componentWeight.x;\
          \n  for (int i = 1; i < in_noOfComponents; ++i)\
          \n    {\
          \n    l_sumValue.x += l_sumValue[i] * in_componentWeight[i];\
          \n    }\
          \n  l_sumValue.x = clamp(l_sumValue.x, 0.0, 1.0);\
          \n  g_fragColor = vec4(vec3(l_sumValue.x), 1.0);"
        );
      }
      else
      {
        return std::string("\
          \n  l_sumValue.x = clamp(l_sumValue.x, 0.0, 1.0);\
          \n  g_fragColor = vec4(vec3(l_sumValue.x), 1.0);"
        );
      }
    }
    else
    {
      return std::string();
    }
  }

  //--------------------------------------------------------------------------
  std::string TerminationDeclarationVertex(vtkRenderer* vtkNotUsed(ren),
                                           vtkVolumeMapper* vtkNotUsed(mapper),
                                           vtkVolume* vtkNotUsed(vol))
  {
    return std::string();
  }

  //--------------------------------------------------------------------------
  std::string TerminationDeclarationFragment(vtkRenderer* vtkNotUsed(ren),
                                             vtkVolumeMapper* vtkNotUsed(mapper),
                                             vtkVolume* vtkNotUsed(vol))
  {
    return std::string();
  }

  //--------------------------------------------------------------------------
  std::string PickingActorPassDeclaration(vtkRenderer* vtkNotUsed(ren),
    vtkVolumeMapper* vtkNotUsed(mapper), vtkVolume* vtkNotUsed(vol))
  {
  return std::string("\
  \n  uniform vec3 in_propId;");
  };

  //--------------------------------------------------------------------------
  std::string TerminationInit(vtkRenderer* vtkNotUsed(ren),
                              vtkVolumeMapper* vtkNotUsed(mapper),
                              vtkVolume* vtkNotUsed(vol))
  {
    return std::string("\
      \n  // Flag to indicate if the raymarch loop should terminate \
      \n  bool stop = false;\
      \n\
      \n  // 2D Texture fragment coordinates [0,1] from fragment coordinates \
      \n  // the frame buffer texture has the size of the plain buffer but \
      \n  // we use a fraction of it. The texture coordinates is less than 1 if \
      \n  // the reduction factor is less than 1. \
      \n  // Device coordinates are between -1 and 1. We need texture \
      \n  // coordinates between 0 and 1 the in_depthSampler buffer has the \
      \n  // original size buffer. \
      \n  vec2 fragTexCoord = (gl_FragCoord.xy - in_windowLowerLeftCorner) *\
      \n                      in_inverseWindowSize;\
      \n  float l_terminatePointMax = 0.0;\
      \n\
      \n#ifdef GL_ES\
      \n  vec4 l_depthValue = vec4(1.0,1.0,1.0,1.0);\
      \n#else\
      \n  vec4 l_depthValue = texture2D(in_depthSampler, fragTexCoord);\
      \n#endif\
      \n  // Depth test\
      \n  if(gl_FragCoord.z >= l_depthValue.x)\
      \n    {\
      \n    discard;\
      \n    }\
      \n\
      \n  // color buffer or max scalar buffer have a reduced size.\
      \n  fragTexCoord = (gl_FragCoord.xy - in_windowLowerLeftCorner) *\
      \n                 in_inverseOriginalWindowSize;\
      \n\
      \n  // Compute max number of iterations it will take before we hit\
      \n  // the termination point\
      \n\
      \n  // Abscissa of the point on the depth buffer along the ray.\
      \n  // point in texture coordinates\
      \n  vec4 terminatePoint;\
      \n  terminatePoint.x = (gl_FragCoord.x - in_windowLowerLeftCorner.x) * 2.0 *\
      \n                     in_inverseWindowSize.x - 1.0;\
      \n  terminatePoint.y = (gl_FragCoord.y - in_windowLowerLeftCorner.y) * 2.0 *\
      \n                     in_inverseWindowSize.y - 1.0;\
      \n  terminatePoint.z = (2.0 * l_depthValue.x - (gl_DepthRange.near +\
      \n                     gl_DepthRange.far)) / gl_DepthRange.diff;\
      \n  terminatePoint.w = 1.0;\
      \n\
      \n  // From normalized device coordinates to eye coordinates.\
      \n  // in_projectionMatrix is inversed because of way VT\
      \n  // From eye coordinates to texture coordinates\
      \n  terminatePoint = ip_inverseTextureDataAdjusted *\
      \n                   in_inverseVolumeMatrix *\
      \n                   in_inverseModelViewMatrix *\
      \n                   in_inverseProjectionMatrix *\
      \n                   terminatePoint;\
      \n  terminatePoint /= terminatePoint.w;\
      \n\
      \n  l_terminatePointMax = length(terminatePoint.xyz - g_dataPos.xyz) /\
      \n                        length(g_dirStep);\
      \n  float l_currentT = 0.0;");
  }

  //--------------------------------------------------------------------------
  std::string TerminationImplementation(vtkRenderer* vtkNotUsed(ren),
                                        vtkVolumeMapper* vtkNotUsed(mapper),
                                        vtkVolume* vtkNotUsed(vol))
  {
    return std::string("\
      \n    if(any(greaterThan(g_dataPos, in_texMax)) ||\
      \n      any(lessThan(g_dataPos, in_texMin)))\
      \n      {\
      \n      break;\
      \n      }\
      \n\
      \n    // Early ray termination\
      \n    // if the currently composited colour alpha is already fully saturated\
      \n    // we terminated the loop or if we have hit an obstacle in the\
      \n    // direction of they ray (using depth buffer) we terminate as well.\
      \n    if((g_fragColor.a > (1.0 - 1.0/255.0)) || \
      \n       l_currentT >= l_terminatePointMax)\
      \n      {\
      \n      break;\
      \n      }\
      \n    ++l_currentT;"
    );
  }

  //--------------------------------------------------------------------------
  std::string TerminationExit(vtkRenderer* vtkNotUsed(ren),
                              vtkVolumeMapper* vtkNotUsed(mapper),
                              vtkVolume* vtkNotUsed(vol))
  {
    return std::string();
  }

  //--------------------------------------------------------------------------
  std::string CroppingDeclarationVertex(vtkRenderer* vtkNotUsed(ren),
                                        vtkVolumeMapper* vtkNotUsed(mapper),
                                        vtkVolume* vtkNotUsed(vol))
  {
    return std::string();
  }

  //--------------------------------------------------------------------------
  std::string CroppingDeclarationFragment(vtkRenderer* vtkNotUsed(ren),
                                          vtkVolumeMapper* mapper,
                                          vtkVolume* vtkNotUsed(vol))
  {
    if (!mapper->GetCropping()) {
      return std::string();
    }

    return std::string("\
      \nuniform float in_croppingPlanes[6];\
      \nuniform int in_croppingFlags [32];\
      \n// X: axis = 0, Y: axis = 1, Z: axis = 2\
      \n// cp Cropping plane bounds (minX, maxX, minY, maxY, minZ, maxZ)\
      \nint computeRegionCoord(float cp[6], vec3 pos, int axis)\
      \n  {\
      \n  int cpmin = axis * 2;\
      \n  int cpmax = cpmin + 1;\
      \n\
      \n  if (pos[axis] < cp[cpmin])\
      \n    {\
      \n    return 1;\
      \n    }\
      \n  else if (pos[axis] >= cp[cpmin] &&\
      \n           pos[axis]  < cp[cpmax])\
      \n    {\
      \n    return 2;\
      \n    }\
      \n  else if (pos[axis] >= cp[cpmax])\
      \n    {\
      \n    return 3;\
      \n    }\
      \n  return 0;\
      \n  }\
      \n\
      \nint computeRegion(float cp[6], vec3 pos)\
      \n  {\
      \n  return (computeRegionCoord(cp, pos, 0) +\
      \n         (computeRegionCoord(cp, pos, 1) - 1) * 3 +\
      \n         (computeRegionCoord(cp, pos, 2) - 1) * 9);\
      \n  }"
    );
  }

  //--------------------------------------------------------------------------
  std::string CroppingInit(vtkRenderer* vtkNotUsed(ren),
                           vtkVolumeMapper* mapper,
                           vtkVolume* vtkNotUsed(vol))
  {
    if (!mapper->GetCropping()) {
      return std::string();
    }

    return std::string("\
      \n  // Convert cropping region to texture space\
      \n  float croppingPlanesTexture[6];\
      \n  mat4  datasetToTextureMat = in_inverseTextureDatasetMatrix;\
      \n\
      \n  vec4 temp = vec4(in_croppingPlanes[0], 0.0, 0.0, 1.0);\
      \n  temp = datasetToTextureMat * temp;\
      \n  if (temp[3] != 0.0)\
      \n   {\
      \n   temp[0] /= temp[3];\
      \n   }\
      \n  croppingPlanesTexture[0] = temp[0];\
      \n\
      \n  temp = vec4(in_croppingPlanes[1], 0.0, 0.0, 1.0);\
      \n  temp = datasetToTextureMat * temp;\
      \n  if (temp[3] != 0.0)\
      \n   {\
      \n   temp[0] /= temp[3];\
      \n   }\
      \n  croppingPlanesTexture[1] = temp[0];\
      \n\
      \n  temp = vec4(0.0, in_croppingPlanes[2], 0.0, 1.0);\
      \n  temp = datasetToTextureMat * temp;\
      \n  if (temp[3] != 0.0)\
      \n   {\
      \n   temp[1] /= temp[3];\
      \n   }\
      \n  croppingPlanesTexture[2] = temp[1];\
      \n\
      \n  temp = vec4(0.0, in_croppingPlanes[3], 0.0, 1.0);\
      \n  temp = datasetToTextureMat * temp;\
      \n  if (temp[3] != 0.0)\
      \n   {\
      \n   temp[1] /= temp[3];\
      \n   }\
      \n  croppingPlanesTexture[3] = temp[1];\
      \n\
      \n  temp = vec4(0.0, 0.0, in_croppingPlanes[4], 1.0);\
      \n  temp = datasetToTextureMat * temp;\
      \n  if (temp[3] != 0.0)\
      \n   {\
      \n   temp[2] /= temp[3];\
      \n   }\
      \n  croppingPlanesTexture[4] = temp[2];\
      \n\
      \n  temp = vec4(0.0, 0.0, in_croppingPlanes[5], 1.0);\
      \n  temp = datasetToTextureMat * temp;\
      \n  if (temp[3] != 0.0)\
      \n   {\
      \n   temp[2] /= temp[3];\
      \n   }\
      \n  croppingPlanesTexture[5] = temp[2];"
    );
  }

  //--------------------------------------------------------------------------
  std::string CroppingImplementation(vtkRenderer* vtkNotUsed(ren),
                                     vtkVolumeMapper* mapper,
                                     vtkVolume* vtkNotUsed(vol))
  {
    if (!mapper->GetCropping()) {
      return std::string();
    }

    return std::string("\
      \n    // Determine region\
      \n    int regionNo = computeRegion(croppingPlanesTexture, g_dataPos);\
      \n\
      \n    // Do & operation with cropping flags\
      \n    // Pass the flag that its Ok to sample or not to sample\
      \n    if (in_croppingFlags[regionNo] == 0)\
      \n      {\
      \n      // Skip this voxel\
      \n      l_skip = true;\
      \n      }"
    );
  }

  //--------------------------------------------------------------------------
  std::string CroppingExit(vtkRenderer* vtkNotUsed(ren),
                           vtkVolumeMapper* vtkNotUsed(mapper),
                           vtkVolume* vtkNotUsed(vol))
  {
    return std::string();
  }

  //--------------------------------------------------------------------------
  std::string ClippingDeclarationVertex(vtkRenderer* vtkNotUsed(ren),
                                        vtkVolumeMapper* vtkNotUsed(mapper),
                                        vtkVolume* vtkNotUsed(vol))
  {
    return std::string();
  }

  //--------------------------------------------------------------------------
  std::string ClippingDeclarationFragment(vtkRenderer* vtkNotUsed(ren),
                                          vtkVolumeMapper* vtkNotUsed(mapper),
                                          vtkVolume* vtkNotUsed(vol))
  {
    return std::string();
  }

  //--------------------------------------------------------------------------
  std::string ClippingInit(vtkRenderer* ren,
                           vtkVolumeMapper* mapper,
                           vtkVolume* vtkNotUsed(vol))
  {
    if (!mapper->GetClippingPlanes())
    {
      return std::string();
    }

    std::string shaderStr;
    if (!ren->GetActiveCamera()->GetParallelProjection())
    {
      shaderStr = std::string("\
        vec4 temp = in_volumeMatrix * vec4(rayDir, 0.0);\
        \n    if (temp.w != 0.0)\
        \n      {\
        \n      temp = temp/temp.w;\
        \n      temp.w = 1.0;\
        \n      }\
        vec3 objRayDir = temp.xyz;");
    }
    else
    {
      shaderStr = std::string("\
        vec3 objRayDir = normalize(in_projectionDirection);");
    }

    shaderStr += std::string("\
      \n  int clippingPlanesSize = int(in_clippingPlanes[0]);\
      \n  vec4 objDataPos = vec4(0.0);\
      \n  mat4 textureToObjMat = in_volumeMatrix *\
      \n                             in_textureDatasetMatrix;\
      \n\
      \n  vec4 terminatePointObj = textureToObjMat * terminatePoint;\
      \n  if (terminatePointObj.w != 0.0)\
      \n   {\
      \n   terminatePointObj = terminatePointObj/ terminatePointObj.w ;\
      \n   terminatePointObj.w = 1.0;\
      \n   }\
      \n\
      \n  for (int i = 0; i < clippingPlanesSize; i = i + 6)\
      \n    {\
      \n    if (in_useJittering)\
      \n      {\
      \n      objDataPos = textureToObjMat * vec4(g_dataPos - (g_dirStep\
      \n                                           * jitterValue), 1.0);\
      \n      }\
      \n    else\
      \n      {\
      \n      objDataPos = textureToObjMat * vec4(g_dataPos - g_dirStep, 1.0);\
      \n      }\
      \n    if (objDataPos.w != 0.0)\
      \n      {\
      \n      objDataPos = objDataPos/objDataPos.w; objDataPos.w = 1.0;\
      \n      }\
      \n    vec3 planeOrigin = vec3(in_clippingPlanes[i + 1],\
      \n                            in_clippingPlanes[i + 2],\
      \n                            in_clippingPlanes[i + 3]);\
      \n    vec3 planeNormal = vec3(in_clippingPlanes[i + 4],\
      \n                            in_clippingPlanes[i + 5],\
      \n                            in_clippingPlanes[i + 6]);\
      \n    vec3 normalizedPlaneNormal = normalize(planeNormal);\
      \n\
      \n    float rayDotNormal = dot(objRayDir, normalizedPlaneNormal);\
      \n    bool frontFace = rayDotNormal > 0;\
      \n    float distance = dot(normalizedPlaneNormal, planeOrigin - objDataPos.xyz);\
      \n\
      \n    if (frontFace && // Observing from the clipped side (plane's front face)\
      \n      distance > 0.0) // Ray-entry lies on the clipped side.\
      \n      {\
      \n      // Scale the point-plane distance to the ray direction and update the\
      \n      // entry point.\
      \n      float rayScaledDist = distance / rayDotNormal;\
      \n      vec4 newObjDataPos = vec4(objDataPos.xyz + rayScaledDist * objRayDir, 1.0);\
      \n      newObjDataPos = in_inverseTextureDatasetMatrix\
      \n        * in_inverseVolumeMatrix * vec4(newObjDataPos.xyz, 1.0);\
      \n      if (newObjDataPos.w != 0.0)\
      \n        {\
      \n        newObjDataPos /= newObjDataPos.w;\
      \n        }\
      \n      if (in_useJittering)\
      \n        {\
      \n        g_dataPos = newObjDataPos.xyz + g_dirStep * jitterValue;\
      \n        }\
      \n      else\
      \n        {\
      \n        g_dataPos = newObjDataPos.xyz + g_dirStep;\
      \n        }\
      \n\
      \n      bool stop = any(greaterThan(g_dataPos, in_texMax)) ||\
      \n        any(lessThan(g_dataPos, in_texMin));\
      \n      if (stop)\
      \n        {\
      \n        // The ray exits the bounding box before ever intersecting the plane (only\
      \n        // the clipped space is hit).\
      \n        discard;\
      \n        }\
      \n\
      \n      bool behindGeometry = dot(terminatePointObj.xyz - planeOrigin.xyz, normalizedPlaneNormal) < 0.0;\
      \n      if (behindGeometry)\
      \n        {\
      \n        // Geometry appears in front of the plane.\
      \n        discard;\
      \n        }\
      \n\
      \n      // Update the number of ray marching steps to account for the clipped entry point (\
      \n      // this is necessary in case the ray hits geometry after marching behind the plane,\
      \n      // given that the number of steps was assumed to be from the not-clipped entry).\
      \n      l_terminatePointMax = length(terminatePoint.xyz - g_dataPos.xyz) /\
      \n        length(g_dirStep);\
      \n      }\
      \n  }");

    return shaderStr;
  }

  //--------------------------------------------------------------------------
  std::string ClippingImplementation(vtkRenderer* vtkNotUsed(ren),
                                     vtkVolumeMapper* mapper,
                                     vtkVolume* vtkNotUsed(vol))
  {
    if (!mapper->GetClippingPlanes())
    {
      return std::string();
    }
    else
    {
      return std::string("\
        \n    for (int i = 0; i < clippingPlanesSize && !l_skip; i = i + 6)\
        \n      {\
        \n      vec4 objDataPos = textureToObjMat * vec4(g_dataPos, 1.0);\
        \n      if (objDataPos.w != 0.0)\
        \n        {\
        \n        objDataPos /= objDataPos.w;\
        \n        }\
        \n      vec3 planeOrigin = vec3(in_clippingPlanes[i + 1],\
        \n                              in_clippingPlanes[i + 2],\
        \n                              in_clippingPlanes[i + 3]);\
        \n      vec3 planeNormal = vec3(in_clippingPlanes[i + 4],\
        \n                              in_clippingPlanes[i + 5],\
        \n                              in_clippingPlanes[i + 6]);\
        \n      if (dot(vec3(objDataPos.xyz - planeOrigin), planeNormal) < 0 && dot(objRayDir, planeNormal) < 0)\
        \n        {\
        \n         l_skip = true;\
        \n         g_exit = true;\
        \n        }\
        \n      }"
      );
    }
  }

  //--------------------------------------------------------------------------
  std::string ClippingExit(vtkRenderer* vtkNotUsed(ren),
                           vtkVolumeMapper* vtkNotUsed(mapper),
                           vtkVolume* vtkNotUsed(vol))
  {
    return std::string();
  }

  //--------------------------------------------------------------------------
  std::string BinaryMaskDeclaration(vtkRenderer* vtkNotUsed(ren),
                                            vtkVolumeMapper* vtkNotUsed(mapper),
                                            vtkVolume* vtkNotUsed(vol),
                                            vtkImageData* maskInput,
                                            vtkVolumeMask* mask,
                                            int vtkNotUsed(maskType))
  {
    if (!mask || !maskInput)
    {
      return std::string();
    }
    else
    {
      return std::string("uniform sampler3D in_mask;");
    }
  }

  //--------------------------------------------------------------------------
  std::string BinaryMaskImplementation(vtkRenderer* vtkNotUsed(ren),
                                       vtkVolumeMapper* vtkNotUsed(mapper),
                                       vtkVolume* vtkNotUsed(vol),
                                       vtkImageData* maskInput,
                                       vtkVolumeMask* mask,
                                       int maskType)
  {
    if (!mask || !maskInput ||
        maskType == vtkGPUVolumeRayCastMapper::LabelMapMaskType)
    {
      return std::string();
    }
    else
    {
      return std::string("\
        \nvec4 maskValue = texture3D(in_mask, g_dataPos);\
        \nif(maskValue.r <= 0.0)\
        \n  {\
        \n  l_skip = true;\
        \n  }"
      );
    }
  }

  //--------------------------------------------------------------------------
  std::string CompositeMaskDeclarationFragment(vtkRenderer* vtkNotUsed(ren),
                                               vtkVolumeMapper* vtkNotUsed(mapper),
                                               vtkVolume* vtkNotUsed(vol),
                                               vtkImageData* maskInput,
                                               vtkVolumeMask* mask,
                                               int maskType)
  {
    if (!mask || !maskInput ||
        maskType != vtkGPUVolumeRayCastMapper::LabelMapMaskType)
    {
      return std::string();
    }
    else
    {
      return std::string("\
        \nuniform float in_maskBlendFactor;\
        \nuniform sampler2D in_mask1;\
        \nuniform sampler2D in_mask2;"
      );
    }
  }

  //--------------------------------------------------------------------------
  std::string CompositeMaskImplementation(vtkRenderer* vtkNotUsed(ren),
                                          vtkVolumeMapper* vtkNotUsed(mapper),
                                          vtkVolume* vtkNotUsed(vol),
                                          vtkImageData* maskInput,
                                          vtkVolumeMask* mask,
                                          int maskType,
                                          int noOfComponents)
  {
    if (!mask || !maskInput ||
        maskType != vtkGPUVolumeRayCastMapper::LabelMapMaskType)
    {
      return std::string();
    }
    else
    {
      std::string shaderStr = std::string("\
        \nvec4 scalar = texture3D(in_volume, g_dataPos);");

      // simulate old intensity textures
      if (noOfComponents == 1)
      {
        shaderStr += std::string("\
          \n      scalar.r = scalar.r*in_volume_scale.r + in_volume_bias.r;\
          \n      scalar = vec4(scalar.r,scalar.r,scalar.r,scalar.r);"
          );
      }
      else
      {
        // handle bias and scale
        shaderStr += std::string("\
          \n      scalar = scalar*in_volume_scale + in_volume_bias;"
          );
      }

      return shaderStr + std::string("\
        \nif (in_maskBlendFactor == 0.0)\
        \n  {\
        \n  g_srcColor = computeColor(scalar, computeOpacity(scalar));\
        \n  }\
        \nelse\
        \n  {\
        \n  float opacity = computeOpacity(scalar);\
        \n  // Get the mask value at this same location\
        \n  vec4 maskValue = texture3D(in_mask, g_dataPos);\
        \n  if(maskValue.r == 0.0)\
        \n    {\
        \n    g_srcColor = computeColor(scalar, opacity);\
        \n    }\
        \n  else\
        \n    {\
        \n    if (maskValue.r == 1.0/255.0)\
        \n      {\
        \n      g_srcColor = texture2D(in_mask1, vec2(scalar.w,0.0));\
        \n      }\
        \n    else\
        \n      {\
        \n      // maskValue.r == 2.0/255.0\
        \n      g_srcColor = texture2D(in_mask2, vec2(scalar.w,0.0));\
        \n      }\
        \n    g_srcColor.a = 1.0;\
        \n    if(in_maskBlendFactor < 1.0)\
        \n      {\
        \n      g_srcColor = (1.0 - in_maskBlendFactor) *\
        \n                    computeColor(scalar, opacity) +\
        \n                    in_maskBlendFactor * g_srcColor;\
        \n      }\
        \n    }\
        \n    g_srcColor.a = opacity;\
        \n  }"
      );
    }
  }

  //--------------------------------------------------------------------------
  std::string RenderToImageInit(vtkRenderer* vtkNotUsed(ren),
                                vtkVolumeMapper* vtkNotUsed(mapper),
                                vtkVolume* vtkNotUsed(vol))
  {
  return std::string("\
    \n  vec3 l_opaqueFragPos = vec3(-1.0);\
    \n  if(in_clampDepthToBackface)\
    \n    {\
    \n    l_opaqueFragPos = g_dataPos;\
    \n    }\
    \n  bool l_updateDepth = true;"
  );
  }

  //--------------------------------------------------------------------------
  std::string RenderToImageImplementation(
    vtkRenderer* vtkNotUsed(ren), vtkVolumeMapper* vtkNotUsed(mapper),
    vtkVolume* vtkNotUsed(vol))
  {
  return std::string("\
    \n    if(!l_skip && g_srcColor.a > 0.0 && l_updateDepth)\
    \n      {\
    \n      l_opaqueFragPos = g_dataPos;\
    \n      l_updateDepth = false;\
    \n      }"
  );
  }

  //--------------------------------------------------------------------------
  std::string RenderToImageExit(vtkRenderer* vtkNotUsed(ren),
                                vtkVolumeMapper* vtkNotUsed(mapper),
                                vtkVolume* vtkNotUsed(vol))
  {
  return std::string("\
    \n  if (l_opaqueFragPos == vec3(-1.0))\
    \n    {\
    \n    gl_FragData[1] = vec4(1.0);\
    \n    }\
    \n  else\
    \n    {\
    \n    vec4 depthValue = in_projectionMatrix * in_modelViewMatrix *\
    \n                      in_volumeMatrix * in_textureDatasetMatrix *\
    \n                      vec4(l_opaqueFragPos, 1.0);\
    \n    depthValue /= depthValue.w;\
    \n    gl_FragData[1] = vec4(vec3(0.5 * (gl_DepthRange.far -\
    \n                       gl_DepthRange.near) * depthValue.z + 0.5 *\
    \n                      (gl_DepthRange.far + gl_DepthRange.near)), 1.0);\
    \n    }"
  );
  }

  //--------------------------------------------------------------------------
  std::string DepthPassInit(vtkRenderer* vtkNotUsed(ren),
                                     vtkVolumeMapper* vtkNotUsed(mapper),
                                     vtkVolume* vtkNotUsed(vol))
  {
  return std::string("\
    \n  vec3 l_isoPos = g_dataPos;"
  );
  }

  //--------------------------------------------------------------------------
  std::string DepthPassImplementation(
    vtkRenderer* vtkNotUsed(ren), vtkVolumeMapper* vtkNotUsed(mapper),
    vtkVolume* vtkNotUsed(vol))
  {
  return std::string("\
    \n    if(!l_skip && g_srcColor.a > 0.0)\
    \n      {\
    \n      l_isoPos = g_dataPos;\
    \n      g_exit = true; l_skip = true;\
    \n      }"
  );
  }

  //--------------------------------------------------------------------------
  std::string DepthPassExit(vtkRenderer* vtkNotUsed(ren),
                                     vtkVolumeMapper* vtkNotUsed(mapper),
                                     vtkVolume* vtkNotUsed(vol))
  {
  return std::string("\
    \n  vec4 depthValue = in_projectionMatrix * in_modelViewMatrix *\
    \n                  in_volumeMatrix * in_textureDatasetMatrix *\
    \n                  vec4(l_isoPos, 1.0);\
    \n  gl_FragData[0] = vec4(l_isoPos, 1.0);\
    \n  gl_FragData[1] = vec4(vec3((depthValue.z/depthValue.w) * 0.5 + 0.5),\
    \n                        1.0);"
  );
  }
}

#endif // vtkVolumeShaderComposer_h
// VTK-HeaderTest-Exclude: vtkVolumeShaderComposer.h
