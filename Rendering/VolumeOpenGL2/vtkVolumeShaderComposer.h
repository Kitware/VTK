// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef vtkVolumeShaderComposer_h
#define vtkVolumeShaderComposer_h
#include <vtkCamera.h>
#include <vtkImplicitFunction.h>
#include <vtkOpenGLGPUVolumeRayCastMapper.h>
#include <vtkRectilinearGrid.h>
#include <vtkRenderer.h>
#include <vtkUniformGrid.h>
#include <vtkVolume.h>
#include <vtkVolumeInputHelper.h>
#include <vtkVolumeMapper.h>
#include <vtkVolumeProperty.h>
#include <vtkVolumeTexture.h>

#include <map>
#include <sstream>
#include <string>

namespace
{
inline bool HasGradientOpacity(vtkOpenGLGPUVolumeRayCastMapper::VolumeInputMap& inputs)
{
  for (auto& item : inputs)
  {
    vtkVolumeProperty* volProp = item.second.Volume->GetProperty();
    const bool gradOp = (volProp->HasGradientOpacity() || volProp->HasLabelGradientOpacity()) &&
      !volProp->GetDisableGradientOpacity();
    if (gradOp)
      return true;
  }
  return false;
}

inline bool HasLighting(vtkOpenGLGPUVolumeRayCastMapper::VolumeInputMap& inputs)
{
  for (auto& item : inputs)
  {
    vtkVolumeProperty* volProp = item.second.Volume->GetProperty();
    const bool lighting = volProp->GetShade() == 1;
    if (lighting)
      return true;
  }
  return false;
}

inline bool UseClippedVoxelIntensity(vtkOpenGLGPUVolumeRayCastMapper::VolumeInputMap& inputs)
{
  for (auto& item : inputs)
  {
    vtkVolumeProperty* volProp = item.second.Volume->GetProperty();
    const bool useClippedVoxelIntensity = volProp->GetUseClippedVoxelIntensity() == 1;
    if (useClippedVoxelIntensity)
    {
      return true;
    }
  }
  return false;
}

inline std::string ArrayBaseName(const std::string& arrayName)
{
  return arrayName.substr(0, arrayName.length() - 3);
}
}

// NOTE:
// In this code, we referred to various spaces described below:
// Object space: Raw coordinates in space defined by volume matrix
// Dataset space: Raw coordinates
// Eye space: Coordinates in eye space (as referred in computer graphics)

namespace vtkvolume
{
VTK_ABI_NAMESPACE_BEGIN
//--------------------------------------------------------------------------
inline std::string ComputeClipPositionImplementation(
  vtkRenderer* vtkNotUsed(ren), vtkVolumeMapper* vtkNotUsed(mapper), vtkVolume* vtkNotUsed(vol))
{
  return std::string(
    "  //Transform vertex (data coordinates) to clip coordinates\n"
    "  // p_clip = T_ProjViewModel * T_dataToWorld * p_data\n"
    "  vec4 pos = in_projectionMatrix * in_modelViewMatrix * in_volumeMatrix[0] *\n"
    "    vec4(in_vertexPos.xyz, 1.0);\n"
    "  gl_Position = pos;\n");
}

//--------------------------------------------------------------------------
inline std::string ComputeTextureCoordinates(
  vtkRenderer* vtkNotUsed(ren), vtkVolumeMapper* vtkNotUsed(mapper), vtkVolume* vtkNotUsed(vol))
{
  return std::string(
    "  // Transform vertex (data coordinates) to texture coordinates.\n"
    "  // p_texture = T_dataToTex * p_data\n"
    "  vec3 uvx = sign(in_cellSpacing[0]) * (in_inverseTextureDatasetMatrix[0] *\n"
    "  vec4(in_vertexPos, 1.0)).xyz;\n"
    "\n"
    "  // For point dataset, we offset the texture coordinate\n"
    "  // to account for OpenGL treating voxel at the center of the cell.\n"
    "  // Transform cell tex-coordinates to point tex-coordinates (cellToPoint\n"
    "  // is an identity matrix in the case of cell data).\n"
    "  ip_textureCoords = (in_cellToPoint[0] * vec4(uvx, 1.0)).xyz;\n"
    "  ip_inverseTextureDataAdjusted = in_cellToPoint[0] * in_inverseTextureDatasetMatrix[0];\n");
}

//--------------------------------------------------------------------------
inline std::string BaseDeclarationVertex(vtkRenderer* vtkNotUsed(ren), vtkVolumeMapper* mapper,
  vtkVolume* vtkNotUsed(vol), bool multipleInputs)
{
  auto gpuMapper = vtkGPUVolumeRayCastMapper::SafeDownCast(mapper);
  const int numInputs = gpuMapper->GetInputCount();

  std::ostringstream ss;
  ss << "uniform vec3 in_cellSpacing[" << numInputs
     << "];\n"
        "uniform mat4 in_modelViewMatrix;\n"
        "uniform mat4 in_projectionMatrix;\n";

  const int numTransf = multipleInputs ? numInputs + 1 : 1;
  ss << "uniform mat4 in_volumeMatrix[" << numTransf
     << "];\n"
        "uniform mat4 in_inverseTextureDatasetMatrix["
     << numTransf
     << "];\n"
        "uniform mat4 in_cellToPoint["
     << numTransf
     << "];\n"
        "\n"
        "//This variable could be 'invariant varying' but it is declared\n"
        "//as 'varying' to avoid compiler compatibility issues.\n"
        "out mat4 ip_inverseTextureDataAdjusted;\n";

  return ss.str();
}

//--------------------------------------------------------------------------
inline std::string BaseDeclarationFragment(vtkRenderer* vtkNotUsed(ren), vtkVolumeMapper* mapper,
  vtkOpenGLGPUVolumeRayCastMapper::VolumeInputMap& inputs, int totalNumberOfLights,
  int numberPositionalLights, bool defaultLighting, int noOfComponents, int independentComponents)
{
  const int numInputs = static_cast<int>(inputs.size());

  std::ostringstream toShaderStr;
  toShaderStr << "uniform sampler3D in_volume[" << numInputs << "];\n";

  toShaderStr << "uniform vec4 in_volume_scale[" << numInputs
              << "];\n"
                 "uniform vec4 in_volume_bias["
              << numInputs << "];\n";

  if (vtkRectilinearGrid::SafeDownCast(mapper->GetInput()))
  {
    toShaderStr << "uniform sampler1D in_coordTexs;\n";
    toShaderStr << "uniform vec3 in_coordTexSizes;\n";
    toShaderStr << "uniform vec3 in_coordsScale;\n";
    toShaderStr << "uniform vec3 in_coordsBias;\n";
  }

  if (mapper->GetInput()->GetPointGhostArray() || mapper->GetInput()->GetCellGhostArray())
  {
    toShaderStr << "uniform sampler3D in_blanking;\n";
  }

  toShaderStr << "uniform int in_noOfComponents;\n"
                 "\n"
                 "uniform sampler2D in_depthSampler;\n";

  vtkOpenGLGPUVolumeRayCastMapper* glMapper = vtkOpenGLGPUVolumeRayCastMapper::SafeDownCast(mapper);
  if (glMapper->GetUseJittering())
  {
    toShaderStr << "uniform sampler2D in_noiseSampler;\n";
  }

  // For multiple inputs (numInputs > 1), an additional transformation is
  // needed for the bounding-box.
  const int numTransf = (numInputs > 1) ? numInputs + 1 : 1;
  toShaderStr << "uniform mat4 in_volumeMatrix[" << numTransf
              << "];\n"
                 "uniform mat4 in_inverseVolumeMatrix["
              << numTransf
              << "];\n"
                 "uniform mat4 in_textureDatasetMatrix["
              << numTransf
              << "];\n"
                 "uniform mat4 in_inverseTextureDatasetMatrix["
              << numTransf
              << "];\n"
                 "uniform mat4 in_textureToEye["
              << numTransf
              << "];\n"
                 "uniform vec3 in_texMin["
              << numTransf
              << "];\n"
                 "uniform vec3 in_texMax["
              << numTransf
              << "];\n"
                 "// Eye position in dataset space\n"
                 "uniform vec3 in_eyePosObjs["
              << numTransf
              << "];\n"
                 "uniform mat4 in_cellToPoint["
              << numTransf << "];\n";

  toShaderStr << "// view and model matrices\n"
                 "uniform mat4 in_projectionMatrix;\n"
                 "uniform mat4 in_inverseProjectionMatrix;\n"
                 "uniform mat4 in_modelViewMatrix;\n"
                 "uniform mat4 in_inverseModelViewMatrix;\n"
                 "in mat4 ip_inverseTextureDataAdjusted;\n"
                 "\n"
                 "// Ray step size\n"
                 "uniform vec3 in_cellStep["
              << numInputs << "];\n";

  if (glMapper->GetVolumetricScatteringBlending() > 0.0)
  {

    toShaderStr << "mat4 g_eyeToTexture = in_inverseTextureDatasetMatrix[0] *"
                   " in_inverseVolumeMatrix[0] * in_inverseModelViewMatrix;\n";
  }

  if (inputs[0].Volume->GetProperty() && inputs[0].Volume->GetProperty()->GetShade() &&
    !defaultLighting && totalNumberOfLights > 0)
  {
    toShaderStr << "mat4 g_texToView = in_modelViewMatrix * in_volumeMatrix[0] *"
                   "in_textureDatasetMatrix[0];\n";
  }

  toShaderStr << "uniform vec2 in_scalarsRange[" << numInputs * 4
              << "];\n"
                 "uniform vec3 in_cellSpacing["
              << numInputs
              << "];\n"
                 "\n"
                 "// Sample distance\n"
                 "uniform float in_sampleDistance;\n"
                 "\n"
                 "// Scales\n"
                 "uniform vec2 in_windowLowerLeftCorner;\n"
                 "uniform vec2 in_inverseOriginalWindowSize;\n"
                 "uniform vec2 in_inverseWindowSize;\n"
                 "uniform vec3 in_textureExtentsMax;\n"
                 "uniform vec3 in_textureExtentsMin;\n"
                 "\n"
                 "// Material and lighting\n"
                 "uniform vec3 in_diffuse[4];\n"
                 "uniform vec3 in_ambient[4];\n"
                 "uniform vec3 in_specular[4];\n"
                 "uniform float in_shininess[4];\n"
                 "\n"
                 "// Others\n"
                 "vec3 g_rayJitter = vec3(0.0);\n"
                 "\n"
                 "uniform vec2 in_averageIPRange;\n";

  const bool hasGradientOpacity = HasGradientOpacity(inputs);
  if (totalNumberOfLights > 0 || hasGradientOpacity)
  {
    toShaderStr << "uniform bool in_twoSidedLighting;\n";
  }

  if (glMapper->GetVolumetricScatteringBlending() > 0.0)
  {
    toShaderStr << R"***(
uniform float in_giReach;
uniform float in_anisotropy;
uniform float in_volumetricScatteringBlending;

)***";
  }

  if (totalNumberOfLights > 0)
  {
    std::string totalLights = std::to_string(totalNumberOfLights);
    std::string positionalLights = std::to_string(numberPositionalLights);

    if (!defaultLighting)
    {
      toShaderStr << "#define TOTAL_NUMBER_LIGHTS " << totalLights
                  << "\n"
                     "#define NUMBER_POS_LIGHTS "
                  << positionalLights
                  << "\n"
                     "vec4 g_fragWorldPos;\n"
                     "uniform vec3 in_lightAmbientColor[TOTAL_NUMBER_LIGHTS];\n"
                     "uniform vec3 in_lightDiffuseColor[TOTAL_NUMBER_LIGHTS];\n"
                     "uniform vec3 in_lightSpecularColor[TOTAL_NUMBER_LIGHTS];\n"
                     "uniform vec3 in_lightDirection[TOTAL_NUMBER_LIGHTS];\n";
      if (numberPositionalLights > 0)
      {
        toShaderStr << "uniform vec3 in_lightPosition[NUMBER_POS_LIGHTS];\n"
                       "uniform vec3 in_lightAttenuation[NUMBER_POS_LIGHTS];\n"
                       "uniform float in_lightConeAngle[NUMBER_POS_LIGHTS];\n"
                       "uniform float in_lightExponent[NUMBER_POS_LIGHTS];\n";
      }

      if (glMapper->GetVolumetricScatteringBlending() > 0.0)
      {
        toShaderStr << "vec3 g_lightDirectionTex[TOTAL_NUMBER_LIGHTS];\n";

        if (numberPositionalLights > 0)
        {
          toShaderStr << "vec3 g_lightPositionTex[NUMBER_POS_LIGHTS];\n";
        }
      }
    }
    else
    {
      toShaderStr << "uniform vec3 in_lightAmbientColor[1];\n"
                     "uniform vec3 in_lightDiffuseColor[1];\n"
                     "uniform vec3 in_lightSpecularColor[1];\n"
                     "vec4 g_lightPosObj["
                  << numInputs
                  << "];\n"
                     "vec3 g_ldir["
                  << numInputs
                  << "];\n"
                     "vec3 g_vdir["
                  << numInputs
                  << "];\n"
                     "vec3 g_h["
                  << numInputs << "];\n";
    }
  }

  if (noOfComponents > 1 && independentComponents)
  {
    toShaderStr << "uniform vec4 in_componentWeight;\n";
  }

  if (glMapper->GetCurrentPass() != vtkOpenGLGPUVolumeRayCastMapper::DepthPass &&
    glMapper->GetUseDepthPass())
  {
    toShaderStr << "uniform sampler2D in_depthPassSampler;\n";
  }

  if (glMapper->GetBlendMode() == vtkVolumeMapper::ISOSURFACE_BLEND)
  {
    toShaderStr << "#if NUMBER_OF_CONTOURS\n"
                   "uniform float in_isosurfacesValues[NUMBER_OF_CONTOURS];\n"
                   "\n"
                   "int findIsoSurfaceIndex(float scalar, float array[NUMBER_OF_CONTOURS+2])\n"
                   "{\n"
                   "  int index = NUMBER_OF_CONTOURS >> 1;\n"
                   "  while (scalar > array[index]) ++index;\n"
                   "  while (scalar < array[index]) --index;\n"
                   "  return index;\n"
                   "}\n"
                   "#endif\n";
  }
  else if (glMapper->GetBlendMode() == vtkVolumeMapper::SLICE_BLEND)
  {
    vtkVolume* vol = inputs.begin()->second.Volume;
    vtkImplicitFunction* func = vol->GetProperty()->GetSliceFunction();

    if (func && func->IsA("vtkPlane"))
    {
      toShaderStr
        << "uniform vec3 in_slicePlaneOrigin;\n"
           "uniform vec3 in_slicePlaneNormal;\n"
           "vec3 g_intersection;\n"
           "\n"
           "float intersectRayPlane(vec3 rayOrigin, vec3 rayDir)\n"
           "{\n"
           "  vec4 planeNormal = in_inverseVolumeMatrix[0] * vec4(in_slicePlaneNormal, 0.0);\n"
           "  float denom = dot(planeNormal.xyz, rayDir);\n"
           "  if (abs(denom) > 1e-6)\n"
           "  {\n"
           "    vec4 planeOrigin = in_inverseVolumeMatrix[0] * vec4(in_slicePlaneOrigin, 1.0);\n"
           "    return dot(planeOrigin.xyz - rayOrigin, planeNormal.xyz) / denom;\n"
           "  }\n"
           "  return -1.0;\n"
           "}\n";
    }
  }

  return toShaderStr.str();
}

//--------------------------------------------------------------------------
inline std::string BaseInit(vtkRenderer* vtkNotUsed(ren), vtkVolumeMapper* mapper,
  vtkOpenGLGPUVolumeRayCastMapper::VolumeInputMap& inputs, bool defaultLighting)
{
  vtkOpenGLGPUVolumeRayCastMapper* glMapper = vtkOpenGLGPUVolumeRayCastMapper::SafeDownCast(mapper);
  vtkVolume* vol = inputs.begin()->second.Volume;
  const int numInputs = static_cast<int>(inputs.size());

  std::ostringstream shaderStr;
  if (glMapper->GetCurrentPass() != vtkOpenGLGPUVolumeRayCastMapper::DepthPass &&
    glMapper->GetUseDepthPass() && glMapper->GetBlendMode() == vtkVolumeMapper::COMPOSITE_BLEND)
  {
    shaderStr << "\
        \n  //\
        \n  vec2 fragTexCoord2 = (gl_FragCoord.xy - in_windowLowerLeftCorner) *\
        \n                        in_inverseWindowSize;\
        \n  vec4 depthValue = texture2D(in_depthPassSampler, fragTexCoord2);\
        \n  vec4 rayOrigin = WindowToNDC(gl_FragCoord.x, gl_FragCoord.y, depthValue.x);\
        \n\
        \n  // From normalized device coordinates to eye coordinates.\
        \n  // in_projectionMatrix is inversed because of way VT\
        \n  // From eye coordinates to texture coordinates\
        \n  rayOrigin = in_inverseTextureDatasetMatrix[0] *\
        \n              in_inverseVolumeMatrix[0] *\
        \n              in_inverseModelViewMatrix *\
        \n              in_inverseProjectionMatrix *\
        \n              rayOrigin;\
        \n  rayOrigin /= rayOrigin.w;\
        \n  g_rayOrigin = rayOrigin.xyz;";
  }
  else
  {
    shaderStr << "\
        \n  // Get the 3D texture coordinates for lookup into the in_volume dataset\
        \n  g_rayOrigin = ip_textureCoords.xyz;";
  }

  shaderStr << "\n\
      \n  // Getting the ray marching direction (in dataset space)\
      \n  vec3 rayDir = computeRayDirection();\
      \n\
      \n  // 2D Texture fragment coordinates [0,1] from fragment coordinates.\
      \n  // The frame buffer texture has the size of the plain buffer but \
      \n  // we use a fraction of it. The texture coordinate is less than 1 if\
      \n  // the reduction factor is less than 1.\
      \n  // Device coordinates are between -1 and 1. We need texture\
      \n  // coordinates between 0 and 1. The in_depthSampler\
      \n  // buffer has the original size buffer.\
      \n  vec2 fragTexCoord = (gl_FragCoord.xy - in_windowLowerLeftCorner) *\
      \n                      in_inverseWindowSize;\
      \n\
      \n  // Multiply the raymarching direction with the step size to get the\
      \n  // sub-step size we need to take at each raymarching step\
      \n  g_dirStep = (ip_inverseTextureDataAdjusted *\
      \n              vec4(rayDir, 0.0)).xyz * in_sampleDistance;\
      \n  g_lengthStep = length(g_dirStep);\
      \n";

  shaderStr << "\
          \n float jitterValue = 0.0;\
          \n";

  if (glMapper->GetBlendMode() != vtkVolumeMapper::SLICE_BLEND)
  {
    // Intersection is computed with g_rayOrigin, so we should not modify it with Slice mode
    if (glMapper->GetUseJittering())
    {
      shaderStr << "\
          \n    jitterValue = texture2D(in_noiseSampler, gl_FragCoord.xy /\
                                              vec2(textureSize(in_noiseSampler, 0))).x;\
          \n    g_rayJitter = g_dirStep * jitterValue;\
          \n";
    }
    else
    {
      shaderStr << "\
        \n    g_rayJitter = g_dirStep;\
        \n";
    }
    shaderStr << "\
        \n  g_rayOrigin += g_rayJitter;\
        \n";
  }

  shaderStr << "\
      \n  // Flag to determine if voxel should be considered for the rendering\
      \n  g_skip = false;";

  if (vol->GetProperty()->GetShade() && defaultLighting)
  {
    shaderStr << "\
        \n  // Light position in dataset space";
    for (int i = 0; i < numInputs; ++i)
    {
      // In multi-volume case the first volume matrix is of the bounding box
      shaderStr << "\
        \n  g_lightPosObj["
                << i << "] = vec4(in_eyePosObjs[" << (numInputs > 1 ? i + 1 : i) << "], 1.0);\
        \n  g_ldir["
                << i << "] = normalize(g_lightPosObj[" << i << "].xyz - ip_vertexPos);\
        \n  g_vdir["
                << i << "] = normalize(in_eyePosObjs[" << i << "].xyz - ip_vertexPos);\
        \n  g_h["
                << i << "] = normalize(g_ldir[" << i << "] + g_vdir[" << i << "]);";
    }
  }

  return shaderStr.str();
}

//--------------------------------------------------------------------------
inline std::string BaseImplementation(
  vtkRenderer* vtkNotUsed(ren), vtkVolumeMapper* mapper, vtkVolume* vtkNotUsed(vol))
{
  vtkOpenGLGPUVolumeRayCastMapper* glMapper = vtkOpenGLGPUVolumeRayCastMapper::SafeDownCast(mapper);

  std::string str("\
      \n    g_skip = false;");

  // Blanking support
  vtkSmartPointer<vtkDataSet> dataSet = vtkDataSet::SafeDownCast(mapper->GetInput());
  bool blankCells = (dataSet->GetCellGhostArray() != nullptr);
  bool blankPoints = (dataSet->GetPointGhostArray() != nullptr);
  if (blankPoints || blankCells)
  {
    str += std::string("\
        \n     // Check whether the neighboring points/cells are blank.\
        \n     // Note the half cellStep because texels are point centered.\
        \n     vec3 xvec = vec3(in_cellStep[0].x/2.0, 0.0, 0.0);\
        \n     vec3 yvec = vec3(0.0, in_cellStep[0].y/2.0, 0.0);\
        \n     vec3 zvec = vec3(0.0, 0.0, in_cellStep[0].z/2.0);\
        \n     vec3 texPosPVec[3];\
        \n     texPosPVec[0] = g_dataPos + xvec;\
        \n     texPosPVec[1] = g_dataPos + yvec;\
        \n     texPosPVec[2] = g_dataPos + zvec;\
        \n     vec3 texPosNVec[3];\
        \n     texPosNVec[0] = g_dataPos - xvec;\
        \n     texPosNVec[1] = g_dataPos - yvec;\
        \n     texPosNVec[2] = g_dataPos - zvec;\
        \n     vec4 blankValue = texture3D(in_blanking, g_dataPos);\
        \n     vec4 blankValueXP = texture3D(in_blanking, texPosPVec[0]);\
        \n     vec4 blankValueYP = texture3D(in_blanking, texPosPVec[1]);\
        \n     vec4 blankValueZP = texture3D(in_blanking, texPosPVec[2]);\
        \n     vec4 blankValueXN = texture3D(in_blanking, texPosNVec[0]);\
        \n     vec4 blankValueYN = texture3D(in_blanking, texPosNVec[1]);\
        \n     vec4 blankValueZN = texture3D(in_blanking, texPosNVec[2]);\
        \n     vec3 blankValuePx;\
        \n     blankValuePx[0] = blankValueXP.x;\
        \n     blankValuePx[1] = blankValueYP.x;\
        \n     blankValuePx[2] = blankValueZP.x;\
        \n     vec3 blankValuePy;\
        \n     blankValuePy[0] = blankValueXP.y;\
        \n     blankValuePy[1] = blankValueYP.y;\
        \n     blankValuePy[2] = blankValueZP.y;\
        \n     vec3 blankValueNx;\
        \n     blankValueNx[0] = blankValueXN.x;\
        \n     blankValueNx[1] = blankValueYN.x;\
        \n     blankValueNx[2] = blankValueZN.x;\
        \n     vec3 blankValueNy;\
        \n     blankValueNy[0] = blankValueXN.y;\
        \n     blankValueNy[1] = blankValueYN.y;\
        \n     blankValueNy[2] = blankValueZN.y;\
        \n");
    if (blankPoints)
    {
      str += std::string("\
          \n    // If the current or neighboring points\
          \n    // (that belong to cells that share this texel) are blanked,\
          \n    // skip the texel. In other words, if point 1 were blank,\
          \n    // texels 0, 1 and 2 would have to be skipped.\
          \n    if (blankValue.x > 0.0 ||\
          \n        any(greaterThan(blankValueNx, vec3(0.0))) ||\
          \n        any(greaterThan(blankValuePx, vec3(0.0))))\
          \n      {\
          \n      // skip this texel\
          \n      g_skip = true;\
          \n      }\
          \n");
      if (blankCells)
      {
        str += std::string("\
            \n    // If the current or previous cells (that share this texel)\
            \n    // are blanked, skip the texel. In other words, if cell 1\
            \n    // is blanked, texels 1 and 2 would have to be skipped.\
            \n    else if (blankValue.y > 0.0 ||\
            \n             any(greaterThan(blankValuePy, vec3(0.0))) ||\
            \n             any(greaterThan(blankValueNy, vec3(0.0))))\
            \n      {\
            \n      // skip this texel\
            \n      g_skip = true;\
            \n      }\
            \n");
      }
    }
    else if (blankCells)
    {
      str += std::string("\
          \n    // If the current or previous cells (that share this texel)\
          \n    // are blanked, skip the texel. In other words, if cell 1\
          \n    // is blanked, texels 1 and 2 would have to be skipped.\
          \n    if (blankValue.x > 0.0 ||\
          \n        any(greaterThan(blankValueNx, vec3(0.0))) ||\
          \n        any(greaterThan(blankValuePx, vec3(0.0))))\
          \n      {\
          \n      // skip this texel\
          \n      g_skip = true;\
          \n      }\
          \n");
    }
  }

  if (glMapper->GetBlendMode() == vtkVolumeMapper::SLICE_BLEND)
  {
    str += std::string("\
        \n    g_dataPos = g_intersection;\
        \n");
  }

  return str;
}

//--------------------------------------------------------------------------
inline std::string BaseExit(
  vtkRenderer* vtkNotUsed(ren), vtkVolumeMapper* vtkNotUsed(mapper), vtkVolume* vtkNotUsed(vol))
{
  return std::string();
}

//--------------------------------------------------------------------------
inline std::string ComputeGradientOpacity1DDecl(vtkVolume* vol, int noOfComponents,
  int independentComponents, std::map<int, std::string> gradientTableMap)
{
  auto volProperty = vol->GetProperty();
  std::ostringstream ss;
  if (volProperty->HasGradientOpacity())
  {
    ss << "uniform sampler2D " << ArrayBaseName(gradientTableMap[0]) << "[" << noOfComponents
       << "];\n";
  }
  bool useLabelGradientOpacity =
    (volProperty->HasLabelGradientOpacity() && (noOfComponents == 1 || !independentComponents));
  if (useLabelGradientOpacity)
  {
    ss << "uniform sampler2D in_labelMapGradientOpacity;\n";
  }

  std::string shaderStr = ss.str();

  if (volProperty->HasGradientOpacity() && noOfComponents > 0)
  {
    if (noOfComponents == 1 || !independentComponents)
    {
      shaderStr += std::string("\
          \nfloat computeGradientOpacity(vec4 grad)\
          \n  {\
          \n  return texture2D(" +
        gradientTableMap[0] + ", vec2(grad.w, 0.0)).r;\
          \n  }");
    }
    else
    {
      shaderStr += std::string("\
        \nfloat computeGradientOpacity(vec4 grad, int component)\
        \n  {");

      for (int i = 0; i < noOfComponents; ++i)
      {
        std::ostringstream toString;
        toString << i;
        shaderStr += std::string("\
            \n  if (component == " +
          toString.str() + ")");

        shaderStr += std::string("\
            \n    {\
            \n    return texture2D(" +
          gradientTableMap[i] + ", vec2(grad.w, 0.0)).r;\
            \n    }");
      }

      shaderStr += std::string("\
          \n  }");
    }
  }

  if (useLabelGradientOpacity)
  {
    shaderStr += std::string("\
        \nfloat computeGradientOpacityForLabel(vec4 grad, float label)\
        \n  {\
        \n  return texture2D(in_labelMapGradientOpacity, vec2(grad.w, label)).r;\
        \n  }");
  }

  return shaderStr;
}

//--------------------------------------------------------------------------
inline std::string ComputeGradientDeclaration(
  vtkOpenGLGPUVolumeRayCastMapper* mapper, vtkOpenGLGPUVolumeRayCastMapper::VolumeInputMap& inputs)
{
  const bool hasLighting = HasLighting(inputs);
  const bool hasGradientOp = HasGradientOpacity(inputs);

  std::string shaderStr;
  if (hasLighting || hasGradientOp)
  {
    shaderStr += std::string(
      "// c is short for component\n"
      "vec4 computeGradient(in vec3 texPos, in int c, in sampler3D volume,in int index)\n"
      "{\n"
      "  // Approximate Nabla(F) derivatives with central differences.\n"
      "  vec3 g1; // F_front\n"
      "  vec3 g2; // F_back\n"
      "  vec3 xvec = vec3(in_cellStep[index].x, 0.0, 0.0);\n"
      "  vec3 yvec = vec3(0.0, in_cellStep[index].y, 0.0);\n"
      "  vec3 zvec = vec3(0.0, 0.0, in_cellStep[index].z);\n"
      "  vec3 texPosPvec[3];\n"
      "  texPosPvec[0] = texPos + xvec;\n"
      "  texPosPvec[1] = texPos + yvec;\n"
      "  texPosPvec[2] = texPos + zvec;\n"
      "  vec3 texPosNvec[3];\n"
      "  texPosNvec[0] = texPos - xvec;\n"
      "  texPosNvec[1] = texPos - yvec;\n"
      "  texPosNvec[2] = texPos - zvec;\n"
      "  g1.x = texture3D(volume, vec3(texPosPvec[0]))[c];\n"
      "  g1.y = texture3D(volume, vec3(texPosPvec[1]))[c];\n"
      "  g1.z = texture3D(volume, vec3(texPosPvec[2]))[c];\n"
      "  g2.x = texture3D(volume, vec3(texPosNvec[0]))[c];\n"
      "  g2.y = texture3D(volume, vec3(texPosNvec[1]))[c];\n"
      "  g2.z = texture3D(volume, vec3(texPosNvec[2]))[c];\n"
      "\n");
    if (UseClippedVoxelIntensity(inputs) && mapper->GetClippingPlanes())
    {
      shaderStr +=
        std::string("  vec4 g1ObjDataPos[3], g2ObjDataPos[3];\n"
                    "  for (int i = 0; i < 3; ++i)\n"
                    "  {\n"
                    "    g1ObjDataPos[i] = clip_texToObjMat * vec4(texPosPvec[i], 1.0);\n"
                    "    if (g1ObjDataPos[i].w != 0.0)\n"
                    "    {\n"
                    "      g1ObjDataPos[i] /= g1ObjDataPos[i].w;\n"
                    "    }\n"
                    "    g2ObjDataPos[i] = clip_texToObjMat * vec4(texPosNvec[i], 1.0);\n"
                    "    if (g2ObjDataPos[i].w != 0.0)\n"
                    "    {\n"
                    "      g2ObjDataPos[i] /= g2ObjDataPos[i].w;\n"
                    "    }\n"
                    "  }\n"
                    "\n"
                    "  for (int i = 0; i < clip_numPlanes && !g_skip; i = i + 6)\n"
                    "  {\n"
                    "    vec3 planeOrigin = vec3(in_clippingPlanes[i + 1],\n"
                    "                            in_clippingPlanes[i + 2],\n"
                    "                            in_clippingPlanes[i + 3]);\n"
                    "    vec3 planeNormal = normalize(vec3(in_clippingPlanes[i + 4],\n"
                    "                                      in_clippingPlanes[i + 5],\n"
                    "                                      in_clippingPlanes[i + 6]));\n"
                    "    for (int j = 0; j < 3; ++j)\n"
                    "    {\n"
                    "      if (dot(vec3(planeOrigin - g1ObjDataPos[j].xyz), planeNormal) > 0)\n"
                    "      {\n"
                    "        g1[j] = in_clippedVoxelIntensity;\n"
                    "      }\n"
                    "      if (dot(vec3(planeOrigin - g2ObjDataPos[j].xyz), planeNormal) > 0)\n"
                    "      {\n"
                    "        g2[j] = in_clippedVoxelIntensity;\n"
                    "      }\n"
                    "    }\n"
                    "  }\n"
                    "\n");
    }
    shaderStr += std::string("  // Apply scale and bias to the fetched values.\n"
                             "  g1 = g1 * in_volume_scale[index][c] + in_volume_bias[index][c];\n"
                             "  g2 = g2 * in_volume_scale[index][c] + in_volume_bias[index][c];\n"
                             "\n");
    if (!hasGradientOp)
    {
      shaderStr +=
        std::string("  // Central differences: (F_front - F_back) / 2h\n"
                    "  // This version of computeGradient() is only used for lighting\n"
                    "  // calculations (only direction matters), hence the difference is\n"
                    "  // not scaled by 2h and a dummy gradient mag is returned (-1.).\n"
                    "  return vec4((g1 - g2) / in_cellSpacing[index], -1.0);\n"
                    "}\n");
    }
    else
    {
      shaderStr += std::string(
        "  // Scale values the actual scalar range.\n"
        "  float range = in_scalarsRange[4*index+c][1] - in_scalarsRange[4*index+c][0];\n"
        "  g1 = in_scalarsRange[4*index+c][0] + range * g1;\n"
        "  g2 = in_scalarsRange[4*index+c][0] + range * g2;\n"
        "\n"
        "  // Central differences: (F_front - F_back) / 2h\n"
        "  g2 = g1 - g2;\n"
        "\n"
        "  float avgSpacing = (in_cellSpacing[index].x +\n"
        "   in_cellSpacing[index].y + in_cellSpacing[index].z) / 3.0;\n"
        "  vec3 aspect = in_cellSpacing[index] * 2.0 / avgSpacing;\n"
        "  g2 /= aspect;\n"
        "  float grad_mag = length(g2);\n"
        "\n"
        "  // Handle normalizing with grad_mag == 0.0\n"
        "  g2 = grad_mag > 0.0 ? normalize(g2) : vec3(0.0);\n"
        "\n"
        "  // Since the actual range of the gradient magnitude is unknown,\n"
        "  // assume it is in the range [0, 0.25 * dataRange].\n"
        "  range = range != 0 ? range : 1.0;\n"
        "  grad_mag = grad_mag / (0.25 * range);\n"
        "  grad_mag = clamp(grad_mag, 0.0, 1.0);\n"
        "\n"
        "  return vec4(g2.xyz, grad_mag);\n"
        "}\n");
    }
  }
  else
  {
    shaderStr += std::string(
      "vec4 computeGradient(in vec3 texPos, in int c, in sampler3D volume, in int index)\n"
      "{\n"
      "  return vec4(0.0);\n"
      "}\n");
  }

  return shaderStr;
}

//---------------------------------------------------------------------------
inline std::string ComputeMatricesInit(
  vtkOpenGLGPUVolumeRayCastMapper* vtkNotUsed(mapper), int numberPositionalLights)
{
  std::string resStr;
  resStr += R"***(
  for(int i=0; i<TOTAL_NUMBER_LIGHTS; i++)
  {
    g_lightDirectionTex[i] = (g_eyeToTexture * vec4(-in_lightDirection[i], 0.0)).xyz;
  }
  )***";

  if (numberPositionalLights > 0)
  {
    resStr += R"***(
  for(int i=0; i<NUMBER_POS_LIGHTS; i++)
  {
    g_lightPositionTex[i] = (g_eyeToTexture * vec4(in_lightPosition[i], 1.0)).xyz;
  }
    )***";
  }
  return resStr;
}

//--------------------------------------------------------------------------
inline std::string ComputeRGBA2DWithGradientDeclaration(vtkRenderer* vtkNotUsed(ren),
  vtkVolumeMapper* vtkNotUsed(mapper), vtkVolume* vtkNotUsed(vol), int noOfComponents,
  int independentComponents, std::map<int, std::string> opacityTableMap, int useGradient)
{
  std::string resStr;
  std::string functionBody;
  bool severalIndpt = noOfComponents > 1 && independentComponents;
  std::string functionSignature = severalIndpt
    ? "vec4 computeRGBAWithGrad(vec4 scalar, vec4 grad, int component)\n"
    : "vec4 computeRGBAWithGrad(vec4 scalar, vec4 grad)\n";

  if (severalIndpt)
  {
    // Multiple independent components

    if (!useGradient)
    {
      functionBody +=
        "vec4 yscalar = texture3D(in_transfer2DYAxis, g_dataPos);\n"
        "for (int i = 0; i < 4; ++i)\n"
        "{\n"
        "  yscalar[i] = yscalar[i] * in_transfer2DYAxis_scale[i] + in_transfer2DYAxis_bias[i];\n"
        "}\n";
    }

    for (int i = 0; i < noOfComponents; ++i)
    {
      std::string secondAxis(useGradient
          // we take the same grad for all components so we have to be sure that
          //  the one given as a parameter is computed wrt the right component
          ? "grad.w"
          : std::string("yscalar[") + std::to_string(i) + "]");

      functionBody += "  if(component == " + std::to_string(i) +
        ")\n"
        "  {\n"
        "    return texture2D(" +
        opacityTableMap[i] + ",\n" + "    vec2(scalar[" + std::to_string(i) + "], " + secondAxis +
        "))\n" + "  }\n";
    }
  }

  else if (noOfComponents == 2 && !independentComponents)
  {
    std::string secondAxis(useGradient ? "grad.w" : "yscalar.y");

    functionBody += "  return texture2D(" + opacityTableMap[0] +
      ",\n"
      "    vec2(scalar.y, " +
      secondAxis + "));\n";
  }

  else
  {
    if (useGradient)
    {
      // Dependent components (RGBA) || Single component
      functionBody += "  return texture2D(" + opacityTableMap[0] +
        ",\n"
        "    vec2(scalar.a, grad.w));\n";
    }
    else
    {
      // Dependent compoennts (RGBA) || Single component
      functionBody +=
        "  vec4 yscalar = texture3D(in_transfer2DYAxis, g_dataPos);\n"
        "  yscalar.r = yscalar.r * in_transfer2DYAxis_scale.r + in_transfer2DYAxis_bias.r;\n"
        "  yscalar = vec4(yscalar.r);\n"
        "  return texture2D(" +
        opacityTableMap[0] +
        ",\n"
        "    vec2(scalar.a, yscalar.w));\n";
    }
  }

  resStr = functionSignature + "{\n" + functionBody + "}\n";

  return resStr;
}

//-----------------------------------------------------------------------
inline std::string ComputeOpacityEvaluationCall(vtkOpenGLGPUVolumeRayCastMapper* vtkNotUsed(mapper),
  vtkOpenGLGPUVolumeRayCastMapper::VolumeInputMap& inputs, int noOfComponents,
  int independentComponents, int useGradYAxis, std::string position, bool requestColor = false)
{
  // relies on the declaration of variables opacity, gradient, c, volume, index, scalar, gradTF,
  // opacityTF, label in the scope
  std::string resStr;

  if (inputs.size() > 1)
  {
    // Multi Volume
    const bool hasGradOp = ::HasGradientOpacity(inputs);
    resStr += "  opacity = computeOpacity(vec4(scalar), opacityTF);\n";
    // either all volumes have a TF either none have one, so we can have
    // the same opacity call for all volumes
    if (hasGradOp)
    {
      resStr += std::string("  gradient = computeGradient(") + position + ", c, volume, index);\n";
      resStr += "  opacity *= computeGradientOpacity(gradient, gradTF);\n";
    }
    // ignore request color for now, but given the actual architecture, it should be a
    // succession of 'if' comparing the volume idx
    if (requestColor)
    {
      vtkGenericWarningMacro(<< "ComputeOpacityEvaluationCall was called with requestColor, but "
                                "MultiVolume does not support this option yet.");
    }
  }
  else
  {
    // Single Volume
    vtkVolumeProperty* volProp = inputs[0].Volume->GetProperty();
    const bool hasGradOp = volProp->HasGradientOpacity() && !volProp->GetDisableGradientOpacity();
    const bool useLabelGradientOpacity = (volProp->HasLabelGradientOpacity() &&
      (noOfComponents == 1 || !independentComponents) && !volProp->GetDisableGradientOpacity());

    const int tfMode = volProp->GetTransferFunctionMode();

    bool indpComps = (noOfComponents > 1 && independentComponents);
    std::string compArgument = (indpComps) ? std::string(", c") : std::string();

    const bool needGrad = (tfMode == vtkVolumeProperty::TF_2D && useGradYAxis); // to be sure

    if (tfMode == vtkVolumeProperty::TF_1D)
    {

      std::string compWeights = indpComps ? std::string(" * in_componentWeight[c]") : std::string();

      resStr += std::string("  opacity = computeOpacity(vec4(scalar)") + compArgument +
        std::string(")") + compWeights + ";\n";

      if (hasGradOp || useLabelGradientOpacity)
      {
        resStr += std::string("  gradient = computeGradient(") + position +
          std::string(", c, volume, index);\n"
                      "  if(gradient.w >= 0.0) {\n") +
          (hasGradOp ? (std::string("    opacity *= computeGradientOpacity(gradient") +
                         compArgument + ")" + compWeights + ";\n")
                     : std::string())

          + (useLabelGradientOpacity
                ? (std::string("    opacity *= computeGradientOpacityForLabel(gradient, label);\n"))
                : std::string())

          + std::string("  }\n");
      }

      if (requestColor)
      {
        resStr +=
          "    color = texture2D(" + inputs[0].RGBTablesMap[0] + ", vec2(scalar, 0.0)).xyz;\n";
      }
    }
    else
    {
      // 2D TF
      if (needGrad)
      {
        resStr +=
          std::string("  gradient = computeGradient(") + position + ", c, volume, index);\n";
      }
      resStr += std::string("  vec4 lutRes = computeRGBAWithGrad(vec4(scalar), gradient") +
        compArgument + std::string(");\n");

      resStr += "    opacity = lutRes.a;\n";

      if (requestColor)
      {
        resStr += "    color = lutRes.xyz;\n";
      }
    }
  }

  return resStr;
}

//--------------------------------------------------------------------------
inline std::string ComputeDensityGradientDeclaration(vtkOpenGLGPUVolumeRayCastMapper* mapper,
  vtkOpenGLGPUVolumeRayCastMapper::VolumeInputMap& inputs, int noOfComponents,
  int independentComponents, int useGradYAxis)
{
  const bool hasLighting = ::HasLighting(inputs);
  const bool hasGradientOp = ::HasGradientOpacity(inputs);

  std::string functionSignature;

  if (inputs.size() > 1)
  {
    if (hasGradientOp)
    {
      functionSignature = std::string(
        "vec4 computeDensityGradient(in vec3 texPos, in int c, in sampler3D volume, "
        "const in sampler2D opacityTF, const in sampler2D gradTF, in int index, float label)\n");
    }
    else
    {
      functionSignature =
        std::string("vec4 computeDensityGradient(in vec3 texPos, in int c, in sampler3D volume, "
                    "const in sampler2D opacityTF, in int index, float label)\n");
    }
  }
  else
  {
    functionSignature = std::string("vec4 computeDensityGradient(in vec3 texPos, in int c, in "
                                    "sampler3D volume, in int index, float label)\n");
  }

  std::string shaderStr;
  if (hasLighting || hasGradientOp)
  {

    std::string opacityTFcall;
    std::string gradComput;
    // this table remembers the correspondence results <-> texture coordinates
    static const std::array<std::pair<const char*, const char*>, 6> results_texPos = { {
      { "  g1.x", "texPosPvec[0]" },
      { "  g1.y", "texPosPvec[1]" },
      { "  g1.z", "texPosPvec[2]" },
      { "  g2.x", "texPosNvec[0]" },
      { "  g2.y", "texPosNvec[1]" },
      { "  g2.z", "texPosNvec[2]" },
    } };

    shaderStr += std::string("// c is short for component\n") + functionSignature +
      std::string("{\n"
                  "  // Approximate Nabla(F) derivatives with central differences.\n"
                  "  vec3 g1; // F_front\n"
                  "  vec3 g2; // F_back\n"
                  "  vec3 xvec = vec3(in_cellStep[index].x, 0.0, 0.0);\n"
                  "  vec3 yvec = vec3(0.0, in_cellStep[index].y, 0.0);\n"
                  "  vec3 zvec = vec3(0.0, 0.0, in_cellStep[index].z);\n"
                  "  vec3 texPosPvec[3];\n"
                  "  texPosPvec[0] = texPos + xvec;\n"
                  "  texPosPvec[1] = texPos + yvec;\n"
                  "  texPosPvec[2] = texPos + zvec;\n"
                  "  vec3 texPosNvec[3];\n"
                  "  texPosNvec[0] = texPos - xvec;\n"
                  "  texPosNvec[1] = texPos - yvec;\n"
                  "  texPosNvec[2] = texPos - zvec;\n"
                  "  float scalar;\n"
                  "  float opacity;\n"
                  "  vec4 gradient;\n"
                  "\n");

    for (auto& gradComp : results_texPos)
    {
      // opacityTFcall corresponds to code snippet used to compute the opacity
      opacityTFcall = ComputeOpacityEvaluationCall(
        mapper, inputs, noOfComponents, independentComponents, useGradYAxis, gradComp.second);
      shaderStr += std::string("  scalar = texture3D(volume,") + gradComp.second +
        std::string(")[c];\n"
                    "  scalar = scalar * in_volume_scale[index][c] + in_volume_bias[index][c];\n") +
        opacityTFcall + gradComp.first + " = opacity;\n";
    }

    if (::UseClippedVoxelIntensity(inputs) && mapper->GetClippingPlanes())
    {
      shaderStr +=
        std::string("  vec4 g1ObjDataPos[3], g2ObjDataPos[3];\n"
                    "  for (int i = 0; i < 3; ++i)\n"
                    "  {\n"
                    "    g1ObjDataPos[i] = clip_texToObjMat * vec4(texPosPvec[i], 1.0);\n"
                    "    if (g1ObjDataPos[i].w != 0.0)\n"
                    "    {\n"
                    "      g1ObjDataPos[i] /= g1ObjDataPos[i].w;\n"
                    "    }\n"
                    "    g2ObjDataPos[i] = clip_texToObjMat * vec4(texPosNvec[i], 1.0);\n"
                    "    if (g2ObjDataPos[i].w != 0.0)\n"
                    "    {\n"
                    "      g2ObjDataPos[i] /= g2ObjDataPos[i].w;\n"
                    "    }\n"
                    "  }\n"
                    "\n"
                    "  for (int i = 0; i < clip_numPlanes && !g_skip; i = i + 6)\n"
                    "  {\n"
                    "    vec3 planeOrigin = vec3(in_clippingPlanes[i + 1],\n"
                    "                            in_clippingPlanes[i + 2],\n"
                    "                            in_clippingPlanes[i + 3]);\n"
                    "    vec3 planeNormal = normalize(vec3(in_clippingPlanes[i + 4],\n"
                    "                                      in_clippingPlanes[i + 5],\n"
                    "                                      in_clippingPlanes[i + 6]));\n"
                    "    for (int j = 0; j < 3; ++j)\n"
                    "    {\n"
                    "      if (dot(vec3(planeOrigin - g1ObjDataPos[j].xyz), planeNormal) > 0)\n"
                    "      {\n"
                    "        g1[j] = in_clippedVoxelIntensity;\n"
                    "      }\n"
                    "      if (dot(vec3(planeOrigin - g2ObjDataPos[j].xyz), planeNormal) > 0)\n"
                    "      {\n"
                    "        g2[j] = in_clippedVoxelIntensity;\n"
                    "      }\n"
                    "    }\n"
                    "  }\n"
                    "\n");
    }

    if (!hasGradientOp)
    {
      shaderStr +=
        std::string("  // Central differences: (F_front - F_back) / 2h\n"
                    "  // This version of computeGradient() is only used for lighting\n"
                    "  // calculations (only direction matters), hence the difference is\n"
                    "  // not scaled by 2h and a dummy gradient mag is returned (-1.).\n"
                    "  return vec4((g1 - g2) / in_cellSpacing[index], -1.0);\n"
                    "}\n");
    }
    else
    {
      shaderStr += std::string(
        "  // Scale values the actual scalar range.\n"
        "  float range = in_scalarsRange[4*index+c][1] - in_scalarsRange[4*index+c][0];\n"
        "  g1 = in_scalarsRange[4*index+c][0] + range * g1;\n"
        "  g2 = in_scalarsRange[4*index+c][0] + range * g2;\n"
        "\n"
        "  // Central differences: (F_front - F_back) / 2h\n"
        "  g2 = g1 - g2;\n"
        "\n"
        "  float avgSpacing = (in_cellSpacing[index].x +\n"
        "   in_cellSpacing[index].y + in_cellSpacing[index].z) / 3.0;\n"
        "  vec3 aspect = in_cellSpacing[index] * 2.0 / avgSpacing;\n"
        "  g2 /= aspect;\n"
        "  float grad_mag = length(g2);\n"
        "\n"
        "  // Handle normalizing with grad_mag == 0.0\n"
        "  g2 = grad_mag > 0.0 ? normalize(g2) : vec3(0.0);\n"
        "\n"
        "  // Since the actual range of the gradient magnitude is unknown,\n"
        "  // assume it is in the range [0, 0.25 * dataRange].\n"
        "  range = range != 0 ? range : 1.0;\n"
        "  grad_mag = grad_mag / (0.25 * range);\n"
        "  grad_mag = clamp(grad_mag, 0.0, 1.0);\n"
        "\n"
        "  return vec4(g2.xyz, grad_mag);\n"
        "}\n");
    }
  }
  else
  {
    shaderStr += functionSignature +
      std::string("{\n"
                  "  return vec4(0.0);\n"
                  "}\n");
  }

  return shaderStr;
}

inline std::string PhaseFunctionDeclaration(
  vtkRenderer* vtkNotUsed(ren), vtkVolumeMapper* vtkNotUsed(mapper), vtkVolume* vol)
{
  std::string resStr;
  // to be compatible with the surface shading model,
  // the phase function should be normalized to 4pi instead of 1
  // that's why the isotropic phase function returns 1 and not 1/4pi for example
  if (std::abs(vol->GetProperty()->GetScatteringAnisotropy()) < 0.01)
  {
    resStr += R"***(
float phase_function(float cos_angle)
{
  return 1.0;
}
    )***";
  }
  else
  {
    resStr += R"***(
float g_anisotropy2 = in_anisotropy * in_anisotropy;

float phase_function(float cos_angle)
{
  float d = 1.0 + g_anisotropy2 - 2.0 * in_anisotropy * cos_angle;
  return (1.0 - g_anisotropy2) / (d * sqrt(d));
}

  )***";
  }
  return resStr;
}

//--------------------------------------------------------------------------
inline std::string ComputeLightingDeclaration(vtkRenderer* vtkNotUsed(ren), vtkVolumeMapper* mapper,
  vtkVolume* vol, int noOfComponents, int independentComponents, int totalNumberOfLights,
  int numberPositionalLights, bool defaultLighting)
{
  auto glMapper = vtkOpenGLGPUVolumeRayCastMapper::SafeDownCast(mapper);
  vtkVolumeProperty* volProperty = vol->GetProperty();
  std::string shaderStr = std::string("\
      \nvec4 computeLighting(vec4 color, int component, float label)\
      \n{\
      \n  vec4 finalColor = vec4(0.0);\n");

  // Shading for composite blending only
  int const shadeReqd = volProperty->GetShade() &&
    (mapper->GetBlendMode() == vtkVolumeMapper::COMPOSITE_BLEND ||
      mapper->GetBlendMode() == vtkVolumeMapper::ISOSURFACE_BLEND ||
      mapper->GetBlendMode() == vtkVolumeMapper::SLICE_BLEND);

  int const transferMode = volProperty->GetTransferFunctionMode();

  if (independentComponents)
  {
    shaderStr += "\n  int lightingComponent=component;\n";
  }
  else
  {
    shaderStr += "\n  int lightingComponent=0;\n";
  }

  bool const volumetricShadow = glMapper->GetVolumetricScatteringBlending() > 0.0;
  std::string volumetricCall = volumetricShadow
    ? "\n   vol_shadow = volumeShadow(g_dataPos, tex_light.xyz, 0.0, component, in_volume[0], "
      "0, label);"
    : "";
  std::string volumetricDeclarations =
    volumetricShadow ? "\n  float vol_shadow = 1.0;\n  vec4 tex_light = vec4(0.0);\n" : "\n";

  // If shading is required, we compute a shading gradient (used for the shading model)
  if (shadeReqd)
  {
    if (glMapper->GetComputeNormalFromOpacity())
    {
      // we compute the gradienty according to the volume's opacity !
      shaderStr +=
        std::string("  vec4 shading_gradient = computeDensityGradient(g_dataPos, component, "
                    "in_volume[0], 0, label);\n");
    }
    else
    {
      // otherwise we take the scalar gradient directly
      shaderStr += std::string(
        "  vec4 shading_gradient = computeGradient(g_dataPos, component, in_volume[0], 0);\n");
    }
  }

  // If we need the scalar gradient (typically to sample a transfer function)
  if (volProperty->HasGradientOpacity() || volProperty->HasLabelGradientOpacity())
  {
    // If we didn't compute it before, we compute it
    if (!shadeReqd || glMapper->GetComputeNormalFromOpacity())
    {
      shaderStr +=
        std::string("  vec4 gradient = computeGradient(g_dataPos, component, in_volume[0], 0);\n");
    }
    // otherwise, we use what we already computed
    else
    {
      shaderStr += std::string("  vec4 gradient = shading_gradient;\n");
    }
  }

  if (shadeReqd)
  {
    if (defaultLighting)
    {
      shaderStr += R"***(
  vec3 diffuse = vec3(0.0);
  vec3 specular = vec3(0.0);
  vec3 normal = shading_gradient.xyz;
  float normalLength = length(normal);
  if (normalLength > 0.0)
  {
    normal = normalize(normal);
  }
  else
  {
    normal = vec3(0.0, 0.0, 0.0);
  }
  // XXX: normal is oriented inside the volume, so we take -g_ldir/-g_vdir
  float nDotL = dot(normal, -g_ldir[0]);
  vec3 r = normalize(2.0 * nDotL * normal + g_ldir[0]);
  float vDotR = dot(r, -g_vdir[0]);
  if (nDotL < 0.0 && in_twoSidedLighting)
  {
    nDotL = -nDotL;
  }
  if (nDotL > 0.0)
  {
    diffuse = nDotL * in_diffuse[lightingComponent] *
    in_lightDiffuseColor[0] * color.rgb;
    vDotR = max(vDotR, 0.0);
    specular = pow(vDotR, in_shininess[lightingComponent]) *
                 in_specular[lightingComponent] *
                 in_lightSpecularColor[0];
  }
  // For the headlight, ignore the light's ambient color
  // for now as it is causing the old mapper tests to fail
  finalColor.xyz = in_ambient[lightingComponent] * color.rgb +
                   diffuse + specular;

        )***";
    }
    else if (totalNumberOfLights > 0)
    {
      shaderStr += R"***(
  g_fragWorldPos = g_texToView * vec4(g_dataPos, 1.0);
  if (g_fragWorldPos.w != 0.0)
  {
  g_fragWorldPos /= g_fragWorldPos.w;
  }
  vec3 viewDirection = normalize(-g_fragWorldPos.xyz);
  vec3 ambient = vec3(0,0,0);
  vec3 diffuse = vec3(0,0,0);
  vec3 specular = vec3(0,0,0);
  vec3 vertLightDirection;
  vec3 normal = normalize((in_textureToEye[0] * vec4(shading_gradient.xyz, 0.0)).xyz);
  vec3 lightDir;
        )***";

      if (numberPositionalLights > 0)
      {
        shaderStr += R"***(
  for (int posNum = 0; posNum < NUMBER_POS_LIGHTS; posNum++)
  {
    float attenuation = 1.0;
    lightDir = in_lightDirection[posNum];
    vertLightDirection = (g_fragWorldPos.xyz - in_lightPosition[posNum]);
    float distance = length(vertLightDirection);
    vertLightDirection = normalize(vertLightDirection);
    attenuation = 1.0 /
                  (in_lightAttenuation[posNum].x
                  + in_lightAttenuation[posNum].y * distance
                  + in_lightAttenuation[posNum].z * distance * distance);
    // per OpenGL standard cone angle is 90 or less for a spot light
    if (in_lightConeAngle[posNum] <= 90.0)
    {
      float coneDot = dot(vertLightDirection, lightDir);
      // if inside the cone
      if (coneDot >= cos(radians(in_lightConeAngle[posNum])))
      {
        attenuation = attenuation * pow(coneDot, in_lightExponent[posNum]);
      }
      else
      {
        attenuation = 0.0;
      }
    }

    float nDotL = dot(normal, vertLightDirection);
    if (nDotL < 0.0 && in_twoSidedLighting)
    {
      nDotL = -nDotL;
    }
    if (nDotL > 0.0)
    {
      float df = max(0.0, attenuation * nDotL);
      diffuse += (df * in_lightDiffuseColor[posNum]);
      vec3 r = normalize(2.0 * nDotL * normal - vertLightDirection);
      float rDotV = dot(-viewDirection, r);
      if (rDotV < 0.0 && in_twoSidedLighting)
      {
        rDotV = -rDotV;
      }
      if (rDotV > 0.0)
      {
        float sf = attenuation * pow(rDotV, in_shininess[lightingComponent]);
        specular += (sf * in_lightSpecularColor[posNum]);
      }
    }
    ambient += in_lightAmbientColor[posNum];
  }
        )***";
      }

      shaderStr += R"***(
  for (int dirNum = NUMBER_POS_LIGHTS; dirNum < TOTAL_NUMBER_LIGHTS; dirNum++)
  {
    vertLightDirection = in_lightDirection[dirNum];
    float nDotL = dot(normal, vertLightDirection);
    if (nDotL < 0.0 && in_twoSidedLighting)
    {
      nDotL = -nDotL;
    }
    if (nDotL > 0.0)
    {
      float df = max(0.0, nDotL);
      diffuse += (df * in_lightDiffuseColor[dirNum]);
      vec3 r = normalize(2.0 * nDotL * normal - vertLightDirection);
      float rDotV = dot(-viewDirection, r);
      if (rDotV > 0.0)
      {
        float sf = pow(rDotV, in_shininess[lightingComponent]);
        specular += (sf * in_lightSpecularColor[dirNum]);
      }
    }
    ambient += in_lightAmbientColor[dirNum];
  }
  finalColor.xyz = in_ambient[lightingComponent] * ambient +
                   in_diffuse[lightingComponent] * diffuse * color.rgb +
                   in_specular[lightingComponent] * specular;

      )***";
    }
  }
  else
  {
    shaderStr += std::string("\n  finalColor = vec4(color.rgb, 0.0);");
  }

  if (glMapper->GetVolumetricScatteringBlending() > 0.0 && totalNumberOfLights > 0)
  {

    float vsBlend = glMapper->GetVolumetricScatteringBlending();
    std::string blendingFormula = std::string("  float vol_coef = ") +
      (vsBlend < 1.0 ? "2.0 * in_volumetricScatteringBlending * exp( - 2.0 * "
                       "in_volumetricScatteringBlending * shading_gradient.w * color.a)"
                     : "2.0 * (1.0 - in_volumetricScatteringBlending) * exp( - 2.0 * "
                       "in_volumetricScatteringBlending * shading_gradient.w * color.a) + 2.0 * "
                       "in_volumetricScatteringBlending - 1.0") +
      ";\n";

    shaderStr +=
      (defaultLighting
          ? std::string()
          : std::string(
              "vec3 view_tdir = normalize((g_eyeToTexture * vec4(viewDirection, 0.0)).xyz);\n")) +
      R"***(
  vec3 secondary_contrib = vec3(0.0);
  vec3 tex_light = vec3(0.0);
  shading_gradient.w = length(shading_gradient.xyz);
  vec3 diffuse_light = vec3(0.0);
  float attenuation = 0.0;
  float vol_shadow = 0.0;
  float phase = 1.0;
    )***";

    if (defaultLighting)
    {
      shaderStr += R"***(
  tex_light = (in_inverseTextureDatasetMatrix[0] * vec4(in_eyePosObjs[0], 1.0)).xyz;
  phase = phase_function(-1); // always angle of pi
  vol_shadow = volumeShadow(g_dataPos, tex_light, 1.0, component, in_volume[0], 0, label);
  secondary_contrib += vol_shadow * phase * color.rgb * in_diffuse[lightingComponent] * in_lightDiffuseColor[0];
  secondary_contrib += in_ambient[lightingComponent] * in_lightAmbientColor[0];
      )***";
    }
    else
    {
      if (numberPositionalLights > 0)
      {
        shaderStr += R"***(
  float dist_light = 0.0;
  for(int posNum = 0; posNum < NUMBER_POS_LIGHTS; posNum++)
  {
    tex_light = g_lightPositionTex[posNum];
    vec3 light_vert = g_fragWorldPos.xyz - in_lightPosition[posNum];
    dist_light = length(light_vert);
    float light_angle = dot(normalize(light_vert), normalize(in_lightDirection[posNum]));
    phase = phase_function(dot(normalize(g_dataPos - tex_light), view_tdir));
    attenuation = 1.0 /
                  (in_lightAttenuation[posNum].x
                  + in_lightAttenuation[posNum].y * dist_light
                  + in_lightAttenuation[posNum].z * dist_light * dist_light);
    attenuation *= max(0.0, sign(light_angle - cos(radians(in_lightConeAngle[posNum]))))
                     * pow(light_angle, in_lightExponent[posNum]);
    vol_shadow = volumeShadow(g_dataPos, tex_light, 1.0, component, in_volume[0], 0, label);
    secondary_contrib += vol_shadow * phase * attenuation * color.rgb * in_diffuse[lightingComponent] * in_lightDiffuseColor[posNum];
    secondary_contrib += in_ambient[lightingComponent] * in_lightAmbientColor[posNum];
  }
      )***";
      }

      shaderStr += R"***(
  for(int dirNum = NUMBER_POS_LIGHTS; dirNum < TOTAL_NUMBER_LIGHTS; dirNum++)
  {
    tex_light = g_lightDirectionTex[dirNum];
    phase = phase_function(dot(normalize(-tex_light), view_tdir));
    vol_shadow = volumeShadow(g_dataPos, tex_light, 0.0, component, in_volume[0], 0, label);
    secondary_contrib += vol_shadow * phase * color.rgb * in_diffuse[lightingComponent] * in_lightDiffuseColor[dirNum];
    secondary_contrib += in_ambient[lightingComponent] * in_lightAmbientColor[dirNum];
  }
      )***";
    }

    shaderStr += blendingFormula +
      R"***(
  finalColor.xyz = (1.0 - vol_coef) * finalColor.xyz + vol_coef * secondary_contrib;
      )***";
  }

  // For 1D transfers only (2D transfer functions hold scalar and
  // gradient-magnitude opacities combined in the same table).
  // For multiple inputs, a different computeGradientOpacity() signature
  // is defined.
  if (transferMode == vtkVolumeProperty::TF_1D && glMapper->GetInputCount() == 1)
  {
    if (noOfComponents == 1 || !independentComponents)
    {
      if (volProperty->HasGradientOpacity())
      {
        shaderStr += std::string("\
            \n  if (gradient.w >= 0.0 && label == 0.0)\
            \n    {\
            \n    color.a *= computeGradientOpacity(gradient);\
            \n    }");
      }
      if (volProperty->HasLabelGradientOpacity())
      {
        shaderStr += std::string("\
            \n  if (gradient.w >= 0.0 && label > 0.0)\
            \n    {\
            \n    color.a *= computeGradientOpacityForLabel(gradient, label);\
            \n    }");
      }
    }
    else if (noOfComponents > 1 && independentComponents && volProperty->HasGradientOpacity())
    {
      shaderStr += std::string("\
        \n  if (gradient.w >= 0.0)\
        \n    {\
        \n    for (int i = 0; i < in_noOfComponents; ++i)\
        \n      {\
        \n      color.a = color.a *\
        \n      computeGradientOpacity(gradient, i) * in_componentWeight[i];\
        \n      }\
        \n    }");
    }
  }

  shaderStr += std::string("\
      \n  finalColor.a = color.a;\
      \n  //VTK::ComputeLighting::Exit\
      \n  return finalColor;\
      \n  }");

  return shaderStr;
}

//--------------------------------------------------------------------------
inline std::string ComputeLightingMultiDeclaration(vtkRenderer* vtkNotUsed(ren),
  vtkVolumeMapper* mapper, vtkVolume* vol, int noOfComponents, int independentComponents,
  int vtkNotUsed(totalNumberOfLights), bool defaultLighting)
{
  auto glMapper = vtkOpenGLGPUVolumeRayCastMapper::SafeDownCast(mapper);
  vtkVolumeProperty* volProperty = vol->GetProperty();
  std::string shaderStr = std::string();

  // if no gradient TF is needed, don't add it into the function signature
  if (volProperty->HasGradientOpacity())
  {
    shaderStr += std::string("\
      \nvec4 computeLighting(vec3 texPos, vec4 color, const in sampler2D gradientTF, const in sampler3D volume, const in sampler2D opacityTF, const int volIdx, int component)\
      \n  {\
      \n  vec4 finalColor = vec4(0.0);\n");
  }
  else
  {
    shaderStr += std::string("\
      \nvec4 computeLighting(vec3 texPos, vec4 color, const in sampler3D volume, const in sampler2D opacityTF, const int volIdx, int component)\
      \n  {\
      \n  vec4 finalColor = vec4(0.0);\n");
  }

  if (independentComponents)
  {
    shaderStr += "\n  int lightingComponent=component;\n";
  }
  else
  {
    shaderStr += "\n  int lightingComponent=0;\n";
  }

  // Shading for composite blending only
  int const shadeReqd = volProperty->GetShade() &&
    (mapper->GetBlendMode() == vtkVolumeMapper::COMPOSITE_BLEND ||
      mapper->GetBlendMode() == vtkVolumeMapper::ISOSURFACE_BLEND);

  int const transferMode = volProperty->GetTransferFunctionMode();

  // If shading is required, we compute a shading gradient (used for the shading model)
  if (shadeReqd)
  {
    /*
    We compute the gradient every time, because the alternative would be to test whether
    the volume has gradient cache or not. But as both branches will be evaluated anyway
    on GPU, we might as well compute the gradient every time.
    */
    if (glMapper->GetComputeNormalFromOpacity())
    {
      if (volProperty->HasGradientOpacity())
      {
        shaderStr += "  vec4 shading_gradient = computeDensityGradient(texPos, component, volume, "
                     "opacityTF, gradientTF, volIdx, 0.0);\n";
      }
      else
      {
        shaderStr += "  vec4 shading_gradient = computeDensityGradient(texPos, component, volume, "
                     "opacityTF, volIdx, 0.0);\n";
      }
    }
    else
    {
      shaderStr +=
        "  vec4 shading_gradient = computeGradient(texPos, component, volume, volIdx);\n";
    }
  }

  // If we need the scalar gradient (typically to sample a transfer function)
  if (volProperty->HasGradientOpacity())
  {
    if (!shadeReqd || glMapper->GetComputeNormalFromOpacity())
    {
      shaderStr += "  vec4 gradient = computeGradient(texPos, component, volume, volIdx);\n";
    }
    else
    {
      // if we already computed it
      shaderStr += "  vec4 gradient = shading_gradient;\n";
    }
  }

  if (shadeReqd && defaultLighting)
  {
    shaderStr += std::string("\
        \n  vec3 diffuse = vec3(0.0);\
        \n  vec3 specular = vec3(0.0);\
        \n  vec3 normal = shading_gradient.xyz;\
        \n  float normalLength = length(normal);\
        \n  if (normalLength > 0.0)\
        \n    {\
        \n    normal = normalize(normal);\
        \n    }\
        \n  else\
        \n    {\
        \n    normal = vec3(0.0, 0.0, 0.0);\
        \n    }\
        \n   // normal is oriented inside the volume (because normal = gradient, oriented inside the volume)\
        \n   // thus we have to take minus everything\
        \n   float nDotL = dot(normal, -g_ldir[volIdx]);\
        \n   vec3 r = normalize(2.0 * nDotL * normal + g_ldir[volIdx]);\
        \n   float vDotR = dot(r, -g_vdir[volIdx]);\
        \n   if (nDotL < 0.0 && in_twoSidedLighting)\
        \n     {\
        \n     nDotL = -nDotL;\
        \n     }\
        \n   if (nDotL > 0.0)\
        \n     {\
        \n        diffuse = nDotL * in_diffuse[lightingComponent] *\
        \n                 in_lightDiffuseColor[0] * color.rgb;\
        \n        vDotR = max(vDotR, 0.0);\
        \n        specular = pow(vDotR, in_shininess[lightingComponent]) *\
        \n                   in_specular[lightingComponent] *\
        \n                   in_lightSpecularColor[0];\
        \n     }\
        \n  // For the headlight, ignore the light's ambient color\
        \n  // for now as it is causing the old mapper tests to fail\
        \n  finalColor.xyz = in_ambient[lightingComponent] * color.rgb +\
        \n                   diffuse + specular;\
        \n");
  }
  else
  {
    shaderStr += std::string("\n  finalColor = vec4(color.rgb, 0.0);");
  }

  // For 1D transfers only (2D transfer functions hold scalar and
  // gradient-magnitude opacities combined in the same table).
  if (transferMode == vtkVolumeProperty::TF_1D)
  {
    if (volProperty->HasGradientOpacity() && (noOfComponents == 1 || !independentComponents))
    {
      shaderStr += std::string("\
          \n  if (gradient.w >= 0.0)\
          \n    {\
          \n    color.a = color.a *\
          \n              computeGradientOpacity(gradient, gradientTF);\
          \n    }");
    }
  }

  shaderStr += std::string("\
      \n  finalColor.a = color.a;\
      \n  //VTK::ComputeLighting::Exit\
      \n  return clamp(finalColor, 0.0, 1.0);\
      \n  }");

  return shaderStr;
}

//--------------------------------------------------------------------------
inline std::string ComputeRayDirectionDeclaration(vtkRenderer* ren,
  vtkVolumeMapper* vtkNotUsed(mapper), vtkVolume* vtkNotUsed(vol), int vtkNotUsed(noOfComponents))
{
  if (!ren->GetActiveCamera()->GetParallelProjection())
  {
    return std::string("\
        \nvec3 computeRayDirection()\
        \n  {\
        \n  return normalize(ip_vertexPos.xyz - in_eyePosObjs[0].xyz);\
        \n  }");
  }
  else
  {
    return std::string("\
        \nuniform vec3 in_projectionDirection;\
        \nvec3 computeRayDirection()\
        \n  {\
        \n  return normalize((in_inverseVolumeMatrix[0] *\
        \n                   vec4(in_projectionDirection, 0.0)).xyz);\
        \n  }");
  }
}

//--------------------------------------------------------------------------
inline std::string ComputeColorUniforms(vtkOpenGLGPUVolumeRayCastMapper::VolumeInputMap& inputs,
  int noOfComponents, vtkVolumeProperty* volProp)
{
  std::string resStr;
  if (inputs.size() > 1)
  {
    // multi volume
    for (auto& item : inputs)
    {
      const auto& prop = item.second.Volume->GetProperty();
      if (prop->GetTransferFunctionMode() != vtkVolumeProperty::TF_1D)
        continue;

      auto& map = item.second.RGBTablesMap;
      const auto numComp = map.size();
      resStr +=
        "uniform sampler2D " + ArrayBaseName(map[0]) + "[" + std::to_string(numComp) + "];\n";
    }
  }
  else
  {
    // single volume
    if (volProp->GetTransferFunctionMode() == vtkVolumeProperty::TF_1D)
    {
      resStr += "uniform sampler2D " + ArrayBaseName(inputs[0].RGBTablesMap[0]) + "[" +
        std::to_string(noOfComponents) + "];\n";
    }
    // in case of TF_2D, the texture needed is defined with computeOpacity
  }
  return resStr;
}

//--------------------------------------------------------------------------
inline std::string ComputeColorDeclaration(vtkRenderer* vtkNotUsed(ren),
  vtkVolumeMapper* vtkNotUsed(mapper), vtkVolume* vtkNotUsed(vol), int noOfComponents,
  int independentComponents, std::map<int, std::string> colorTableMap)
{
  std::ostringstream ss;

  std::string shaderStr = ss.str();
  if (noOfComponents == 1)
  {
    shaderStr += std::string("\
          \nvec4 computeColor(vec4 scalar, float opacity)\
          \n  {\
          \n  return clamp(computeLighting(vec4(texture2D(" +
      colorTableMap[0] + ",\
          \n                         vec2(scalar.w, 0.0)).xyz, opacity), 0, 0.0), 0.0, 1.0);\
          \n  }");
    return shaderStr;
  }
  else if (noOfComponents > 1 && independentComponents)
  {
    std::ostringstream toString;

    shaderStr += std::string("\
          \nvec4 computeColor(vec4 scalar, float opacity, int component)\
          \n  {");

    for (int i = 0; i < noOfComponents; ++i)
    {
      toString << i;
      shaderStr += std::string("\
            \n  if (component == " +
        toString.str() + ")");

      shaderStr += std::string("\
            \n    {\
            \n    return clamp(computeLighting(vec4(texture2D(\
            \n      " +
        colorTableMap[i]);
      shaderStr += std::string(", vec2(\
            \n      scalar[" +
        toString.str() + "],0.0)).xyz,\
            \n      opacity)," +
        toString.str() + ", 0.0), 0.0, 1.0);\
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
    shaderStr += std::string("\
          \nvec4 computeColor(vec4 scalar, float opacity)\
          \n  {\
          \n  return clamp(computeLighting(vec4(texture2D(" +
      colorTableMap[0] + ",\
          \n                                        vec2(scalar.x, 0.0)).xyz,\
          \n                              opacity), 0, 0.0), 0.0, 1.0);\
          \n  }");
    return shaderStr;
  }
  else if (noOfComponents == 4 && !independentComponents)
  {
    shaderStr += std::string("\
          \nvec4 computeColor(vec4 scalar, float opacity)\
          \n  {\
          \n  return clamp(computeLighting(vec4(scalar.xyz, opacity), 3, 0.0), 0.0, 1.0);\
          \n  }");
    return shaderStr;
  }
  else
  {
    shaderStr += std::string("\
          \nvec4 computeColor(vec4 scalar, float opacity)\
          \n  {\
          \n  return clamp(computeLighting(vec4(scalar.xyz, opacity), 0, 0.0), 0.0, 1.0);\
          \n  }");
    return shaderStr;
  }
}

//--------------------------------------------------------------------------
inline std::string ComputeColorMultiDeclaration(
  vtkOpenGLGPUVolumeRayCastMapper::VolumeInputMap& inputs, bool useGradientTF)
{
  std::ostringstream ss;
  int lastComponentMode = vtkVolumeInputHelper::INVALID;
  std::map<int, std::string> lastColorTableMap;
  for (auto& item : inputs)
  {
    auto prop = item.second.Volume->GetProperty();
    if (prop->GetTransferFunctionMode() != vtkVolumeProperty::TF_1D)
      continue;
    auto& map = item.second.RGBTablesMap;
    lastComponentMode = item.second.ComponentMode;
    lastColorTableMap = map;
  }

  if (lastComponentMode == vtkVolumeInputHelper::LA)
  {
    ss << "vec4 computeColor(vec4 scalar, const in sampler2D colorTF)\
      \n  {\
      \n  return clamp(computeLighting(vec4(texture2D(colorTF,\
      \n                         vec2(scalar.w, 0.0)).xyz, opacity), 0), 0.0, 1.0);\
      \n  }\n";
  }
  else
  {
    std::ostringstream colorDec;
    colorDec << "  vec3 color = ";
    if (lastComponentMode == vtkVolumeInputHelper::RGBA)
    {
      // Use RGB components without mapping through the color transfer function.
      colorDec << "scalar.xyz;\n";
    }
    else // vtkVolumeInputHelper::INDEPENDENT
    {
      // MultiVolume assumes input is 1-component, see ShadingMultipleInputs.
      // To support multiple independent components, each component should be mapped through the
      // transfer function as done in ComputeColorDeclaration for single volumes.
      colorDec << "texture2D(colorTF, vec2(scalar.w, 0.0)).xyz;\n";
    }

    if (useGradientTF)
    {
      ss
        << "vec4 computeColor(vec3 texPos, vec4 scalar, float opacity, const in sampler2D colorTF, "
           "const in sampler2D gradientTF, const in sampler3D volume, const in sampler2D "
           "opacityTF, const int volIdx)\n\n"
           "{\n";
      ss << colorDec.str()
         << "  return clamp(computeLighting(texPos, vec4(color, opacity), gradientTF, volume, "
            "opacityTF,"
            "volIdx, 0), 0.0, 1.0);\n"
            "}\n";
    }
    else
    {
      ss
        << "vec4 computeColor(vec3 texPos, vec4 scalar, float opacity, const in sampler2D colorTF, "
           "const in sampler3D volume, const in sampler2D opacityTF, const int volIdx)\n\n"
           "{\n";
      ss << colorDec.str()
         << "  return clamp(computeLighting(texPos, vec4(color, opacity), volume, opacityTF,"
            "volIdx, 0), 0.0, 1.0);\n"
            "}\n";
    }
  }

  return ss.str();
}

//--------------------------------------------------------------------------
inline std::string ComputeOpacityMultiDeclaration(
  vtkOpenGLGPUVolumeRayCastMapper::VolumeInputMap& inputs)
{
  std::ostringstream ss;
  for (auto& item : inputs)
  {
    auto prop = item.second.Volume->GetProperty();
    if (prop->GetTransferFunctionMode() != vtkVolumeProperty::TF_1D)
      continue;

    auto& map = item.second.OpacityTablesMap;
    const auto numComp = map.size();
    ss << "uniform sampler2D " << ArrayBaseName(map[0]) << "[" << numComp << "];\n";
  }

  ss << "float computeOpacity(vec4 scalar, const in sampler2D opacityTF)\n"
        "{\n"
        "  return texture2D(opacityTF, vec2(scalar.w, 0)).r;\n"
        "}\n";
  return ss.str();
}

//--------------------------------------------------------------------------
inline std::string ComputeGradientOpacityMulti1DDecl(
  vtkOpenGLGPUVolumeRayCastMapper::VolumeInputMap& inputs)
{
  std::ostringstream ss;

  for (auto& item : inputs)
  {
    auto prop = item.second.Volume->GetProperty();
    if (prop->GetTransferFunctionMode() != vtkVolumeProperty::TF_1D || !prop->HasGradientOpacity())
      continue;

    auto& map = item.second.GradientOpacityTablesMap;
    const auto numComp = map.size();
    ss << "uniform sampler2D " << ArrayBaseName(map[0]) << "[" << numComp << "];\n";
  }

  ss << "float computeGradientOpacity(vec4 grad, const in sampler2D gradientTF)\n"
        "{\n"
        "  return texture2D(gradientTF, vec2(grad.w, 0.0)).r;\n"
        "}\n";
  return ss.str();
}

//--------------------------------------------------------------------------
inline std::string ComputeOpacityDeclaration(vtkRenderer* vtkNotUsed(ren),
  vtkVolumeMapper* vtkNotUsed(mapper), vtkVolume* vtkNotUsed(vol), int noOfComponents,
  int independentComponents, std::map<int, std::string> opacityTableMap)
{
  std::ostringstream ss;
  ss << "uniform sampler2D " << ArrayBaseName(opacityTableMap[0]) << "[" << noOfComponents
     << "];\n";

  std::string shaderStr = ss.str();
  if (noOfComponents > 1 && independentComponents)
  {
    shaderStr += std::string("\
        \nfloat computeOpacity(vec4 scalar, int component)\
        \n{");

    for (int i = 0; i < noOfComponents; ++i)
    {
      std::ostringstream toString;
      toString << i;
      shaderStr += std::string("\
          \n  if (component == " +
        toString.str() + ")");

      shaderStr += std::string("\
          \n  {\
          \n    return texture2D(" +
        opacityTableMap[i]);

      shaderStr += std::string(",vec2(scalar[" + toString.str() + "], 0)).r;\
          \n  }");
    }

    shaderStr += std::string("\n}");
    return shaderStr;
  }
  else if (noOfComponents == 2 && !independentComponents)
  {
    shaderStr += std::string("\
        \nfloat computeOpacity(vec4 scalar)\
        \n{\
        \n  return texture2D(" +
      opacityTableMap[0] + ", vec2(scalar.y, 0)).r;\
        \n}");
    return shaderStr;
  }
  else
  {
    shaderStr += std::string("\
        \nfloat computeOpacity(vec4 scalar)\
        \n{\
        \n  return texture2D(" +
      opacityTableMap[0] + ", vec2(scalar.w, 0)).r;\
        \n}");
    return shaderStr;
  }
}

//--------------------------------------------------------------------------
inline std::string ComputeColor2DYAxisDeclaration(int noOfComponents,
  int vtkNotUsed(independentComponents), std::map<int, std::string> colorTableMap)
{
  if (noOfComponents == 1)
  {
    // Single component
    return std::string(
      "vec4 computeColor(vec4 scalar, float opacity)\n"
      "{\n"
      "  vec4 yscalar = texture3D(in_transfer2DYAxis, g_dataPos);\n"
      "  yscalar.r = yscalar.r * in_transfer2DYAxis_scale.r + in_transfer2DYAxis_bias.r;\n"
      "  yscalar = vec4(yscalar.r);\n"
      "  vec4 color = texture2D(" +
      colorTableMap[0] +
      ",\n"
      "                         vec2(scalar.w, yscalar.w));\n"
      "  return computeLighting(color, 0, 0);\n"
      "}\n");
  }
  return std::string("vec4 computeColor(vec4 scalar, float opacity)\n"
                     "{\n"
                     "  return vec4(0, 0, 0, 0)\n"
                     "}\n");
}

//--------------------------------------------------------------------------
inline std::string ComputeColor2DDeclaration(vtkRenderer* vtkNotUsed(ren),
  vtkVolumeMapper* vtkNotUsed(mapper), vtkVolume* vtkNotUsed(vol), int noOfComponents,
  int independentComponents, std::map<int, std::string> colorTableMap, int useGradient)
{
  if (!useGradient)
  {
    return ComputeColor2DYAxisDeclaration(noOfComponents, independentComponents, colorTableMap);
  }
  if (noOfComponents == 1)
  {
    // Single component
    return std::string("vec4 computeColor(vec4 scalar, float opacity)\n"
                       "{\n"
                       "  vec4 color = texture2D(" +
      colorTableMap[0] +
      ",\n"
      "    vec2(scalar.w, g_gradients_0[0].w));\n"
      "  return computeLighting(color, 0, 0);\n"
      "}\n");
  }
  else if (noOfComponents > 1 && independentComponents)
  {
    // Multiple independent components
    std::string shaderStr;
    shaderStr += std::string("vec4 computeColor(vec4 scalar, float opacity, int component)\n"
                             "{\n");

    for (int i = 0; i < noOfComponents; ++i)
    {
      std::ostringstream toString;
      toString << i;
      std::string const num = toString.str();
      shaderStr += std::string("  if (component == " + num +
        ")\n"
        "  {\n"
        "    vec4 color = texture2D(" +
        colorTableMap[i] +
        ",\n"
        "      vec2(scalar[" +
        num + "], g_gradients_0[" + num +
        "].w));\n"
        "    return computeLighting(color, " +
        num +
        ", 0.0);\n"
        "  }\n");
    }
    shaderStr += std::string("}\n");

    return shaderStr;
  }
  else if (noOfComponents == 2 && !independentComponents)
  {
    // Dependent components (Luminance/ Opacity)
    return std::string("vec4 computeColor(vec4 scalar, float opacity)\n"
                       "{\n"
                       "  vec4 color = texture2D(" +
      colorTableMap[0] +
      ",\n"
      "    vec2(scalar.x, g_gradients_0[0].w));\n"
      "  return computeLighting(color, 0, 0.0);\n"
      "}\n");
  }
  else
  {
    return std::string("vec4 computeColor(vec4 scalar, float opacity)\n"
                       "{\n"
                       "  return computeLighting(vec4(scalar.xyz, opacity), 0, 0.0);\n"
                       "}\n");
  }
}

//--------------------------------------------------------------------------
inline std::string Transfer2DDeclaration(vtkOpenGLGPUVolumeRayCastMapper::VolumeInputMap& inputs)
{
  std::ostringstream ss;
  for (auto& item : inputs)
  {
    auto prop = item.second.Volume->GetProperty();
    if (prop->GetTransferFunctionMode() != vtkVolumeProperty::TF_2D)
      continue;

    auto& map = item.second.TransferFunctions2DMap;
    const auto numComp = map.size();
    ss << "uniform sampler2D " << ArrayBaseName(map[0]) << "[" << numComp << "];\n";
  }

  std::string result = ss.str() +
    std::string("uniform sampler3D in_transfer2DYAxis;\n"
                "uniform vec4 in_transfer2DYAxis_scale;\n"
                "uniform vec4 in_transfer2DYAxis_bias;\n");

  return result;
}

//--------------------------------------------------------------------------
inline std::string ComputeOpacity2DDeclaration(vtkRenderer* vtkNotUsed(ren),
  vtkVolumeMapper* vtkNotUsed(mapper), vtkVolume* vtkNotUsed(vol), int noOfComponents,
  int independentComponents, std::map<int, std::string> opacityTableMap, int useGradient)
{
  std::ostringstream toString;
  if (noOfComponents > 1 && independentComponents)
  {
    // Multiple independent components
    toString << "float computeOpacity(vec4 scalar, int component)\n"
                "{\n";
    if (!useGradient)
    {
      toString
        << "vec4 yscalar = texture3D(in_transfer2DYAxis, g_dataPos);\n"
           "for (int i = 0; i < 4; ++i)\n"
           "{\n"
           "  yscalar[i] = yscalar[i] * in_transfer2DYAxis_scale[i] + in_transfer2DYAxis_bias[i];\n"
           "}\n";
      if (noOfComponents == 1)
      {
        toString << "yscalar = vec4(yscalar.r);\n";
      }
    }

    for (int i = 0; i < noOfComponents; ++i)
    {
      if (useGradient)
      {
        toString << "  if (component == " << i
                 << ")\n"
                    "  {\n"
                    "    return texture2D("
                 << opacityTableMap[i]
                 << ",\n"
                    "      vec2(scalar["
                 << i << "], g_gradients_0[" << i
                 << "].w)).a;\n"
                    "  }\n";
      }
      else
      {
        toString << "  if (component == " << i
                 << ")\n"
                    "  {\n"
                    "    return texture2D("
                 << opacityTableMap[i]
                 << ",\n"
                    "      vec2(scalar["
                 << i << "], yscalar[" << i
                 << "])).a;\n"
                    "  }\n";
      }
    }

    toString << "}\n";
  }

  else if (noOfComponents == 2 && !independentComponents)
  {
    if (useGradient)
    {
      // Dependent components (Luminance/ Opacity)
      toString << "float computeOpacity(vec4 scalar)\n"
                  "{\n"
                  "  return texture2D(" +
          opacityTableMap[0] +
          ",\n"
          "    vec2(scalar.y, g_gradients_0[0].w)).a;\n"
          "}\n";
    }
    else
    {
      // Dependent components (Luminance/ Opacity)
      toString << "float computeOpacity(vec4 scalar)\n"
                  "{\n"
                  "  return texture2D(" +
          opacityTableMap[0] +
          ",\n"
          "    vec2(scalar.y, yscalar.y)).a;\n"
          "}\n";
    }
  }

  else
  {
    if (useGradient)
    {
      // Dependent compoennts (RGBA) || Single component
      toString << "float computeOpacity(vec4 scalar)\n"
                  "{\n"
                  "  return texture2D(" +
          opacityTableMap[0] +
          ",\n"
          "    vec2(scalar.a, g_gradients_0[0].w)).a;\n"
          "}\n";
    }
    else
    {
      // Dependent compoennts (RGBA) || Single component
      toString
        << "float computeOpacity(vec4 scalar)\n"
           "{\n"
           "  vec4 yscalar = texture3D(in_transfer2DYAxis, g_dataPos);\n"
           "  yscalar.r = yscalar.r * in_transfer2DYAxis_scale.r + in_transfer2DYAxis_bias.r;\n"
           "  yscalar = vec4(yscalar.r);\n"
           "  return texture2D(" +
          opacityTableMap[0] +
          ",\n"
          "    vec2(scalar.a, yscalar.w)).a;\n"
          "}\n";
    }
  }
  return toString.str();
}

//--------------------------------------------------------------------------
inline std::string ComputeVolumetricShadowDec(vtkOpenGLGPUVolumeRayCastMapper* mapper,
  vtkVolume* vtkNotUsed(vol), int noOfComponents, int independentComponents,
  vtkOpenGLGPUVolumeRayCastMapper::VolumeInputMap& inputs, int useGradYAxis)
{
  std::string resStr;
  std::string declarations;
  std::string functionSignature;
  std::string opacityEval;
  std::string rayInit;

  const size_t numInputs = inputs.size();
  const bool hasGradOp = ::HasGradientOpacity(inputs);

  // for now, shadow is mono-chromatic (we only sample opacity)
  // it could be RGB

  functionSignature = "float volumeShadow(vec3 sample_position, vec3 light_pos_dir, float is_Pos, "
                      " in int c, in sampler3D volume, " +
    (numInputs > 1 ? std::string("in sampler2D opacityTF, ") : std::string()) +
    (numInputs > 1 && hasGradOp ? std::string("in sampler2D gradTF, ") : std::string()) +
    "int index, float label)\n";

  declarations +=
    R"***(
  float shadow = 1.0;
  vec3 direction = vec3(0.0);
  vec3 norm_dir = vec3(0.0);
  float maxdist = 0.0;
  float scalar;
  vec4 gradient;
  float opacity = 0.0;
  vec3 color;
  Ray ray;
  Hit hit;
  float sampled_dist = 0.0;
  vec3 sampled_point = vec3(0.0);
    )***";

  rayInit +=
    R"***(
  // direction is light_pos_dir when light is directional
  // and light_pos_dir - sample_position when positional
  direction = light_pos_dir - is_Pos * sample_position;
  norm_dir = normalize(direction);
  // introduce little offset to avoid sampling shadows at the exact
  // sample position
  sample_position += g_lengthStep * norm_dir;
  direction = light_pos_dir - is_Pos * sample_position;
  ray.origin = sample_position;
  ray.dir = norm_dir;
  safe_0_vector(ray);
  ray.invDir = 1.0/ray.dir;
  if(!BBoxIntersect(vec3(0.0), vec3(1.0), ray, hit))
  {
    // it can happen around the bounding box
    return 1.0;
  }
  if(hit.tmax < g_lengthStep)
  {
    // if we're too close to the bounding box
    return 1.0;
  }
  // in case of directional light, we want direction not to be normalized but to go
  // all the way to the bbox
  direction *= pow(hit.tmax / length(direction), 1.0 - is_Pos);
  maxdist = min(hit.tmax, length(direction));
  maxdist = min(in_giReach, maxdist);
  if(maxdist < EPSILON) return 1.0;

    )***";

  // slight imprecision for the last sample : it can be something else (less) than g_lengthStep
  // because the last step is clamped to the end of the ray
  opacityEval += "  scalar = texture3D(volume, sampled_point)[c];\n"
                 "  scalar = scalar * in_volume_scale[index][c] + in_volume_bias[index][c];\n";
  opacityEval += ComputeOpacityEvaluationCall(
    mapper, inputs, noOfComponents, independentComponents, useGradYAxis, "sampled_point", true);

  resStr += functionSignature + "{\n" + declarations + rayInit +
    R"***(
  float current_dist = 0.0;
  float current_step = g_lengthStep;
  float clamped_step = 0.0;
  while(current_dist < maxdist)
  {
    clamped_step = min(maxdist - current_dist, current_step);
    sampled_dist = current_dist + clamped_step * g_jitterValue;
    sampled_point = sample_position + sampled_dist * norm_dir;
    )***" +
    opacityEval +
    R"***(
    shadow *= 1.0 - opacity;
    current_dist += current_step;
  }
  return shadow;
}
  )***";

  return resStr;
}

//--------------------------------------------------------------------------
inline std::string ShadingDeclarationVertex(
  vtkRenderer* vtkNotUsed(ren), vtkVolumeMapper* vtkNotUsed(mapper), vtkVolume* vtkNotUsed(vol))
{
  return std::string();
}

//--------------------------------------------------------------------------
inline std::string ShadingDeclarationFragment(
  vtkRenderer* vtkNotUsed(ren), vtkVolumeMapper* mapper, vtkVolume* vtkNotUsed(vol))
{
  if (mapper->GetBlendMode() == vtkVolumeMapper::MAXIMUM_INTENSITY_BLEND)
  {
    return std::string("\
        \n bool l_firstValue;\
        \n vec4 l_maxValue;");
  }
  else if (mapper->GetBlendMode() == vtkVolumeMapper::MINIMUM_INTENSITY_BLEND)
  {
    return std::string("\
        \n bool l_firstValue;\
        \n vec4 l_minValue;");
  }
  else if (mapper->GetBlendMode() == vtkVolumeMapper::AVERAGE_INTENSITY_BLEND)
  {
    return std::string("\
        \n  uvec4 l_numSamples;\
        \n  vec4 l_avgValue;");
  }
  else if (mapper->GetBlendMode() == vtkVolumeMapper::ADDITIVE_BLEND)
  {
    return std::string("\
        \n  vec4 l_sumValue;");
  }
  else if (mapper->GetBlendMode() == vtkVolumeMapper::ISOSURFACE_BLEND)
  {
    return std::string("\
        \n  int l_initialIndex = 0;\
        \n  float l_normValues[NUMBER_OF_CONTOURS + 2];");
  }
  else
  {
    return std::string();
  }
}

//--------------------------------------------------------------------------
inline std::string ShadingInit(
  vtkRenderer* vtkNotUsed(ren), vtkVolumeMapper* mapper, vtkVolume* vtkNotUsed(vol))
{
  if (mapper->GetBlendMode() == vtkVolumeMapper::MAXIMUM_INTENSITY_BLEND)
  {
    return std::string("\
        \n  // We get data between 0.0 - 1.0 range\
        \n  l_firstValue = true;\
        \n  l_maxValue = vec4(0.0);");
  }
  else if (mapper->GetBlendMode() == vtkVolumeMapper::MINIMUM_INTENSITY_BLEND)
  {
    return std::string("\
        \n  //We get data between 0.0 - 1.0 range\
        \n  l_firstValue = true;\
        \n  l_minValue = vec4(1.0);");
  }
  else if (mapper->GetBlendMode() == vtkVolumeMapper::AVERAGE_INTENSITY_BLEND)
  {
    return std::string("\
        \n  //We get data between 0.0 - 1.0 range\
        \n  l_avgValue = vec4(0.0);\
        \n  // Keep track of number of samples\
        \n  l_numSamples = uvec4(0);");
  }
  else if (mapper->GetBlendMode() == vtkVolumeMapper::ADDITIVE_BLEND)
  {
    return std::string("\
        \n  //We get data between 0.0 - 1.0 range\
        \n  l_sumValue = vec4(0.0);");
  }
  else if (mapper->GetBlendMode() == vtkVolumeMapper::ISOSURFACE_BLEND)
  {
    return std::string("\
        \n#if NUMBER_OF_CONTOURS\
        \n  l_normValues[0] = -1e20; //-infinity\
        \n  l_normValues[NUMBER_OF_CONTOURS+1] = +1e20; //+infinity\
        \n  for (int i = 0; i < NUMBER_OF_CONTOURS; i++)\
        \n  {\
        \n    l_normValues[i+1] = (in_isosurfacesValues[i] - in_scalarsRange[0].x) / \
        \n                        (in_scalarsRange[0].y - in_scalarsRange[0].x);\
        \n  }\
        \n#endif\
        ");
  }
  else
  {
    return std::string();
  }
}

//--------------------------------------------------------------------------
inline std::string GradientCacheDec(vtkRenderer* vtkNotUsed(ren), vtkVolume* vtkNotUsed(vol),
  vtkOpenGLGPUVolumeRayCastMapper::VolumeInputMap& inputs, int independentComponents = 0)
{
  const int numInputs = static_cast<int>(inputs.size());
  const int comp = numInputs == 1 ?
                                  // Dependent components use a single opacity lut.
    (!independentComponents ? 1 : numInputs)
                                  :
                                  // Independent components not supported with multiple-inputs
    1;

  std::ostringstream toShader;
  for (const auto& item : inputs)
  {
    auto& input = item.second;
    if (input.Volume->GetProperty()->HasGradientOpacity())
    {
      toShader << "vec4 " << input.GradientCacheName << "[" << comp << "];\n";
    }
  }

  return toShader.str();
}

//--------------------------------------------------------------------------
inline std::string PreComputeGradientsImpl(vtkRenderer* vtkNotUsed(ren), vtkVolume* vtkNotUsed(vol),
  int noOfComponents = 1, int independentComponents = 0)
{
  std::ostringstream shader;
  if (independentComponents)
  {
    if (noOfComponents == 1)
    {
      shader << "g_gradients_0[0] = computeGradient(g_dataPos, 0, in_volume[0], 0);\n";
    }
    else
    {
      // Multiple components
      shader << "for (int comp = 0; comp < in_noOfComponents; comp++)\n"
                "{\n"
                "  g_gradients_0[comp] = computeGradient(g_dataPos, comp, in_volume[0], 0);\n"
                "}\n";
    }
  }
  else
  {
    shader << "g_gradients_0[0] = computeGradient(g_dataPos, 0, in_volume[0], 0);\n";
  }

  return shader.str();
}

//--------------------------------------------------------------------------
inline std::string ShadingMultipleInputs(
  vtkVolumeMapper* mapper, vtkOpenGLGPUVolumeRayCastMapper::VolumeInputMap& inputs)
{
  std::ostringstream toShaderStr;
  toShaderStr << "    if (!g_skip)\n"
                 "    {\n"
                 "      vec3 texPos;\n";

  switch (mapper->GetBlendMode())
  {
    case vtkVolumeMapper::COMPOSITE_BLEND:
    default:
    {
      int i = 0;
      for (auto& item : inputs)
      {
        auto& input = item.second;
        auto property = input.Volume->GetProperty();
        // Transformation index. Index 0 refers to the global bounding-box.
        const auto idx = i + 1;
        toShaderStr <<
          // From global texture coordinates (bbox) to volume_i texture coords.
          // texPos = T * g_dataPos
          // T = T_dataToTex1 * T_worldToData * T_bboxTexToWorld;
          "      texPos = (in_cellToPoint[" << idx << "] * in_inverseTextureDatasetMatrix[" << idx
                    << "] * in_inverseVolumeMatrix[" << idx
                    << "] *\n"
                       "        in_volumeMatrix[0] * in_textureDatasetMatrix[0] * "
                       "vec4(g_dataPos.xyz, 1.0)).xyz;\n"
                       "      if ((all(lessThanEqual(texPos, vec3(1.0))) &&\n"
                       "        all(greaterThanEqual(texPos, vec3(0.0)))))\n"
                       "      {\n"
                       "        vec4 scalar = texture3D(in_volume["
                    << i
                    << "], texPos);\n"
                       "        scalar = scalar * in_volume_scale["
                    << i << "] + in_volume_bias[" << i << "];\n";

        // MultiVolume considers input has one component when independent component is on.
        if (property->GetIndependentComponents())
        {
          toShaderStr << "        scalar = vec4(scalar.r);\n";
        }

        toShaderStr << "        g_srcColor = vec4(0.0);\n";

        if (property->GetTransferFunctionMode() == vtkVolumeProperty::TF_1D)
        {
          std::string gradientopacity_param = (property->HasGradientOpacity())
            ? input.GradientOpacityTablesMap[0] + std::string(", ")
            : std::string();

          toShaderStr << "        g_srcColor.a = computeOpacity(scalar,"
                      << input.OpacityTablesMap[0]
                      << ");\n"
                         "        if (g_srcColor.a > 0.0)\n"
                         "        {\n"
                         "          g_srcColor = computeColor(texPos, scalar, g_srcColor.a, "
                      << input.RGBTablesMap[0] << ", " << gradientopacity_param << "in_volume[" << i
                      << "], " << input.OpacityTablesMap[0] << ", " << i << ");\n";

          if (property->HasGradientOpacity())
          {
            const auto& grad = input.GradientCacheName;
            toShaderStr << "          " << grad << "[0] = computeGradient(texPos, 0, "
                        << "in_volume[" << i << "], " << i
                        << ");\n"
                           "          if ("
                        << grad
                        << "[0].w >= 0.0)\n"
                           "          {\n"
                           "            g_srcColor.a *= computeGradientOpacity("
                        << grad << "[0], " << input.GradientOpacityTablesMap[0]
                        << ");\n"
                           "          }\n";
          }
        }
        else if (property->GetTransferFunctionMode() == vtkVolumeProperty::TF_2D)
        {
          const auto& grad = input.GradientCacheName;
          toShaderStr <<
            // Sample 2DTF directly
            "        " << grad << "[0] = computeGradient(texPos, 0, "
                      << "in_volume[" << i << "], " << i
                      << ");\n"
                         "        g_srcColor = texture2D("
                      << input.TransferFunctions2DMap[0] << ", vec2(scalar.r, "
                      << input.GradientCacheName
                      << "[0].w));\n"
                         "        if (g_srcColor.a > 0.0)\n"
                         "        {\n";
        }

        toShaderStr
          << "          g_srcColor.rgb *= g_srcColor.a;\n"
             "          g_fragColor = (1.0f - g_fragColor.a) * g_srcColor + g_fragColor;\n"
             "        }\n"
             "      }\n\n";

        i++;
      }
    }
    break;
  }
  toShaderStr << "    }\n";

  return toShaderStr.str();
}

//--------------------------------------------------------------------------
inline std::string ShadingSingleInput(vtkRenderer* vtkNotUsed(ren), vtkVolumeMapper* mapper,
  vtkVolume* vtkNotUsed(vol), vtkImageData* maskInput, vtkVolumeTexture* mask, int maskType,
  int noOfComponents, int independentComponents = 0)
{
  auto glMapper = vtkOpenGLGPUVolumeRayCastMapper::SafeDownCast(mapper);

  std::string shaderStr;

  shaderStr += std::string("\
      \n    if (!g_skip)\
      \n      {\
      \n      vec4 scalar;\
      \n");
  if (vtkRectilinearGrid::SafeDownCast(mapper->GetInput()))
  {
    shaderStr += std::string("\
      \n      // Compute IJK vertex position for current sample in the rectilinear grid\
      \n      vec4 dataPosWorld = in_volumeMatrix[0] * in_textureDatasetMatrix[0] * vec4(g_dataPos, 1.0);\
      \n      dataPosWorld = dataPosWorld / dataPosWorld.w;\
      \n      dataPosWorld.w = 1.0;\
      \n      ivec3 ijk = ivec3(0);\
      \n      vec3 ijkTexCoord = vec3(0.0);\
      \n      vec3 pCoords = vec3(0.0);\
      \n      vec3 xPrev, xNext, tmp;\
      \n      int sz = textureSize(in_coordTexs, 0);\
      \n      vec4 dataPosWorldScaled = dataPosWorld * vec4(in_coordsScale, 1.0) +\
      \n                                vec4(in_coordsBias, 1.0);\
      \n      for (int j = 0; j < 3; ++j)\
      \n        {\
      \n        xPrev = texture1D(in_coordTexs, 0.0).xyz;\
      \n        xNext = texture1D(in_coordTexs, (in_coordTexSizes[j] - 1) / sz).xyz;\
      \n        if (xNext[j] < xPrev[j])\
      \n          {\
      \n          tmp = xNext;\
      \n          xNext = xPrev;\
      \n          xPrev = tmp;\
      \n          }\
      \n        for (int i = 0; i < int(in_coordTexSizes[j]); i++)\
      \n          {\
      \n          xNext = texture1D(in_coordTexs, (i + 0.5) / sz).xyz;\
      \n          if (dataPosWorldScaled[j] >= xPrev[j] && dataPosWorldScaled[j] < xNext[j])\
      \n            {\
      \n            ijk[j] = i - 1;\
      \n            pCoords[j] = (dataPosWorldScaled[j] - xPrev[j]) / (xNext[j] - xPrev[j]);\
      \n            break;\
      \n            }\
      \n          else if (dataPosWorldScaled[j] == xNext[j])\
      \n            {\
      \n            ijk[j] = i - 1;\
      \n            pCoords[j] = 1.0;\
      \n            break;\
      \n            }\
      \n          xPrev = xNext;\
      \n          }\
      \n        ijkTexCoord[j] = (ijk[j] + pCoords[j]) / in_coordTexSizes[j];\
      \n        }\
      \n      scalar = texture3D(in_volume[0], sign(in_cellSpacing[0]) * ijkTexCoord);\
      \n");
  }
  else
  {
    shaderStr += std::string("\
      \n      scalar = texture3D(in_volume[0], g_dataPos);\
      \n");
  }

  // simulate old intensity textures
  if (noOfComponents == 1)
  {
    shaderStr += std::string("\
        \n      scalar.r = scalar.r * in_volume_scale[0].r + in_volume_bias[0].r;\
        \n      scalar = vec4(scalar.r);");
  }
  else
  {
    // handle bias and scale
    shaderStr += std::string("\
        \n      scalar = scalar * in_volume_scale[0] + in_volume_bias[0];");
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
            \n        }");
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
           \n        }");
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
          \n        }");
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
            \n        }");
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
          \n        }");
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
          \n        }");
    }
  }
  else if (mapper->GetBlendMode() == vtkVolumeMapper::AVERAGE_INTENSITY_BLEND)
  {
    if (noOfComponents > 1 && independentComponents)
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
        \n           l_avgValue[i] += computeOpacity(scalar, i) * scalar[i];\
        \n           ++l_numSamples[i];\
        \n           }\
        \n         }");
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
        \n        l_avgValue.x += computeOpacity(scalar) * scalar.x;\
        \n        ++l_numSamples.x;\
        \n        }");
    }
  }
  else if (mapper->GetBlendMode() == vtkVolumeMapper::ADDITIVE_BLEND)
  {
    if (noOfComponents > 1 && independentComponents)
    {
      shaderStr += std::string("\
        \n       for (int i = 0; i < in_noOfComponents; ++i)\
        \n         {\
        \n         float opacity = computeOpacity(scalar, i);\
        \n         l_sumValue[i] = l_sumValue[i] + opacity * scalar[i];\
        \n         }");
    }
    else
    {
      shaderStr += std::string("\
          \n      float opacity = computeOpacity(scalar);\
          \n      l_sumValue.x = l_sumValue.x + opacity * scalar.x;");
    }
  }
  else if (mapper->GetBlendMode() == vtkVolumeMapper::ISOSURFACE_BLEND)
  {
    shaderStr += std::string("\
        \n#if NUMBER_OF_CONTOURS\
        \n    int maxComp = 0;");

    std::string compParamStr;
    if (noOfComponents > 1 && independentComponents)
    {
      shaderStr += std::string("\
          \n    for (int i = 1; i < in_noOfComponents; ++i)\
          \n    {\
          \n      if (in_componentWeight[i] > in_componentWeight[maxComp])\
          \n        maxComp = i;\
          \n    }");
      compParamStr = ", maxComp";
    }
    shaderStr += std::string("\
        \n    if (g_currentT == 0)\
        \n    {\
        \n      l_initialIndex = findIsoSurfaceIndex(scalar[maxComp], l_normValues);\
        \n    }\
        \n    else\
        \n    {\
        \n      float s;\
        \n      bool shade = false;\
        \n      l_initialIndex = clamp(l_initialIndex, 0, NUMBER_OF_CONTOURS);\
        \n      if (scalar[maxComp] < l_normValues[l_initialIndex])\
        \n      {\
        \n        s = l_normValues[l_initialIndex];\
        \n        l_initialIndex--;\
        \n        shade = true;\
        \n      }\
        \n      if (scalar[maxComp] > l_normValues[l_initialIndex+1])\
        \n      {\
        \n        s = l_normValues[l_initialIndex+1];\
        \n        l_initialIndex++;\
        \n        shade = true;\
        \n      }\
        \n      if (shade == true)\
        \n      {\
        \n        vec4 vs = vec4(s);\
        \n        g_srcColor.a = computeOpacity(vs " +
      compParamStr + ");\
        \n        g_srcColor = computeColor(vs, g_srcColor.a " +
      compParamStr + ");\
        \n        g_srcColor.rgb *= g_srcColor.a;\
        \n        g_fragColor = (1.0f - g_fragColor.a) * g_srcColor + g_fragColor;\
        \n      }\
        \n    }\
        \n#endif");
  }
  else if (mapper->GetBlendMode() == vtkVolumeMapper::SLICE_BLEND)
  {
    shaderStr += std::string("\
        \n    // test if the intersection is inside the volume bounds\
        \n    if (any(greaterThan(g_dataPos, vec3(1.0))) || any(lessThan(g_dataPos, vec3(0.0))))\
        \n    {\
        \n      discard;\
        \n    }\
        \n    float opacity = computeOpacity(scalar);\
        \n    g_fragColor = computeColor(scalar, opacity);\
        \n    g_fragColor.rgb *= opacity;\
        \n    g_exit = true;");
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
      if (glMapper->GetUseDepthPass() &&
        glMapper->GetCurrentPass() == vtkOpenGLGPUVolumeRayCastMapper::DepthPass)
      {
        shaderStr += std::string("\
            \n        // Data fetching from the red channel of volume texture\
            \n        float opacity = computeOpacity(scalar, i);\
            \n        if (opacity > 0.0)\
            \n          {\
            \n          g_srcColor.a = opacity;\
            \n          }\
            \n       }");
      }
      else if (!mask || !maskInput || maskType != vtkGPUVolumeRayCastMapper::LabelMapMaskType)
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
          \n      g_fragColor = (1.0f - g_fragColor.a) * tmp + g_fragColor;");
      }
    }
    else if (glMapper->GetUseDepthPass() &&
      glMapper->GetCurrentPass() == vtkOpenGLGPUVolumeRayCastMapper::DepthPass)
    {
      shaderStr += std::string("\
          \n      g_srcColor = vec4(0.0);\
          \n      g_srcColor.a = computeOpacity(scalar);");
    }
    else
    {
      if (!mask || !maskInput || maskType != vtkGPUVolumeRayCastMapper::LabelMapMaskType)
      {
        shaderStr += std::string("\
             \n      g_srcColor = vec4(0.0);\
             \n      g_srcColor.a = computeOpacity(scalar);\
             \n      if (g_srcColor.a > 0.0)\
             \n        {\
             \n        g_srcColor = computeColor(scalar, g_srcColor.a);");
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
           \n        g_fragColor = (1.0f - g_fragColor.a) * g_srcColor + g_fragColor;");

      if (!mask || !maskInput || maskType != vtkGPUVolumeRayCastMapper::LabelMapMaskType)
      {
        shaderStr += std::string("\
             \n        }");
      }
    }
  }
  else
  {
    shaderStr += std::string();
  }

  shaderStr += std::string("\
      \n      }");
  return shaderStr;
}

//--------------------------------------------------------------------------
inline std::string PickingActorPassExit(
  vtkRenderer* vtkNotUsed(ren), vtkVolumeMapper* vtkNotUsed(mapper), vtkVolume* vtkNotUsed(vol))
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
}

//--------------------------------------------------------------------------
inline std::string PickingIdLow24PassExit(
  vtkRenderer* vtkNotUsed(ren), vtkVolumeMapper* vtkNotUsed(mapper), vtkVolume* vtkNotUsed(vol))
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
}

//--------------------------------------------------------------------------
inline std::string PickingIdHigh24PassExit(
  vtkRenderer* vtkNotUsed(ren), vtkVolumeMapper* vtkNotUsed(mapper), vtkVolume* vtkNotUsed(vol))
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
}

//--------------------------------------------------------------------------
inline std::string ShadingExit(vtkRenderer* vtkNotUsed(ren), vtkVolumeMapper* mapper,
  vtkVolume* vtkNotUsed(vol), int noOfComponents, int independentComponents = 0)
{
  vtkOpenGLGPUVolumeRayCastMapper* glMapper = vtkOpenGLGPUVolumeRayCastMapper::SafeDownCast(mapper);

  if (glMapper->GetUseDepthPass() &&
    glMapper->GetCurrentPass() == vtkOpenGLGPUVolumeRayCastMapper::DepthPass &&
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
          \n   g_fragColor = g_srcColor;");
    }
    else
    {
      return std::string("\
         \n  g_srcColor = computeColor(l_maxValue,\
         \n                            computeOpacity(l_maxValue));\
         \n  g_fragColor.rgb = g_srcColor.rgb * g_srcColor.a;\
         \n  g_fragColor.a = g_srcColor.a;");
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
          \n  g_fragColor = g_srcColor;");
    }
    else
    {
      return std::string("\
          \n  g_srcColor = computeColor(l_minValue,\
          \n                            computeOpacity(l_minValue));\
          \n  g_fragColor.rgb = g_srcColor.rgb * g_srcColor.a;\
          \n  g_fragColor.a = g_srcColor.a;");
    }
  }
  else if (mapper->GetBlendMode() == vtkVolumeMapper::AVERAGE_INTENSITY_BLEND)
  {
    if (noOfComponents > 1 && independentComponents)
    {
      return std::string("\
          \n  for (int i = 0; i < in_noOfComponents; ++i)\
          \n    {\
          \n    if (l_numSamples[i] == uint(0))\
          \n      {\
          \n      continue;\
          \n      }\
          \n    l_avgValue[i] = l_avgValue[i] * in_componentWeight[i] /\
          \n                    l_numSamples[i];\
          \n    if (i > 0)\
          \n      {\
          \n      l_avgValue[0] += l_avgValue[i];\
          \n      }\
          \n    }\
          \n  l_avgValue[0] = clamp(l_avgValue[0], 0.0, 1.0);\
          \n  g_fragColor = vec4(vec3(l_avgValue[0]), 1.0);");
    }
    else
    {
      return std::string("\
         \n  if (l_numSamples.x == uint(0))\
         \n    {\
         \n    discard;\
         \n    }\
         \n  else\
         \n    {\
         \n    l_avgValue.x /= l_numSamples.x;\
         \n    l_avgValue.x = clamp(l_avgValue.x, 0.0, 1.0);\
         \n    g_fragColor = vec4(vec3(l_avgValue.x), 1.0);\
         \n    }");
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
          \n  g_fragColor = vec4(vec3(l_sumValue.x), 1.0);");
    }
    else
    {
      return std::string("\
          \n  l_sumValue.x = clamp(l_sumValue.x, 0.0, 1.0);\
          \n  g_fragColor = vec4(vec3(l_sumValue.x), 1.0);");
    }
  }
  else
  {
    return std::string();
  }
}

//--------------------------------------------------------------------------
inline std::string TerminationDeclarationVertex(
  vtkRenderer* vtkNotUsed(ren), vtkVolumeMapper* vtkNotUsed(mapper), vtkVolume* vtkNotUsed(vol))
{
  return std::string();
}

//--------------------------------------------------------------------------
inline std::string TerminationDeclarationFragment(
  vtkRenderer* vtkNotUsed(ren), vtkVolumeMapper* vtkNotUsed(mapper), vtkVolume* vtkNotUsed(vol))
{
  return std::string("\
      \n const float g_opacityThreshold = 1.0 - 1.0 / 255.0;");
}

//--------------------------------------------------------------------------
inline std::string PickingActorPassDeclaration(
  vtkRenderer* vtkNotUsed(ren), vtkVolumeMapper* vtkNotUsed(mapper), vtkVolume* vtkNotUsed(vol))
{
  return std::string("\
      \n  uniform vec3 in_propId;");
}

//--------------------------------------------------------------------------
inline std::string TerminationInit(
  vtkRenderer* vtkNotUsed(ren), vtkVolumeMapper* mapper, vtkVolume* vol)
{
  std::string shaderStr;
  shaderStr += std::string("\
      \n  // Flag to indicate if the raymarch loop should terminate \
      \n  bool stop = false;\
      \n\
      \n  g_terminatePointMax = 0.0;\
      \n\
      \n  vec4 l_depthValue = texture2D(in_depthSampler, fragTexCoord);\
      \n  // Depth test\
      \n  if(gl_FragCoord.z >= l_depthValue.x)\
      \n    {\
      \n    discard;\
      \n    }\
      \n\
      \n  // color buffer or max scalar buffer have a reduced size.\
      \n  fragTexCoord = (gl_FragCoord.xy - in_windowLowerLeftCorner) *\
      \n                 in_inverseOriginalWindowSize;\
      \n");

  if (mapper->GetBlendMode() == vtkVolumeMapper::SLICE_BLEND)
  {
    vtkImplicitFunction* sliceFunc = vol->GetProperty()->GetSliceFunction();
    if (sliceFunc)
    {
      if (sliceFunc->IsA("vtkPlane"))
      {
        shaderStr += std::string("\
          \n\
          \n  // Intersection with plane\
          \n  float t = intersectRayPlane(ip_vertexPos, rayDir);\
          \n  vec4 intersection = vec4(ip_vertexPos + t * rayDir, 1.0);\
          \n  g_intersection = (in_inverseTextureDatasetMatrix[0] * intersection).xyz;\
          \n  vec4 intersDC = in_projectionMatrix * in_modelViewMatrix * in_volumeMatrix[0] * intersection;\
          \n  intersDC.xyz /= intersDC.w;\
          \n  vec4 intersWin = NDCToWindow(intersDC.x, intersDC.y, intersDC.z);\
          \n  if(intersWin.z >= l_depthValue.x)\
          \n  {\
          \n    discard;\
          \n  }\
          \n");
      }
      else
      {
        vtkErrorWithObjectMacro(
          sliceFunc, "Implicit function type is not supported by this mapper.");
      }
    }
  }

  shaderStr += std::string("\
      \n  // Compute max number of iterations it will take before we hit\
      \n  // the termination point\
      \n\
      \n  // Abscissa of the point on the depth buffer along the ray.\
      \n  // point in texture coordinates\
      \n  vec4 rayTermination = WindowToNDC(gl_FragCoord.x, gl_FragCoord.y, l_depthValue.x);\
      \n\
      \n  // From normalized device coordinates to eye coordinates.\
      \n  // in_projectionMatrix is inversed because of way VT\
      \n  // From eye coordinates to texture coordinates\
      \n  rayTermination = ip_inverseTextureDataAdjusted *\
      \n                    in_inverseVolumeMatrix[0] *\
      \n                    in_inverseModelViewMatrix *\
      \n                    in_inverseProjectionMatrix *\
      \n                    rayTermination;\
      \n  g_rayTermination = rayTermination.xyz / rayTermination.w;\
      \n\
      \n  // Setup the current segment:\
      \n  g_dataPos = g_rayOrigin;\
      \n  g_terminatePos = g_rayTermination;\
      \n\
      \n  g_terminatePointMax = length(g_terminatePos.xyz - g_dataPos.xyz) /\
      \n                        length(g_dirStep);\
      \n  g_currentT = 0.0;");

  return shaderStr;
}

//--------------------------------------------------------------------------
inline std::string TerminationImplementation(
  vtkRenderer* vtkNotUsed(ren), vtkVolumeMapper* vtkNotUsed(mapper), vtkVolume* vtkNotUsed(vol))
{
  return std::string("\
      \n    if(any(greaterThan(max(g_dirStep, vec3(0.0))*(g_dataPos - in_texMax[0]),vec3(0.0))) ||\
      \n      any(greaterThan(min(g_dirStep, vec3(0.0))*(g_dataPos - in_texMin[0]),vec3(0.0))))\
      \n      {\
      \n      break;\
      \n      }\
      \n\
      \n    // Early ray termination\
      \n    // if the currently composited colour alpha is already fully saturated\
      \n    // we terminated the loop or if we have hit an obstacle in the\
      \n    // direction of they ray (using depth buffer) we terminate as well.\
      \n    if((g_fragColor.a > g_opacityThreshold) || \
      \n       g_currentT >= g_terminatePointMax)\
      \n      {\
      \n      break;\
      \n      }\
      \n    ++g_currentT;");
}

//--------------------------------------------------------------------------
inline std::string TerminationExit(
  vtkRenderer* vtkNotUsed(ren), vtkVolumeMapper* vtkNotUsed(mapper), vtkVolume* vtkNotUsed(vol))
{
  return std::string();
}

//--------------------------------------------------------------------------
inline std::string CroppingDeclarationVertex(
  vtkRenderer* vtkNotUsed(ren), vtkVolumeMapper* vtkNotUsed(mapper), vtkVolume* vtkNotUsed(vol))
{
  return std::string();
}

//--------------------------------------------------------------------------
inline std::string CroppingDeclarationFragment(
  vtkRenderer* vtkNotUsed(ren), vtkVolumeMapper* mapper, vtkVolume* vtkNotUsed(vol))
{
  if (!mapper->GetCropping())
  {
    return std::string();
  }

  return std::string("\
      \nuniform float in_croppingPlanes[6];\
      \nuniform int in_croppingFlags [32];\
      \nfloat croppingPlanesTexture[6];\
      \n\
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
      \n  }");
}

//--------------------------------------------------------------------------
inline std::string CroppingInit(
  vtkRenderer* vtkNotUsed(ren), vtkVolumeMapper* mapper, vtkVolume* vtkNotUsed(vol))
{
  if (!mapper->GetCropping())
  {
    return std::string();
  }

  return std::string("\
      \n  // Convert cropping region to texture space\
      \n  mat4  datasetToTextureMat = in_inverseTextureDatasetMatrix[0];\
      \n\
      \n  vec4 tempCrop = vec4(in_croppingPlanes[0], 0.0, 0.0, 1.0);\
      \n  tempCrop = datasetToTextureMat * tempCrop;\
      \n  if (tempCrop[3] != 0.0)\
      \n   {\
      \n   tempCrop[0] /= tempCrop[3];\
      \n   }\
      \n  croppingPlanesTexture[0] = tempCrop[0];\
      \n\
      \n  tempCrop = vec4(in_croppingPlanes[1], 0.0, 0.0, 1.0);\
      \n  tempCrop = datasetToTextureMat * tempCrop;\
      \n  if (tempCrop[3] != 0.0)\
      \n   {\
      \n   tempCrop[0] /= tempCrop[3];\
      \n   }\
      \n  croppingPlanesTexture[1] = tempCrop[0];\
      \n\
      \n  tempCrop = vec4(0.0, in_croppingPlanes[2], 0.0, 1.0);\
      \n  tempCrop = datasetToTextureMat * tempCrop;\
      \n  if (tempCrop[3] != 0.0)\
      \n   {\
      \n   tempCrop[1] /= tempCrop[3];\
      \n   }\
      \n  croppingPlanesTexture[2] = tempCrop[1];\
      \n\
      \n  tempCrop = vec4(0.0, in_croppingPlanes[3], 0.0, 1.0);\
      \n  tempCrop = datasetToTextureMat * tempCrop;\
      \n  if (tempCrop[3] != 0.0)\
      \n   {\
      \n   tempCrop[1] /= tempCrop[3];\
      \n   }\
      \n  croppingPlanesTexture[3] = tempCrop[1];\
      \n\
      \n  tempCrop = vec4(0.0, 0.0, in_croppingPlanes[4], 1.0);\
      \n  tempCrop = datasetToTextureMat * tempCrop;\
      \n  if (tempCrop[3] != 0.0)\
      \n   {\
      \n   tempCrop[2] /= tempCrop[3];\
      \n   }\
      \n  croppingPlanesTexture[4] = tempCrop[2];\
      \n\
      \n  tempCrop = vec4(0.0, 0.0, in_croppingPlanes[5], 1.0);\
      \n  tempCrop = datasetToTextureMat * tempCrop;\
      \n  if (tempCrop[3] != 0.0)\
      \n   {\
      \n   tempCrop[2] /= tempCrop[3];\
      \n   }\
      \n  croppingPlanesTexture[5] = tempCrop[2];");
}

//--------------------------------------------------------------------------
inline std::string CroppingImplementation(
  vtkRenderer* vtkNotUsed(ren), vtkVolumeMapper* mapper, vtkVolume* vtkNotUsed(vol))
{
  if (!mapper->GetCropping())
  {
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
      \n      g_skip = true;\
      \n      }");
}

//--------------------------------------------------------------------------
inline std::string CroppingExit(
  vtkRenderer* vtkNotUsed(ren), vtkVolumeMapper* vtkNotUsed(mapper), vtkVolume* vtkNotUsed(vol))
{
  return std::string();
}

//--------------------------------------------------------------------------
inline std::string ClippingDeclarationVertex(
  vtkRenderer* vtkNotUsed(ren), vtkVolumeMapper* vtkNotUsed(mapper), vtkVolume* vtkNotUsed(vol))
{
  return std::string();
}

//--------------------------------------------------------------------------
inline std::string ClippingDeclarationFragment(
  vtkRenderer* vtkNotUsed(ren), vtkVolumeMapper* mapper, vtkVolume* vtkNotUsed(vol))
{
  if (!mapper->GetClippingPlanes())
  {
    return std::string();
  }

  return std::string("\
      \n /// We support only 8 clipping planes for now\
      \n /// The first value is the size of the data array for clipping\
      \n /// planes (origin, normal)\
      \n uniform float in_clippingPlanes[49];\
      \n uniform float in_clippedVoxelIntensity;\
      \n\
      \n int clip_numPlanes;\
      \n vec3 clip_rayDirObj;\
      \n mat4 clip_texToObjMat;\
      \n mat4 clip_objToTexMat;\
      \n\
      \n// Tighten the sample range as needed to account for clip planes. \
      \n// Arguments are in texture coordinates. \
      \n// Returns true if the range is at all valid after clipping. If not, \
      \n// the fragment should be discarded. \
      \nbool AdjustSampleRangeForClipping(inout vec3 startPosTex, inout vec3 stopPosTex) \
      \n{ \
      \n  vec4 startPosObj = vec4(0.0);\
      \n  {\
      \n    startPosObj = clip_texToObjMat * vec4(startPosTex - g_rayJitter, 1.0);\
      \n    startPosObj = startPosObj / startPosObj.w;\
      \n    startPosObj.w = 1.0;\
      \n  }\
      \n\
      \n  vec4 stopPosObj = vec4(0.0);\
      \n  {\
      \n    stopPosObj = clip_texToObjMat * vec4(stopPosTex, 1.0);\
      \n    stopPosObj = stopPosObj / stopPosObj.w;\
      \n    stopPosObj.w = 1.0;\
      \n  }\
      \n\
      \n  for (int i = 0; i < clip_numPlanes; i = i + 6)\
      \n  {\
      \n    vec3 planeOrigin = vec3(in_clippingPlanes[i + 1],\
      \n                            in_clippingPlanes[i + 2],\
      \n                            in_clippingPlanes[i + 3]);\
      \n    vec3 planeNormal = normalize(vec3(in_clippingPlanes[i + 4],\
      \n                                      in_clippingPlanes[i + 5],\
      \n                                      in_clippingPlanes[i + 6]));\
      \n\
      \n    // Abort if the entire segment is clipped:\
      \n    // (We can do this before adjusting the term point, since it'll \
      \n    // only move further into the clipped area)\
      \n    float startDistance = dot(planeNormal, planeOrigin - startPosObj.xyz);\
      \n    float stopDistance = dot(planeNormal, planeOrigin - stopPosObj.xyz);\
      \n    bool startClipped = startDistance > 0.0;\
      \n    bool stopClipped = stopDistance > 0.0;\
      \n    if (startClipped && stopClipped)\
      \n    {\
      \n      return false;\
      \n    }\
      \n\
      \n    float rayDotNormal = dot(clip_rayDirObj, planeNormal);\
      \n    bool frontFace = rayDotNormal > 0.0;\
      \n\
      \n    // Move the start position further from the eye if needed:\
      \n    if (frontFace && // Observing from the clipped side (plane's front face)\
      \n        startDistance > 0.0) // Ray-entry lies on the clipped side.\
      \n    {\
      \n      // Scale the point-plane distance to the ray direction and update the\
      \n      // entry point.\
      \n      float rayScaledDist = startDistance / rayDotNormal;\
      \n      startPosObj = vec4(startPosObj.xyz + rayScaledDist * clip_rayDirObj, 1.0);\
      \n      vec4 newStartPosTex = clip_objToTexMat * vec4(startPosObj.xyz, 1.0);\
      \n      newStartPosTex /= newStartPosTex.w;\
      \n      startPosTex = newStartPosTex.xyz;\
      \n      startPosTex += g_rayJitter;\
      \n    }\
      \n\
      \n    // Move the end position closer to the eye if needed:\
      \n    if (!frontFace && // Observing from the unclipped side (plane's back face)\
      \n        stopDistance > 0.0) // Ray-entry lies on the unclipped side.\
      \n    {\
      \n      // Scale the point-plane distance to the ray direction and update the\
      \n      // termination point.\
      \n      float rayScaledDist = stopDistance / rayDotNormal;\
      \n      stopPosObj = vec4(stopPosObj.xyz + rayScaledDist * clip_rayDirObj, 1.0);\
      \n      vec4 newStopPosTex = clip_objToTexMat * vec4(stopPosObj.xyz, 1.0);\
      \n      newStopPosTex /= newStopPosTex.w;\
      \n      stopPosTex = newStopPosTex.xyz;\
      \n    }\
      \n  }\
      \n\
      \n  if (any(greaterThan(startPosTex, in_texMax[0])) ||\
      \n      any(lessThan(startPosTex, in_texMin[0])))\
      \n  {\
      \n    return false;\
      \n  }\
      \n\
      \n  return true;\
      \n}\
      \n");
}

//--------------------------------------------------------------------------
inline std::string ClippingInit(
  vtkRenderer* ren, vtkVolumeMapper* mapper, vtkVolume* vtkNotUsed(vol))
{
  if (!mapper->GetClippingPlanes())
  {
    return std::string();
  }

  std::string shaderStr;
  if (!ren->GetActiveCamera()->GetParallelProjection())
  {
    shaderStr = std::string("\
        \n  vec4 tempClip = in_volumeMatrix[0] * vec4(rayDir, 0.0);\
        \n  if (tempClip.w != 0.0)\
        \n  {\
        \n    tempClip = tempClip/tempClip.w;\
        \n    tempClip.w = 1.0;\
        \n  }\
        \n  clip_rayDirObj = normalize(tempClip.xyz);");
  }
  else
  {
    shaderStr = std::string("\
        clip_rayDirObj = normalize(in_projectionDirection);");
  }

  shaderStr += std::string("\
      \n  clip_numPlanes = int(in_clippingPlanes[0]);\
      \n  clip_texToObjMat = in_volumeMatrix[0] * inverse(ip_inverseTextureDataAdjusted);\
      \n  clip_objToTexMat = ip_inverseTextureDataAdjusted * in_inverseVolumeMatrix[0];\
      \n\
      \n  // Adjust for clipping.\
      \n  if (!AdjustSampleRangeForClipping(g_rayOrigin, g_rayTermination))\
      \n  { // entire ray is clipped.\
      \n    discard;\
      \n  }\
      \n\
      \n  // Update the segment post-clip:\
      \n  g_dataPos = g_rayOrigin;\
      \n  g_terminatePos = g_rayTermination;\
      \n  g_terminatePointMax = length(g_terminatePos.xyz - g_dataPos.xyz) /\
      \n                        length(g_dirStep);\
      \n");

  return shaderStr;
}

//--------------------------------------------------------------------------
inline std::string ClippingImplementation(
  vtkRenderer* vtkNotUsed(ren), vtkVolumeMapper* vtkNotUsed(mapper), vtkVolume* vtkNotUsed(vol))
{
  return std::string();
}

//--------------------------------------------------------------------------
inline std::string ClippingExit(
  vtkRenderer* vtkNotUsed(ren), vtkVolumeMapper* vtkNotUsed(mapper), vtkVolume* vtkNotUsed(vol))
{
  return std::string();
}

//--------------------------------------------------------------------------
inline std::string BinaryMaskDeclaration(vtkRenderer* vtkNotUsed(ren),
  vtkVolumeMapper* vtkNotUsed(mapper), vtkVolume* vtkNotUsed(vol), vtkImageData* maskInput,
  vtkVolumeTexture* mask, int vtkNotUsed(maskType))
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
inline std::string BinaryMaskImplementation(vtkRenderer* vtkNotUsed(ren),
  vtkVolumeMapper* vtkNotUsed(mapper), vtkVolume* vtkNotUsed(vol), vtkImageData* maskInput,
  vtkVolumeTexture* mask, int maskType)
{
  if (!mask || !maskInput || maskType == vtkGPUVolumeRayCastMapper::LabelMapMaskType)
  {
    return std::string();
  }
  else
  {
    return std::string("\
        \nvec4 maskValue = texture3D(in_mask, g_dataPos);\
        \nif(maskValue.r <= 0.0)\
        \n  {\
        \n  g_skip = true;\
        \n  }");
  }
}

//--------------------------------------------------------------------------
inline std::string CompositeMaskDeclarationFragment(vtkRenderer* vtkNotUsed(ren),
  vtkVolumeMapper* vtkNotUsed(mapper), vtkVolume* vtkNotUsed(vol), vtkImageData* maskInput,
  vtkVolumeTexture* mask, int maskType)
{
  if (!mask || !maskInput || maskType != vtkGPUVolumeRayCastMapper::LabelMapMaskType)
  {
    return std::string();
  }
  else
  {
    return std::string("\
        \nuniform float in_maskBlendFactor;\
        \nuniform sampler2D in_labelMapTransfer;\
        \nuniform float in_mask_scale;\
        \nuniform float in_mask_bias;\
        \nuniform int in_labelMapNumLabels;\
        \n");
  }
}

//--------------------------------------------------------------------------
inline std::string CompositeMaskImplementation(vtkRenderer* vtkNotUsed(ren),
  vtkVolumeMapper* vtkNotUsed(mapper), vtkVolume* vtkNotUsed(vol), vtkImageData* maskInput,
  vtkVolumeTexture* mask, int maskType, int noOfComponents)
{
  if (!mask || !maskInput || maskType != vtkGPUVolumeRayCastMapper::LabelMapMaskType)
  {
    return std::string();
  }
  else
  {
    std::string shaderStr = std::string("\
        \nvec4 scalar = texture3D(in_volume[0], g_dataPos);");

    // simulate old intensity textures
    if (noOfComponents == 1)
    {
      shaderStr += std::string("\
          \n      scalar.r = scalar.r * in_volume_scale[0].r + in_volume_bias[0].r;\
          \n      scalar = vec4(scalar.r);");
    }
    else
    {
      // handle bias and scale
      shaderStr += std::string("\
          \n      scalar = scalar * in_volume_scale[0] + in_volume_bias[0];");
    }

    // Assumeing single component scalar for label texture lookup.
    // This can be extended to composite color obtained from all components
    // in the scalar array.
    return shaderStr + std::string("\
        \nif (in_maskBlendFactor == 0.0)\
        \n  {\
        \n  g_srcColor.a = computeOpacity(scalar);\
        \n  if (g_srcColor.a > 0)\
        \n    {\
        \n    g_srcColor = computeColor(scalar, g_srcColor.a);\
        \n    }\
        \n  }\
        \nelse\
        \n  {\
        \n  float opacity = computeOpacity(scalar);\
        \n  // Get the mask value at this same location\
        \n  vec4 maskValue = texture3D(in_mask, g_dataPos);\
        \n  maskValue.r = maskValue.r * in_mask_scale + in_mask_bias;\
        \n  // Quantize the height of the labelmap texture over number of labels\
        \n  if (in_labelMapNumLabels > 0)\
        \n    {\
        \n    maskValue.r =\
        \n      floor(maskValue.r * in_labelMapNumLabels) /\
        \n      in_labelMapNumLabels;\
        \n    }\
        \n  else\
        \n    {\
        \n    maskValue.r = 0.0;\
        \n    }\
        \n  if(maskValue.r == 0.0)\
        \n    {\
        \n    g_srcColor.a = opacity;\
        \n    if (g_srcColor.a > 0)\
        \n      {\
        \n      g_srcColor = computeColor(scalar, g_srcColor.a);\
        \n      }\
        \n    }\
        \n  else\
        \n    {\
        \n    g_srcColor = texture2D(in_labelMapTransfer,\
        \n                           vec2(scalar.r, maskValue.r));\
        \n    if (g_srcColor.a > 0)\
        \n      {\
        \n      g_srcColor = computeLighting(g_srcColor, 0, maskValue.r);\
        \n      }\
        \n    if (in_maskBlendFactor < 1.0)\
        \n      {\
        \n      vec4 color = opacity > 0 ? computeColor(scalar, opacity) : vec4(0);\
        \n      g_srcColor = (1.0 - in_maskBlendFactor) * color +\
        \n                           in_maskBlendFactor * g_srcColor;\
        \n      }\
        \n    }\
        \n  }");
  }
}

//--------------------------------------------------------------------------
inline std::string RenderToImageDeclarationFragment(
  vtkRenderer* vtkNotUsed(ren), vtkVolumeMapper* vtkNotUsed(mapper), vtkVolume* vtkNotUsed(vol))
{
  return std::string("uniform bool in_clampDepthToBackface;\n"
                     "vec3 l_opaqueFragPos;\n"
                     "bool l_updateDepth;\n");
}

//--------------------------------------------------------------------------
inline std::string RenderToImageInit(
  vtkRenderer* vtkNotUsed(ren), vtkVolumeMapper* vtkNotUsed(mapper), vtkVolume* vtkNotUsed(vol))
{
  return std::string("\
    \n  l_opaqueFragPos = vec3(-1.0);\
    \n  if(in_clampDepthToBackface)\
    \n    {\
    \n    l_opaqueFragPos = g_dataPos;\
    \n    }\
    \n  l_updateDepth = true;");
}

//--------------------------------------------------------------------------
inline std::string RenderToImageImplementation(
  vtkRenderer* vtkNotUsed(ren), vtkVolumeMapper* vtkNotUsed(mapper), vtkVolume* vtkNotUsed(vol))
{
  return std::string("\
    \n    if(!g_skip && g_srcColor.a > 0.0 && l_updateDepth)\
    \n      {\
    \n      l_opaqueFragPos = g_dataPos;\
    \n      l_updateDepth = false;\
    \n      }");
}

//--------------------------------------------------------------------------
inline std::string RenderToImageExit(
  vtkRenderer* vtkNotUsed(ren), vtkVolumeMapper* vtkNotUsed(mapper), vtkVolume* vtkNotUsed(vol))
{
  return std::string("\
    \n  if (l_opaqueFragPos == vec3(-1.0))\
    \n    {\
    \n    gl_FragData[1] = vec4(1.0);\
    \n    }\
    \n  else\
    \n    {\
    \n    vec4 depthValue = in_projectionMatrix * in_modelViewMatrix *\
    \n                      in_volumeMatrix[0] * in_textureDatasetMatrix[0] *\
    \n                      vec4(l_opaqueFragPos, 1.0);\
    \n    depthValue /= depthValue.w;\
    \n    gl_FragData[1] = vec4(vec3(0.5 * (gl_DepthRange.far -\
    \n                       gl_DepthRange.near) * depthValue.z + 0.5 *\
    \n                      (gl_DepthRange.far + gl_DepthRange.near)), 1.0);\
    \n    }");
}

//--------------------------------------------------------------------------
inline std::string DepthPassInit(
  vtkRenderer* vtkNotUsed(ren), vtkVolumeMapper* vtkNotUsed(mapper), vtkVolume* vtkNotUsed(vol))
{
  return std::string("\
    \n  vec3 l_isoPos = g_dataPos;");
}

//--------------------------------------------------------------------------
inline std::string DepthPassImplementation(
  vtkRenderer* vtkNotUsed(ren), vtkVolumeMapper* vtkNotUsed(mapper), vtkVolume* vtkNotUsed(vol))
{
  return std::string("\
    \n    if(!g_skip && g_srcColor.a > 0.0)\
    \n      {\
    \n      l_isoPos = g_dataPos;\
    \n      g_exit = true; g_skip = true;\
    \n      }");
}

//--------------------------------------------------------------------------
inline std::string DepthPassExit(
  vtkRenderer* vtkNotUsed(ren), vtkVolumeMapper* vtkNotUsed(mapper), vtkVolume* vtkNotUsed(vol))
{
  return std::string("\
    \n  vec4 depthValue = in_projectionMatrix * in_modelViewMatrix *\
    \n                  in_volumeMatrix[0] * in_textureDatasetMatrix[0] *\
    \n                  vec4(l_isoPos, 1.0);\
    \n  gl_FragData[0] = vec4(l_isoPos, 1.0);\
    \n  gl_FragData[1] = vec4(vec3((depthValue.z/depthValue.w) * 0.5 + 0.5),\
    \n                        1.0);");
}

//---------------------------------------------------------------------------
inline std::string WorkerImplementation(
  vtkRenderer* vtkNotUsed(ren), vtkVolumeMapper* vtkNotUsed(mapper), vtkVolume* vtkNotUsed(vol))
{
  return std::string("\
    \n  initializeRayCast();\
    \n  castRay(-1.0, -1.0);\
    \n  finalizeRayCast();");
}

//---------------------------------------------------------------------------
inline std::string ImageSampleDeclarationFrag(
  const std::vector<std::string>& varNames, size_t usedNames)
{
  std::string shader = "\n";
  for (size_t i = 0; i < usedNames; i++)
  {
    shader += "uniform sampler2D " + varNames[i] + ";\n";
  }
  return shader;
}

//---------------------------------------------------------------------------
inline std::string ImageSampleImplementationFrag(
  const std::vector<std::string>& varNames, size_t usedNames)
{
  std::string shader = "\n";
  for (size_t i = 0; i < usedNames; i++)
  {
    std::stringstream ss;
    ss << i;
    shader += " gl_FragData[" + ss.str() + "] = texture2D(" + varNames[i] + ", texCoord);\n";
  }
  shader += " return;\n";
  return shader;
}
VTK_ABI_NAMESPACE_END
}

#endif // vtkVolumeShaderComposer_h
// VTK-HeaderTest-Exclude: vtkVolumeShaderComposer.h
