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

#ifndef ___vtkVolumeShaderComposer_h
#define ___vtkVolumeShaderComposer_h

#include "vtkVolumeMask.h"

#include <vtkCamera.h>
#include <vtkRenderer.h>
#include <vtkVolume.h>
#include <vtkVolumeMapper.h>
#include <vtkVolumeProperty.h>

#include <string>

namespace vtkvolume
{
  //--------------------------------------------------------------------------
  std::string replace(std::string source, const std::string &search,
                      const std::string replace, bool all)
    {
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
  std::string ComputeClip(vtkRenderer* vtkNotUsed(ren),
                          vtkVolumeMapper* vtkNotUsed(mapper),
                          vtkVolume* vtkNotUsed(vol))
    {
    return std::string(" \n\
      vec4 pos = in_projectionMatrix * in_modelViewMatrix * \n\
                 in_volumeMatrix * vec4(in_vertexPos.xyz, 1.0); \n\
      gl_Position = pos;"
    );
    }

  //--------------------------------------------------------------------------
  std::string ComputeTextureCoords(vtkRenderer* vtkNotUsed(ren),
                                   vtkVolumeMapper* vtkNotUsed(mapper),
                                   vtkVolume* vtkNotUsed(vol))
    {
    return std::string(
      "// Assuming point data only. Also, we offset the texture coordinate \n\
       // to account for OpenGL treating voxel at the center of the cell. \n\
       vec3 uvx = (in_vertexPos - in_volumeExtentsMin) / \n\
                  (in_volumeExtentsMax - in_volumeExtentsMin); \n\
       vec3 delta = in_textureExtentsMax - in_textureExtentsMin; \n\
       ip_textureCoords = (uvx * (delta - vec3(1.0)) + vec3(0.5)) / delta;"
    );
    }

  //--------------------------------------------------------------------------
  std::string BaseGlobalsVert(vtkRenderer* vtkNotUsed(ren),
                              vtkVolumeMapper* vtkNotUsed(mapper),
                              vtkVolume* vtkNotUsed(vol))
    { return std::string(
    "uniform mat4 in_modelViewMatrix; \n\
    uniform mat4 in_projectionMatrix; \n\
    uniform mat4 in_volumeMatrix; \n\
    \n\
    uniform vec3 in_volumeExtentsMin; \n\
    uniform vec3 in_volumeExtentsMax; \n\
    \n\
    uniform vec3 in_textureExtentsMax; \n\
    uniform vec3 in_textureExtentsMin;"
    );
    }

  //--------------------------------------------------------------------------
  std::string BaseGlobalsFrag(vtkRenderer* vtkNotUsed(ren),
                              vtkVolumeMapper* vtkNotUsed(mapper),
                              vtkVolume* vtkNotUsed(vol),
                              int vtkNotUsed(numberOfLights),
                              int lightingComplexity)
    {
    std::string shaderStr = std::string(
      "// Volume dataset \n\
      uniform sampler3D in_volume; \n\
      \n\
      uniform sampler2D in_noiseSampler; \n\
      uniform sampler2D in_depthSampler; \n\
      \n\
      // Camera position \n\
      uniform vec3 in_cameraPos; \n\
      \n\
      // view and model matrices \n\
      uniform mat4 in_volumeMatrix; \n\
      uniform mat4 in_inverseVolumeMatrix; \n\
      uniform mat4 in_projectionMatrix; \n\
      uniform mat4 in_inverseProjectionMatrix; \n\
      uniform mat4 in_modelViewMatrix; \n\
      uniform mat4 in_inverseModelViewMatrix; \n\
      uniform mat4 in_textureDatasetMatrix; \n\
      uniform mat4 in_inverseTextureDatasetMatrix; \n\
      uniform mat4 in_texureToEyeIt; \n\
      \n\
      // Ray step size \n\
      uniform vec3 in_cellStep; \n\
      uniform vec2 in_scalarsRange; \n\
      uniform vec3 in_cellSpacing; \n\
      \n\
      // Sample distance \n\
      uniform float in_sampleDistance; \n\
      \n\
      // Scales \n\
      uniform vec3 in_cellScale; \n\
      uniform vec2 in_windowLowerLeftCorner; \n\
      uniform vec2 in_inverseOriginalWindowSize; \n\
      uniform vec2 in_inverseWindowSize; \n\
      uniform vec3 in_textureExtentsMax; \n\
      uniform vec3 in_textureExtentsMin; \n\
      \n\
      // Material and lighting \n\
      uniform vec3 in_diffuse; \n\
      uniform vec3 in_ambient; \n\
      uniform vec3 in_specular; \n\
      uniform float in_shininess; \n\
      ");

    if (lightingComplexity > 0)
      {
      shaderStr += std::string("\n\
        uniform bool in_twoSidedLighting; \n\
      ");
      }

    if (lightingComplexity == 3)
      {
      shaderStr += std::string("\n\
        uniform int in_numberOfLights; // only allow for up to 6 active lights\n\
        uniform vec3 in_lightColor[6]; // intensity weighted color\n\
        uniform vec3 in_lightDirection[6]; // normalized\n\
        uniform vec3 in_lightPosition[6];\n\
        uniform vec3 in_lightAttenuation[6];\n\
        uniform float in_lightConeAngle[6];\n\
        uniform float in_lightExponent[6];\n\
        uniform int in_lightPositional[6];\n\
      ");
      }
    else if (lightingComplexity == 2)
      {
      shaderStr += std::string("\n\
        uniform int in_numberOfLights; // only allow for up to 6 active lights\n\
        uniform vec3 in_lightColor[6]; // intensity weighted color\n\
        uniform vec3 in_lightDirection[6]; // normalized\n\
      ");
      }

    return shaderStr;
    }

  //--------------------------------------------------------------------------
  std::string BaseInit(vtkRenderer* vtkNotUsed(ren),
                       vtkVolumeMapper* vtkNotUsed(mapper),
                       vtkVolume* vtkNotUsed(vol))
    {
    return std::string(
      "\n\
      // Get the 3D texture coordinates for lookup into the in_volume dataset \n\
      g_dataPos = ip_textureCoords.xyz; \n\
      \n\
      // Eye position in object space  \n\
      g_eyePosObj = (in_inverseVolumeMatrix * vec4(in_cameraPos, 1.0)); \n\
      if (g_eyePosObj.w != 0.0) \n\
        { \n\
        g_eyePosObj.x /= g_eyePosObj.w; \n\
        g_eyePosObj.y /= g_eyePosObj.w; \n\
        g_eyePosObj.z /= g_eyePosObj.w; \n\
        g_eyePosObj.w = 1.0; \n\
        } \n\
      \n\
      // Getting the ray marching direction (in object space); \n\
      vec3 rayDir = computeRayDirection(); \n\
      \n\
      // Multiply the raymarching direction with the step size to get the \n\
      // sub-step size we need to take at each raymarching step  \n\
      g_dirStep = (in_inverseTextureDatasetMatrix * \n\
                   vec4(rayDir, 0.0)).xyz * in_sampleDistance; \n\
      \n\
      g_dataPos += g_dirStep * texture2D(in_noiseSampler, g_dataPos.xy).x;\n\
      \n\
      // Flag to deternmine if voxel should be considered for the rendering \n\
      bool l_skip = false;");
    }

  //--------------------------------------------------------------------------
  std::string BaseIncrement(vtkRenderer* vtkNotUsed(ren),
                            vtkVolumeMapper* vtkNotUsed(mapper),
                            vtkVolume* vtkNotUsed(vol))
    {
    return std::string("\n\
                       l_skip = false;");
    }

  //--------------------------------------------------------------------------
  std::string BaseExit(vtkRenderer* vtkNotUsed(ren),
                       vtkVolumeMapper* vtkNotUsed(mapper),
                       vtkVolume* vtkNotUsed(vol))
    {
    return std::string("");
    }

  //--------------------------------------------------------------------------
  std::string GradientsComputeFunc(vtkRenderer* vtkNotUsed(ren),
                                   vtkVolumeMapper* vtkNotUsed(mapper),
                                   vtkVolume* vol,
                                   int vtkNotUsed(numberOfComponents))
  {
    if (vol->GetProperty()->GetShade() &&
        !vol->GetProperty()->HasGradientOpacity())
      {
      return std::string(" \n\
        float computeGradientOpacity(vec4 grad) \n\
          { \n\
          return 1.0;\n\
          }\n\
        \n\
        vec4 computeGradient() \n\
          { \n\
          vec3 g1; \n\
          vec3 g2; \n\
          vec3 xvec = vec3(in_cellStep[0], 0.0, 0.0); \n\
          vec3 yvec = vec3(0.0, in_cellStep[1], 0.0); \n\
          vec3 zvec = vec3(0.0, 0.0, in_cellStep[2]); \n\
          g1.x = texture3D(in_volume, vec3(g_dataPos + xvec)).x; \n\
          g1.y = texture3D(in_volume, vec3(g_dataPos + yvec)).x; \n\
          g1.z = texture3D(in_volume, vec3(g_dataPos + zvec)).x; \n\
          g2.x = texture3D(in_volume, vec3(g_dataPos - xvec)).x; \n\
          g2.y = texture3D(in_volume, vec3(g_dataPos - yvec)).x; \n\
          g2.z = texture3D(in_volume, vec3(g_dataPos - zvec)).x; \n\
          return vec4((g1 - g2), -1.0); \n\
         }");
    }
    else if (vol->GetProperty()->GetShade() &&
             vol->GetProperty()->HasGradientOpacity())
      {
      return std::string(" \n\
        uniform sampler1D in_gradientTransferFunc;\n\
        float computeGradientOpacity(vec4 grad) \n\
          { \n\
          return texture1D(in_gradientTransferFunc, grad.w).w;\n\
          }\n\
        vec4 computeGradient() \n\
          { \n\
          vec3 g1; \n\
          vec4 g2; \n\
          vec3 xvec = vec3(in_cellStep[0], 0.0, 0.0); \n\
          vec3 yvec = vec3(0.0, in_cellStep[1], 0.0); \n\
          vec3 zvec = vec3(0.0, 0.0, in_cellStep[2]); \n\
          g1.x = texture3D(in_volume, vec3(g_dataPos + xvec)).x; \n\
          g1.y = texture3D(in_volume, vec3(g_dataPos + yvec)).x; \n\
          g1.z = texture3D(in_volume, vec3(g_dataPos + zvec)).x; \n\
          g2.x = texture3D(in_volume, vec3(g_dataPos - xvec)).x; \n\
          g2.y = texture3D(in_volume, vec3(g_dataPos - yvec)).x; \n\
          g2.z = texture3D(in_volume, vec3(g_dataPos - zvec)).x; \n\
          g1.x = in_scalarsRange[0] + ( \n\
                 in_scalarsRange[1] - in_scalarsRange[0]) * g1.x; \n\
          g1.y = in_scalarsRange[0] + ( \n\
                 in_scalarsRange[1] - in_scalarsRange[0]) * g1.y; \n\
          g1.z = in_scalarsRange[0] + ( \n\
                 in_scalarsRange[1] - in_scalarsRange[0]) * g1.z; \n\
          g2.x = in_scalarsRange[0] + ( \n\
                 in_scalarsRange[1] - in_scalarsRange[0]) * g2.x; \n\
          g2.y = in_scalarsRange[0] + ( \n\
                 in_scalarsRange[1] - in_scalarsRange[0]) * g2.y; \n\
          g2.z = in_scalarsRange[0] + ( \n\
                 in_scalarsRange[1] - in_scalarsRange[0]) * g2.z; \n\
          g2.xyz = g1 - g2.xyz; \n\
          vec3 cellSpacing = vec3(in_cellSpacing[0], \n\
                                  in_cellSpacing[1],  \n\
                                  in_cellSpacing[2]); \n\
          vec3 aspect; \n\
          float avgSpacing = (cellSpacing[0] + \n\
                              cellSpacing[1] + \n\
                              cellSpacing[2])/3.0; \n\
          // Adjust the aspect \n\
          aspect.x = cellSpacing[0] * 2.0 / avgSpacing; \n\
          aspect.y = cellSpacing[1] * 2.0 / avgSpacing; \n\
          aspect.z = cellSpacing[2] * 2.0 / avgSpacing; \n\
          g2.x /= aspect.x; \n\
          g2.y /= aspect.y; \n\
          g2.z /= aspect.z; \n\
          float grad_mag = sqrt(g2.x * g2.x  + \n\
                                g2.y * g2.y + \n\
                                g2.z * g2.z); \n\
          if (grad_mag > 0.0) \n\
            { \n\
            g2.x /= grad_mag; \n\
            g2.y /= grad_mag; \n\
            g2.z /= grad_mag; \n\
            } \n\
          else \n\
            { \n\
            g2.xyz = vec3(0.0, 0.0, 0.0); \n\
            } \n\
          grad_mag = grad_mag * 1.0 / (0.25 * (in_scalarsRange[1] - \n\
                                              (in_scalarsRange[0]))); \n\
          grad_mag = clamp(grad_mag, 0.0, 1.0); \n\
          g2.w = grad_mag;\n\
          return g2; \n\
          }");
      }
    else
      {
      return std::string(
        "vec3 computeGradient() \n\
           { \n\
             return vec3(0.0); \n\
           }");
      }
  }

  //--------------------------------------------------------------------------
  std::string LightComputeFunc(vtkRenderer* vtkNotUsed(ren),
                               vtkVolumeMapper* vtkNotUsed(mapper),
                               vtkVolume* vol,
                               int vtkNotUsed(numberOfComponents),
                               int vtkNotUsed(numberOfLights),
                               int lightingComplexity)
    {
    vtkVolumeProperty* volProperty = vol->GetProperty();
    std::string shaderStr = std::string(
      "vec4 computeLighting(vec4 color) \n\
         { \n\
         return color; \n\
      }");

    if (volProperty->GetShade())
      {
      if (lightingComplexity == 1)
        {
        shaderStr = std::string(" \n\
          vec4 computeLighting(vec4 color) \n\
            {\n\
            // Light position in object space \n\
            vec4 lightPosObj = (in_inverseVolumeMatrix * \n\
                                vec4(in_cameraPos, 1.0)); \n\
            if (lightPosObj.w != 0.0) \n\
              { \n\
              lightPosObj.x /= lightPosObj.w; \n\
              lightPosObj.y /= lightPosObj.w; \n\
              lightPosObj.z /= lightPosObj.w; \n\
              lightPosObj.w = 1.0; \n\
            } \n\
            vec3 ldir = normalize(lightPosObj.xyz - ip_vertexPos); \n\
            vec3 vdir = normalize(g_eyePosObj.xyz - ip_vertexPos); \n\
            vec3 h = normalize(ldir + vdir); \n\
            vec4 grad = computeGradient(); \n\
            vec3 g2 = grad.xyz; \n\
            g2 = (1.0/in_cellSpacing) * g2; \n\
            float normalLength = length(g2);\n\
            if (normalLength > 0.0) \n\
              { \n\
              g2 = normalize(g2); \n\
              } \n\
            else \n\
              { \n\
              g2 = vec3(0.0, 0.0, 0.0); \n\
              } \n\
            vec3 finalColor = vec3(0.0); \n\
            float nDotL = dot(g2, ldir); \n\
            float nDotH = dot(g2, h); \n\
            if (nDotL < 0.0 && in_twoSidedLighting) \n\
              { \n\
              nDotL = -nDotL; \n\
              } \n\
            if (nDotH < 0.0 && in_twoSidedLighting) \n\
              { \n\
              nDotH = -nDotH; \n\
              } \n\
            finalColor += in_ambient * color.rgb; \n\
            if (nDotL > 0) \n\
              { \n\
              finalColor += in_diffuse * nDotL * color.rgb; \n\
              } \n\
            if (nDotH > 0) \n\
              { \n\
              finalColor += in_specular * pow(nDotH, in_shininess); \n\
              } \n\
            finalColor = clamp(finalColor, vec3(0.0), vec3(1.0)); \n\
            if (grad.w >= 0.0)\n\
              {\n\
              color.a = color.a * computeGradientOpacity(grad); \n\
              }\n\
            return vec4(finalColor, color.a); \n\
          }");
        }
      else if (lightingComplexity == 2)
        {
        shaderStr = std::string(" \n\
          vec4 computeLighting(vec4 color) \n\
            {\n\
            vec3 vdir = normalize(g_eyePosObj.xyz - ip_vertexPos); \n\
            vec4 grad = computeGradient(); \n\
            vec3 g2 = grad.xyz; \n\
            vec3 diffuse = vec3(0.0); \n\
            vec3 specular = vec3(0.0); \n\
            g2 = (1.0/in_cellSpacing) * g2; \n\
            float normalLength = length(g2);\n\
            if (normalLength > 0.0) \n\
               { \n\
               g2 = normalize(g2); \n\
               } \n\
             else \n\
               { \n\
               g2 = vec3(0.0, 0.0, 0.0); \n\
               } \n\
            vec3 finalColor = vec3(0.0); \n\
            for (int lightNum = 0; lightNum < in_numberOfLights; lightNum++)\n\
              {\n\
              vec3 ldir = normalize((in_inverseVolumeMatrix * \n\
                                     in_inverseModelViewMatrix * \n\
                                    vec4(in_lightDirection[lightNum].xyz, 0.0)).xyz); \n\
              vec3 h = normalize(ldir + vdir); \n\
              float nDotH = dot(g2, h); \n\
              if (nDotH < 0.0 && in_twoSidedLighting) \n\
                { \n\
                nDotH = -nDotH; \n\
                } \n\
              float nDotL = dot(g2, ldir); \n\
              if (nDotL < 0.0 && in_twoSidedLighting) \n\
                { \n\
                nDotL = -nDotL; \n\
                } \n\
              if (nDotL > 0) \n\
                { \n\
                diffuse += in_lightColor[lightNum] * nDotL; \n\
                } \n\
              if (nDotH > 0) \n\
                { \n\
                specular = in_lightColor[lightNum] * pow(nDotH, in_shininess); \n\
                } \n\
              }\n\
            finalColor += (in_ambient + in_diffuse * diffuse + in_specular * specular) * color.rgb; \n\
            finalColor = clamp(finalColor, vec3(0.0), vec3(1.0)); \n\
            if (grad.w >= 0.0)\n\
              {\n\
              color.a = color.a * computeGradientOpacity(grad); \n\
              }\n\
            return vec4(finalColor, color.a); \n\
            }");
        }
      else if (lightingComplexity == 3)
        {
        shaderStr = std::string("\n\
          vec4 computeLighting(vec4 color)\n\
            {\n\
            vec4 fragWorldPos = in_modelViewMatrix * in_volumeMatrix * \n\
                                  in_textureDatasetMatrix * vec4(g_dataPos, 1.0); \n\
            if (fragWorldPos.w != 0.0) \n\
              { \n\
              fragWorldPos /= fragWorldPos.w; \n\
              } \n\
            vec3 viewDirection = normalize(-fragWorldPos.xyz);\n\
            vec3 diffuse = vec3(0,0,0);\n\
            vec3 specular = vec3(0,0,0);\n\
            vec3 vertLightDirection;\n\
            vec4 grad = computeGradient(); \n\
            vec3 normal = (in_texureToEyeIt * vec4(grad.xyz, 0.0)).xyz; \n\
            normal = normalize(normal); \n\
            vec3 lightDir; \n\
            for (int lightNum = 0; lightNum < in_numberOfLights; lightNum++)\n\
              {\n\
              float attenuation = 1.0;\n\
              // directional\n\
              lightDir = in_lightDirection[lightNum]; \n\
              if (in_lightPositional[lightNum] == 0)\n\
                {\n\
                vertLightDirection = lightDir;\n\
                }\n\
              else\n\
                {\n\
                vertLightDirection = (fragWorldPos.xyz - in_lightPosition[lightNum]);\n\
                float distance = length(vertLightDirection);\n\
                vertLightDirection = normalize(vertLightDirection);\n\
                attenuation = 1.0 /\n\
                  (in_lightAttenuation[lightNum].x\n\
                   + in_lightAttenuation[lightNum].y * distance\n\
                   + in_lightAttenuation[lightNum].z * distance * distance);\n\
                // per OpenGL standard cone angle is 90 or less for a spot light\n\
                if (in_lightConeAngle[lightNum] <= 90.0)\n\
                  {\n\
                  float coneDot = dot(vertLightDirection, lightDir);\n\
                  // if inside the cone\n\
                  if (coneDot >= cos(radians(in_lightConeAngle[lightNum])))\n\
                    {\n\
                    attenuation = attenuation * pow(coneDot, in_lightExponent[lightNum]);\n\
                    }\n\
                  else\n\
                    {\n\
                    attenuation = 0.0;\n\
                    }\n\
                  }\n\
                }\n\
            // diffuse and specular lighting\n\
            float df = max(0.0, attenuation * dot(normal, vertLightDirection));\n\
            diffuse += (df * in_lightColor[lightNum]);\n\
            if (dot(normal, -vertLightDirection) > 0.0)\n\
              {\n\
              float sf = attenuation*pow( max(0.0, dot(\n\
                reflect(vertLightDirection, normal), viewDirection)), in_shininess);\n\
              specular += (sf * in_lightColor[lightNum]);\n\
              }\n\
            }\n\
            vec3 finalColor = vec3(0.0); \n\
            finalColor += (in_ambient + in_diffuse * diffuse + in_specular * specular) * color.rgb; \n\
            if (grad.w >= 0.0)\n\
              {\n\
              color.a = color.a * computeGradientOpacity(grad); \n\
              }\n\
            return vec4(finalColor, color.a); \n\
          } \n\
        ");
        }
      }
      else if (vol->GetProperty()->HasGradientOpacity())
        {
        std::string(
          "vec4 computeLighting(vec4 color) \n\
             { \n\
              vec4 grad = computeGradient(); \n\
              if (grad.w >= 0.0)\n\
                {\n\
                color.a = color.a * \n\
                  texture1D(in_gradientTransferFunc, grad.w).w; \n\
                }\n\
             return color; \n\
          }");
        }
      return shaderStr;
    }

  //--------------------------------------------------------------------------
  std::string RayDirectionFunc(vtkRenderer* ren,
                               vtkVolumeMapper* vtkNotUsed(mapper),
                               vtkVolume* vtkNotUsed(vol),
                               int vtkNotUsed(numberOfComponents))
    {
    if (!ren->GetActiveCamera()->GetParallelProjection())
      {
      return std::string(
        "vec3 computeRayDirection() \n\
           { \n\
           return normalize(ip_vertexPos.xyz - g_eyePosObj.xyz); \n\
           }");
      }
    else
      {
      return std::string(
        "uniform vec3 in_projectionDirection; \n\
         vec3 computeRayDirection() \n\
           { \n\
           return normalize((in_inverseVolumeMatrix * \n\
                             vec4(in_projectionDirection, 0.0)).xyz); \n\
           }");
      }
    }

  //--------------------------------------------------------------------------
  std::string ColorTransferFunc(vtkRenderer* vtkNotUsed(ren),
                                vtkVolumeMapper* vtkNotUsed(mapper),
                                vtkVolume* vtkNotUsed(vol),
                                int numberOfComponents)
    {
      if (numberOfComponents == 1)
        {
        return std::string(" \n\
          uniform sampler1D in_colorTransferFunc; \n\
          vec4 computeColor(vec4 scalar) \n\
            { \n\
            return computeLighting(vec4(texture1D(in_colorTransferFunc, \n\
                                                  scalar.w).xyz, \n\
                                        computeOpacity(scalar))); \n\
            }");
        }

        return std::string(" \n\
          vec4 computeColor(vec4 scalar) \n\
          { \n\
          return computeLighting(vec4(scalar.xyz, computeOpacity(scalar))); \n\
          }");
    }

  //--------------------------------------------------------------------------
  std::string OpacityTransferFunc(vtkRenderer* vtkNotUsed(ren),
                                  vtkVolumeMapper* vtkNotUsed(mapper),
                                  vtkVolume* vtkNotUsed(vol),
                                  int vtkNotUsed(numberOfComponents))
    {
    return std::string(
      "uniform sampler1D in_opacityTransferFunc; \n\
       float computeOpacity(vec4 scalar) \n\
         { \n\
         return texture1D(in_opacityTransferFunc, scalar.w).w; \n\
         }");
    }

  //--------------------------------------------------------------------------
  std::string ShadingGlobalsVert(vtkRenderer* vtkNotUsed(ren),
                                 vtkVolumeMapper* vtkNotUsed(mapper),
                                 vtkVolume* vtkNotUsed(vol))
    {
    return std::string("");
    }

  //--------------------------------------------------------------------------
  std::string ShadingGlobalsFrag(vtkRenderer* vtkNotUsed(ren),
                                 vtkVolumeMapper* vtkNotUsed(mapper),
                                 vtkVolume* vtkNotUsed(vol))
    {
    return std::string("");
    }

  //--------------------------------------------------------------------------
  std::string ShadingInit(vtkRenderer* vtkNotUsed(ren),
                          vtkVolumeMapper* mapper,
                          vtkVolume* vtkNotUsed(vol))
    {
    if (mapper->GetBlendMode() == vtkVolumeMapper::MAXIMUM_INTENSITY_BLEND)
      {
      return std::string(
        "// We get data between 0.0 - 1.0 range \n\
         vec4 l_maxValue = vec4(0.0);");
      }
    else if (mapper->GetBlendMode() == vtkVolumeMapper::MINIMUM_INTENSITY_BLEND)
      {
      return std::string(
        "// We get data between 0.0 - 1.0 range \n\
        vec4 l_minValue = vec4(1.0);");
      }
    else if (mapper->GetBlendMode() == vtkVolumeMapper::ADDITIVE_BLEND)
      {
      return std::string(
        "// We get data between 0.0 - 1.0 range \n\
        float l_sumValue = 0.0;");
      }
    else
      {
      return std::string("");
      }
    }

  //--------------------------------------------------------------------------
  std::string ShadingIncrement(vtkRenderer* vtkNotUsed(ren),
                               vtkVolumeMapper* mapper,
                               vtkVolume* vtkNotUsed(vol),
                               vtkImageData* maskInput,
                               vtkVolumeMask* mask, int maskType)
    {
    std::string shaderStr = std::string(
      "if (!l_skip) \n\
         {\n");

    if (mapper->GetBlendMode() == vtkVolumeMapper::MAXIMUM_INTENSITY_BLEND)
      {
      shaderStr += std::string(
        "vec4 scalar = texture3D(in_volume, g_dataPos); \n\
         if (l_maxValue.w < scalar.w) \n\
           { \n\
           l_maxValue = scalar; \n\
           }");
      }
    else if (mapper->GetBlendMode() == vtkVolumeMapper::MINIMUM_INTENSITY_BLEND)
      {
      shaderStr += std::string(
        "vec4 scalar = texture3D(in_volume, g_dataPos) ; \n\
          if (l_minValue.w > scalar.w) \n\
            { \n\
            l_minValue = scalar; \n\
            }");
      }
    else if (mapper->GetBlendMode() == vtkVolumeMapper::ADDITIVE_BLEND)
      {
      shaderStr += std::string(
        "vec4 scalar = texture3D(in_volume, g_dataPos); \n\
        float opacity = computeOpacity(scalar); \n\
        l_sumValue = l_sumValue + opacity * scalar.w;");
      }
    else if (mapper->GetBlendMode() == vtkVolumeMapper::COMPOSITE_BLEND)
      {
      if (!mask || !maskInput ||
          maskType != vtkGPUVolumeRayCastMapper::LabelMapMaskType)
        {
        shaderStr += std::string(
          "// Data fetching from the red channel of volume texture \n\
          vec4 scalar = texture3D(in_volume, g_dataPos); \n\
          vec4 g_srcColor = computeColor(scalar);");
        }

      shaderStr += std::string(
        "// Opacity calculation using compositing: \n\
         // here we use front to back compositing scheme whereby the current \n\
         // sample value is multiplied to the currently accumulated alpha \n\
         // and then this product is subtracted from the sample value to \n\
         // get the alpha from the previous steps. \n\
         // Next, this alpha is multiplied with the current sample colour \n\
         // and accumulated to the composited colour. The alpha value from \n\
         // the previous steps is then accumulated to the composited colour \n\
         // alpha. \n\
         g_srcColor.rgb *= g_srcColor.a; \n\
         g_fragColor = (1.0f - g_fragColor.a) * g_srcColor + g_fragColor;");
      }
     else
       {
       shaderStr += std::string("");
       }
      shaderStr += std::string("}");
      return shaderStr;
    }

  //--------------------------------------------------------------------------
  std::string ShadingExit(vtkRenderer* vtkNotUsed(ren),
                          vtkVolumeMapper* mapper,
                          vtkVolume* vtkNotUsed(vol))
    {
    if (mapper->GetBlendMode() == vtkVolumeMapper::MAXIMUM_INTENSITY_BLEND)
      {
      return std::string(
       "vec4 g_srcColor = vec4(computeColor(l_maxValue).xyz, \n\
                                computeOpacity(l_maxValue)); \n\
        g_fragColor.rgb = g_srcColor.rgb * g_srcColor.a; \n\
        g_fragColor.a = g_srcColor.a;");
      }
    else if (mapper->GetBlendMode() == vtkVolumeMapper::MINIMUM_INTENSITY_BLEND)
      {
      return std::string(
        "vec4 g_srcColor = vec4(computeColor(l_minValue).xyz, \n\
                                 computeOpacity(l_minValue)); \n\
        g_fragColor.rgb = g_srcColor.rgb * g_srcColor.a; \n\
        g_fragColor.a = g_srcColor.a;");
      }
    else if (mapper->GetBlendMode() == vtkVolumeMapper::ADDITIVE_BLEND)
      {
      return std::string(
        "l_sumValue = clamp(l_sumValue, 0.0, 1.0); \n\
         g_fragColor = vec4(vec3(l_sumValue), 1.0);");
      }
    else
      {
      return std::string("");
      }
  }

  //--------------------------------------------------------------------------
  std::string TerminationGlobalsVert(vtkRenderer* vtkNotUsed(ren),
                                     vtkVolumeMapper* vtkNotUsed(mapper),
                                     vtkVolume* vtkNotUsed(vol))
    {
    return std::string("");
    }

  //--------------------------------------------------------------------------
  std::string TerminationGlobalsFrag(vtkRenderer* vtkNotUsed(ren),
                                     vtkVolumeMapper* vtkNotUsed(mapper),
                                     vtkVolume* vtkNotUsed(vol))
    {
    return std::string("");
    }

  //--------------------------------------------------------------------------
  std::string TerminationInit(vtkRenderer* vtkNotUsed(ren),
                              vtkVolumeMapper* vtkNotUsed(mapper),
                              vtkVolume* vtkNotUsed(vol))
    {
    return std::string(
      "// Minimum texture access coordinate \n\
      const vec3 l_tex_min = vec3(0); \n\
      \n\
      // Maximum texture access coordinate \n\
      const vec3 l_tex_max = vec3(1); \n\
      \n\
      // Flag to indicate if the raymarch loop should terminate \n\
      bool stop = false; \n\
      \n\
      // 2D Texture fragment coordinates [0,1] from fragment coordinates \n\
      // the frame buffer texture has the size of the plain buffer but \n\
      // we use a fraction of it. The texture coordinates is less than 1 if \n\
      // the reduction factor is less than 1. \n\
      // Device coordinates are between -1 and 1. We need texture \n\
      // coordinates between 0 and 1 the in_depthSampler buffer has the \n\
      // original size buffer. \n\
      vec2 fragTexCoord = \n\
        (gl_FragCoord.xy - in_windowLowerLeftCorner) * \n\
        in_inverseWindowSize; \n\
      vec4 l_depthValue = texture2D(in_depthSampler, fragTexCoord); \n\
      float l_terminatePointMax = 0.0; \n\
      \n\
      // Depth test \n\
      if(gl_FragCoord.z >= l_depthValue.x) \n\
       { \n\
       discard; \n\
       } \n\
      \n\
      // color buffer or max scalar buffer have a reduced size. \n\
      fragTexCoord = (gl_FragCoord.xy - in_windowLowerLeftCorner) * \n\
                     in_inverseOriginalWindowSize; \n\
      \n\
      // Compute max number of iterations it will take before we hit \n\
      // the termination point \n\
      \n\
      // Abscissa of the point on the depth buffer along the ray. \n\
      // point in texture coordinates \n\
      vec4 terminatePoint; \n\
      terminatePoint.x = \n\
        (gl_FragCoord.x - in_windowLowerLeftCorner.x) * 2.0 * \n\
                            in_inverseWindowSize.x - 1.0; \n\
      terminatePoint.y = \n\
        (gl_FragCoord.y - in_windowLowerLeftCorner.y) * 2.0 * \n\
                            in_inverseWindowSize.y - 1.0; \n\
      terminatePoint.z = (2.0 * l_depthValue.x - (gl_DepthRange.near + \n\
                            gl_DepthRange.far)) / gl_DepthRange.diff; \n\
      terminatePoint.w = 1.0; \n\
      \n\
      // From normalized device coordinates to eye coordinates. \n\
      // in_projectionMatrix is inversed because of way VT \n\
      // From eye coordinates to texture coordinates \n\
      terminatePoint = in_inverseTextureDatasetMatrix * \n\
                          in_inverseVolumeMatrix * \n\
                          in_inverseModelViewMatrix * \n\
                          in_inverseProjectionMatrix * \n\
                          terminatePoint; \n\
      terminatePoint /= terminatePoint.w; \n\
      \n\
      l_terminatePointMax = \n\
        length(terminatePoint.xyz - g_dataPos.xyz) / \n\
                              length(g_dirStep); \n\
      float l_currentT = 0.0;");
    }

  //--------------------------------------------------------------------------
  std::string TerminationIncrement(vtkRenderer* vtkNotUsed(ren),
                                   vtkVolumeMapper* vtkNotUsed(mapper),
                                   vtkVolume* vtkNotUsed(vol))
    {
    return std::string(
      "// The two constants l_tex_min and l_tex_max have a value of \n\
      // vec3(-1,-1,-1) and vec3(1,1,1) respectively. To determine if the \n\
      // data value is outside the in_volume data, we use the sign function. \n\
      // The sign function return -1 if the value is less than 0, 0 if the \n\
      // value is equal to 0 and 1 if value is greater than 0. Hence, the \n\
      // sign function for the calculation (sign(g_dataPos-l_tex_min) and \n\
      // sign (l_tex_max-g_dataPos)) will give us vec3(1,1,1) at the \n\
      // possible minimum and maximum position. \n\
      // When we do a dot product between two vec3(1,1,1) we get answer 3. \n\
      // So to be within the dataset limits, the dot product will return a \n\
      // value less than 3. If it is greater than 3, we are already out of \n\
      // the in_volume dataset \n\
      stop = dot(sign(g_dataPos - l_tex_min), sign(l_tex_max - g_dataPos)) \n\
             < 3.0; \n\
      \n\
      // If the stopping condition is true we brek out of the ray marching \n\
      // loop \n\
      if (stop) \n\
       { \n\
       break; \n\
       } \n\
      // Early ray termination \n\
      // if the currently composited colour alpha is already fully saturated \n\
      // we terminated the loop or if we have hit an obstacle in the \n\
      // direction of they ray (using depth buffer) we terminate as well. \n\
      if((g_fragColor.a > (1 - 1/255.0)) ||  \n\
          l_currentT >= l_terminatePointMax) \n\
        { \n\
        break; \n\
        } \n\
      ++l_currentT;"
    );
    }

  //--------------------------------------------------------------------------
  std::string TerminationExit(vtkRenderer* vtkNotUsed(ren),
                              vtkVolumeMapper* vtkNotUsed(mapper),
                              vtkVolume* vtkNotUsed(vol))
   {
    return std::string("");
   }

  //--------------------------------------------------------------------------
  std::string CroppingGlobalsVert(vtkRenderer* vtkNotUsed(ren),
                                  vtkVolumeMapper* vtkNotUsed(mapper),
                                  vtkVolume* vtkNotUsed(vol))
  {
    return std::string("");
  }

  //--------------------------------------------------------------------------
  std::string CroppingGlobalsFrag(vtkRenderer* vtkNotUsed(ren),
                                  vtkVolumeMapper* mapper,
                                  vtkVolume* vtkNotUsed(vol))
  {
    if (!mapper->GetCropping()) {
      return std::string("");
    }

    return std::string("\n\
      uniform float cropping_planes[6]; \n\
      uniform int cropping_flags [32]; \n\
      // X: axis = 0, Y: axis = 1, Z: axis = 2 \n\
      // cp Cropping plane bounds (minX, maxX, minY, maxY, minZ, maxZ) \n\
      int computeRegionCoord(float cp[6], vec3 pos, int axis) \n\
      { \n\
        int cpmin = axis * 2; \n\
        int cpmax = cpmin + 1; \n\
        \n\
        if (pos[axis] < cp[cpmin]) \n\
          { \n\
          return 1; \n\
          } \n\
        else if (pos[axis] >= cp[cpmin] && \n\
                 pos[axis]  < cp[cpmax]) \n\
          { \n\
          return 2; \n\
          } \n\
        else if (pos[axis] >= cp[cpmax]) \n\
          { \n\
          return 3; \n\
          } \n\
        return 0; \n\
      } \n\
      \n\
      int computeRegion(float cp[6], vec3 pos) \n\
      { \n\
        return (computeRegionCoord(cp, pos, 0) +  \n\
               (computeRegionCoord(cp, pos, 1) - 1) * 3 + \n\
               (computeRegionCoord(cp, pos, 2) - 1) * 9); \n\
      }");
  }

  //--------------------------------------------------------------------------
  std::string CroppingInit(vtkRenderer* vtkNotUsed(ren),
                           vtkVolumeMapper* mapper,
                           vtkVolume* vtkNotUsed(vol))
  {
    if (!mapper->GetCropping()) {
      return std::string("");
    }

    return std::string("\n\
      // Convert cropping region to texture space \n\
      float cropping_planes_ts[6];\n\
      mat4  datasetToTextureMat = in_inverseTextureDatasetMatrix; \n\
      vec4 temp = vec4(cropping_planes[0], cropping_planes[1], 0.0, 1.0); \n\
      temp = datasetToTextureMat * temp; \n\
      if (temp[3] != 0.0) {temp[0] /= temp[3]; temp[1] /= temp[3];} \n\
      cropping_planes_ts[0] = temp[0];\n\
      cropping_planes_ts[1] = temp[1];\n\
      \n\
      temp = vec4(cropping_planes[2], cropping_planes[3], 0.0, 1.0); \n\
      temp = datasetToTextureMat * temp; \n\
      if (temp[3] != 0.0) {temp[0] /= temp[3]; temp[1] /= temp[3];} \n\
      cropping_planes_ts[2] = temp[0];\n\
      cropping_planes_ts[3] = temp[1];\n\
      \n\
      temp = vec4(cropping_planes[4], cropping_planes[5], 0.0, 1.0); \n\
      temp = datasetToTextureMat * temp; \n\
      if (temp[3] != 0.0) {temp[0] /= temp[3]; temp[1] /= temp[3];} \n\
      cropping_planes_ts[4] = temp[0];\n\
      cropping_planes_ts[5] = temp[1];");
  }

  //--------------------------------------------------------------------------
  std::string CroppingIncrement(vtkRenderer* vtkNotUsed(ren),
                                vtkVolumeMapper* mapper,
                                vtkVolume* vtkNotUsed(vol))
  {
    if (!mapper->GetCropping()) {
      return std::string("");
    }

    return std::string("\n\
      // Determine region \n\
      int regionNo = computeRegion(cropping_planes_ts, g_dataPos); \n\
      \n\
      // Do & operation with cropping flags \n\
      // Pass the flag that its Ok to sample or not to sample \n\
      if (cropping_flags[regionNo] == 0) \n\
       { \n\
       // Skip this voxel \n\
       l_skip = true; \n\
       }");
  }

  //--------------------------------------------------------------------------
  std::string CroppingExit(vtkRenderer* vtkNotUsed(ren),
                           vtkVolumeMapper* vtkNotUsed(mapper),
                           vtkVolume* vtkNotUsed(vol))
  {
    return std::string("");
  }

  //--------------------------------------------------------------------------
  std::string ClippingGlobalsVert(vtkRenderer* vtkNotUsed(ren),
                                  vtkVolumeMapper* vtkNotUsed(mapper),
                                  vtkVolume* vtkNotUsed(vol))
  {
    return std::string("");
  }

  //--------------------------------------------------------------------------
  std::string ClippingGlobalsFrag(vtkRenderer* vtkNotUsed(ren),
                                  vtkVolumeMapper* vtkNotUsed(mapper),
                                  vtkVolume* vtkNotUsed(vol))
  {
    return std::string("");
  }

  //--------------------------------------------------------------------------
  std::string ClippingInit(vtkRenderer* vtkNotUsed(ren),
                           vtkVolumeMapper* mapper,
                           vtkVolume* vtkNotUsed(vol))
  {
    if (!mapper->GetClippingPlanes())
      {
      return std::string("");
      }
   else
      {
      return std::string("\n\
        float clippingPlanesTexture[48];\n\
        int clippingPlanesSize = int(in_clippingPlanes[0]);\n\
        \n\
        mat4 world_to_texture_mat = in_inverseTextureDatasetMatrix *\n\
                                    in_inverseVolumeMatrix;\n\
        for (int i = 0; i < clippingPlanesSize; i = i + 6)\n\
          {\n\
          vec4 origin = vec4(in_clippingPlanes[i + 1],\n\
                             in_clippingPlanes[i + 2],\n\
                             in_clippingPlanes[i + 3], 1.0);\n\
          vec4 normal = vec4(in_clippingPlanes[i + 4],\n\
                             in_clippingPlanes[i + 5],\n\
                             in_clippingPlanes[i + 6], 0.0);\n\
          \n\
          origin = world_to_texture_mat * origin;\n\
          normal = world_to_texture_mat * normal;\n\
          \n\
          if (origin[3] != 0.0)\n\
            {\n\
            origin[0] = origin[0] / origin[3];\n\
            origin[1] = origin[1] / origin[3];\n\
            origin[2] = origin[2] / origin[3];\n\
            }\n\
          if (normal[3] != 0.0)\n\
            {\n\
            normal[0] = normal[0] / normal[3];\n\
            normal[1] = normal[1] / normal[3];\n\
            normal[2] = normal[2] / normal[3];\n\
            }\n\
          \n\
          clippingPlanesTexture[i]     = origin[0];\n\
          clippingPlanesTexture[i + 1] = origin[1];\n\
          clippingPlanesTexture[i + 2] = origin[2];\n\
          \n\
          clippingPlanesTexture[i + 3] = normal[0];\n\
          clippingPlanesTexture[i + 4] = normal[1];\n\
          clippingPlanesTexture[i + 5] = normal[2];\n\
          }");
      }
  }

  //--------------------------------------------------------------------------
  std::string ClippingIncrement(vtkRenderer* vtkNotUsed(ren),
                                vtkVolumeMapper* mapper,
                                vtkVolume* vtkNotUsed(vol))
  {
    if (!mapper->GetClippingPlanes())
      {
      return std::string("");
      }
    else
      {
      return std::string("\n\
        for (int i = 0; i < (clippingPlanesSize) && !l_skip; i = i + 6)\n\
         {\n\
         if (dot(vec3(g_dataPos - vec3(clippingPlanesTexture[i],\n\
                                       clippingPlanesTexture[i + 1],\n\
                                       clippingPlanesTexture[i + 2])),\n\
             vec3(clippingPlanesTexture[i + 3],\n\
                  clippingPlanesTexture[i + 4],\n\
                  clippingPlanesTexture[i + 5])) < 0)\n\
           {\n\
           l_skip = true;\n\
           break;\n\
           }\n\
         }");
      }
  }

  //--------------------------------------------------------------------------
  std::string ClippingExit(vtkRenderer* vtkNotUsed(ren),
                           vtkVolumeMapper* vtkNotUsed(mapper),
                           vtkVolume* vtkNotUsed(vol))
  {
    return std::string("");
  }

  //--------------------------------------------------------------------------
  std::string BinaryMaskGlobalsFrag(vtkRenderer* vtkNotUsed(ren),
                                    vtkVolumeMapper* vtkNotUsed(mapper),
                                    vtkVolume* vtkNotUsed(vol),
                                    vtkImageData* maskInput,
                                    vtkVolumeMask* mask,
                                    int vtkNotUsed(maskType))
  {
    if (!mask || !maskInput)
      {
      return std::string("");
      }
    else
      {
      return std::string("uniform sampler3D in_mask;");
      }
  }

  //--------------------------------------------------------------------------
  std::string BinaryMaskIncrement(vtkRenderer* vtkNotUsed(ren),
                                  vtkVolumeMapper* vtkNotUsed(mapper),
                                  vtkVolume* vtkNotUsed(vol),
                                  vtkImageData* maskInput,
                                  vtkVolumeMask* mask,
                                  int maskType)
  {
    if (!mask || !maskInput ||
        maskType == vtkGPUVolumeRayCastMapper::LabelMapMaskType)
      {
      return std::string("");
      }
    else
      {
      return std::string("\n\
        vec4 maskValue = texture3D(in_mask, g_dataPos);\n\
        if(maskValue.a <= 0.0)\n\
          {\n\
          l_skip = true;\n\
          }");
      }
  }

  //--------------------------------------------------------------------------
  std::string CompositeMaskGlobalsFrag(vtkRenderer* vtkNotUsed(ren),
                                       vtkVolumeMapper* vtkNotUsed(mapper),
                                       vtkVolume* vtkNotUsed(vol),
                                       vtkImageData* maskInput,
                                       vtkVolumeMask* mask,
                                       int maskType)
  {
    if (!mask || !maskInput ||
        maskType != vtkGPUVolumeRayCastMapper::LabelMapMaskType)
      {
      return std::string("");
      }
    else
      {
      return std::string("\n\
        uniform float in_maskBlendFactor;\n\
        uniform sampler1D in_mask1;\n\
        uniform sampler1D in_mask2;");
      }
  }

  //--------------------------------------------------------------------------
  std::string CompositeMaskIncrement(vtkRenderer* vtkNotUsed(ren),
                                     vtkVolumeMapper* vtkNotUsed(mapper),
                                     vtkVolume* vtkNotUsed(vol),
                                     vtkImageData* maskInput,
                                     vtkVolumeMask* mask,
                                     int maskType)
  {
    if (!mask || !maskInput ||
        maskType != vtkGPUVolumeRayCastMapper::LabelMapMaskType)
      {
      return std::string("");
      }
    else
      {
      return std::string("\n\
        vec4 scalar = texture3D(in_volume, g_dataPos); \n\
        if (in_maskBlendFactor == 0.0)\n\
          {\n\
          g_srcColor = computeColor(scalar);\n\
          }\n\
        else\n\
         {\n\
         // Get the mask value at this same location\n\
         vec4 maskValue = texture3D(in_mask, g_dataPos);\n\
         if(maskValue.a == 0.0)\n\
           {\n\
           g_srcColor = computeColor(scalar);\n\
           }\n\
         else\n\
           {\n\
           if (maskValue.a == 1.0/255.0)\n\
             {\n\
             g_srcColor = texture1D(in_mask1, scalar.w);\n\
             }\n\
           else\n\
             {\n\
             // maskValue.a == 2.0/255.0\n\
             g_srcColor = texture1D(in_mask2, scalar.w);\n\
             }\n\
           g_srcColor.a = 1.0; \n\
           if(in_maskBlendFactor < 1.0) \n\
             {\n\
             g_srcColor = (1.0 - in_maskBlendFactor) * computeColor(scalar)\n\
               + in_maskBlendFactor * g_srcColor;\n\
             }\n\
           }\n\
          g_srcColor.a = computeOpacity(scalar); \n\
         }");
      }
  }
}

#endif // ___vtkVolumeShaderComposer_h
// VTK-HeaderTest-Exclude: vtkVolumeShaderComposer.h
