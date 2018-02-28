//VTK::System::Dec

/*=========================================================================

  Program:   Visualization Toolkit
  Module:    raycasterfs.glsl

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

//////////////////////////////////////////////////////////////////////////////
///
/// Inputs
///
//////////////////////////////////////////////////////////////////////////////

/// 3D texture coordinates form vertex shader
in vec3 ip_textureCoords;
in vec3 ip_vertexPos;

//////////////////////////////////////////////////////////////////////////////
///
/// Outputs
///
//////////////////////////////////////////////////////////////////////////////

vec4 g_fragColor = vec4(0.0);

//////////////////////////////////////////////////////////////////////////////
///
/// Uniforms, attributes, and globals
///
//////////////////////////////////////////////////////////////////////////////
vec3 g_dataPos;
vec3 g_dirStep;
vec4 g_srcColor;
vec4 g_eyePosObj;
bool g_exit;
bool g_skip;
float g_currentT;
float g_terminatePointMax;
vec4 g_scalar;

uniform vec4 in_volume_scale[1];
uniform vec4 in_volume_bias[1];

out vec4 fragOutput0;

// Volume dataset
uniform sampler3D in_volume[1];
uniform int in_noOfComponents;
uniform int in_independentComponents;

uniform sampler2D in_noiseSampler;
#ifndef GL_ES
uniform sampler2D in_depthSampler;
#endif

// Camera position
uniform vec3 in_cameraPos;

// view and model matrices
uniform mat4 in_volumeMatrix[1];
uniform mat4 in_inverseVolumeMatrix[1];
uniform mat4 in_projectionMatrix;
uniform mat4 in_inverseProjectionMatrix;
uniform mat4 in_modelViewMatrix;
uniform mat4 in_inverseModelViewMatrix;
uniform mat4 in_textureDatasetMatrix[1];
uniform mat4 in_inverseTextureDatasetMatrix[1];
in mat4 ip_inverseTextureDataAdjusted;
uniform vec3 in_texMin;
uniform vec3 in_texMax;
uniform mat4 in_textureToEye[1];

// Ray step size
uniform vec3 in_cellStep[1];
uniform vec2 in_scalarsRange[4];
uniform vec3 in_cellSpacing[1];

// Sample distance
uniform float in_sampleDistance;

// Scales
uniform vec3 in_cellScale;
uniform vec2 in_windowLowerLeftCorner;
uniform vec2 in_inverseOriginalWindowSize;
uniform vec2 in_inverseWindowSize;
uniform vec3 in_textureExtentsMax;
uniform vec3 in_textureExtentsMin;

// Material and lighting
uniform vec3 in_diffuse[4];
uniform vec3 in_ambient[4];
uniform vec3 in_specular[4];
uniform float in_shininess[4];

// Others
uniform bool in_cellFlag;
uniform bool in_useJittering;
vec3 g_rayJitter = vec3(0.0);
uniform bool in_clampDepthToBackface;

uniform vec2 in_averageIPRange;
uniform bool in_twoSidedLighting;
vec3 g_xvec;
vec3 g_yvec;
vec3 g_zvec;
uniform vec3 in_lightAmbientColor[1];
uniform vec3 in_lightDiffuseColor[1];
uniform vec3 in_lightSpecularColor[1];
vec4 g_lightPosObj;
vec3 g_ldir;
vec3 g_vdir;
vec3 g_h;
bool l_updateDepth;
vec3 l_opaqueFragPos;

const float g_opacityThreshold = 1.0 - 1.0 / 255.0;

int clippingPlanesSize;
vec3 objRayDir;
mat4 textureToObjMat;

//VTK::GradientCache::Dec

uniform sampler2D in_opacityTransferFunc_0[1];
float computeOpacity(vec4 scalar)
{
  return texture2D(in_opacityTransferFunc_0[0], vec2(scalar.w, 0)).r;
}

// c is short for component
vec4 computeGradient(int c)
{
  // Approximate Nabla(F) derivatives with central differences.
  vec3 g1; // F_front
  vec3 g2; // F_back
  g1.x = texture3D(in_volume[0], vec3(g_dataPos + g_xvec))[c];
  g1.y = texture3D(in_volume[0], vec3(g_dataPos + g_yvec))[c];
  g1.z = texture3D(in_volume[0], vec3(g_dataPos + g_zvec))[c];
  g2.x = texture3D(in_volume[0], vec3(g_dataPos - g_xvec))[c];
  g2.y = texture3D(in_volume[0], vec3(g_dataPos - g_yvec))[c];
  g2.z = texture3D(in_volume[0], vec3(g_dataPos - g_zvec))[c];

  // Apply scale and bias to the fetched values.
  g1 = g1 * in_volume_scale[0][c] + in_volume_bias[0][c];
  g2 = g2 * in_volume_scale[0][c] + in_volume_bias[0][c];

  // Central differences: (F_front - F_back) / 2h
  // This version of computeGradient() is only used for lighting
  // calculations (only direction matters), hence the difference is
  // not scaled by 2h and a dummy gradient mag is returned (-1.).
  return vec4((g1 - g2), -1.0);
}



uniform sampler2D in_colorTransferFunc_0[1];
vec3 computeRayDirection()
{
  return normalize(ip_vertexPos.xyz - g_eyePosObj.xyz);
}

//VTK::Picking::Dec

//VTK::RenderToImage::Dec

//VTK::DepthPeeling::Dec

/// We support only 8 clipping planes for now
/// The first value is the size of the data array for clipping
/// planes (origin, normal)
uniform float in_clippingPlanes[49];
uniform float in_scale;
uniform float in_bias;

//////////////////////////////////////////////////////////////////////////////
///
/// Helper functions
///
//////////////////////////////////////////////////////////////////////////////

/**
 * Transform window coordinate to NDC.
 */
vec4 WindowToNDC(const float xCoord, const float yCoord, const float zCoord)
{
  vec4 NDCCoord = vec4(0.0, 0.0, 0.0, 1.0);

  NDCCoord.x =
    (xCoord - in_windowLowerLeftCorner.x) * 2.0 * in_inverseWindowSize.x - 1.0;
  NDCCoord.y =
    (yCoord - in_windowLowerLeftCorner.y) * 2.0 * in_inverseWindowSize.y - 1.0;
  NDCCoord.z = (2.0 * zCoord - (gl_DepthRange.near + gl_DepthRange.far)) /
    gl_DepthRange.diff;

  return NDCCoord;
}

/**
 * Transform NDC coordinate to window coordinates.
 */
vec4 NDCToWindow(const float xNDC, const float yNDC, const float zNDC)
{
  vec4 WinCoord = vec4(0.0, 0.0, 0.0, 1.0);

  WinCoord.x =
    (xNDC + 1.f) / (2.f * in_inverseWindowSize.x) + in_windowLowerLeftCorner.x;
  WinCoord.y =
    (yNDC + 1.f) / (2.f * in_inverseWindowSize.y) + in_windowLowerLeftCorner.y;
  WinCoord.z =
    (zNDC * gl_DepthRange.diff + (gl_DepthRange.near + gl_DepthRange.far)) /
    2.f;

  return WinCoord;
}

//////////////////////////////////////////////////////////////////////////////
///
/// Ray-casting
///
//////////////////////////////////////////////////////////////////////////////

/**
 * Global initialization. This method should only be called once per shader
 * invocation regardless of whether castRay() is called several times (e.g.
 * vtkDualDepthPeelingPass). Any castRay() specific initialization should be
 * placed within that function.
 */
void initializeRayCast()
{
  /// Initialize g_fragColor (output) to 0
  g_fragColor = vec4(0.0);
  g_dirStep = vec3(0.0);
  g_srcColor = vec4(0.0);
  g_exit = false;

  bool l_adjustTextureExtents = !in_cellFlag;
  // Get the 3D texture coordinates for lookup into the in_volume dataset
  g_dataPos = ip_textureCoords.xyz;

  // Eye position in dataset space
  g_eyePosObj = (in_inverseVolumeMatrix[0] * vec4(in_cameraPos, 1.0));
  if (g_eyePosObj.w != 0.0)
  {
    g_eyePosObj.x /= g_eyePosObj.w;
    g_eyePosObj.y /= g_eyePosObj.w;
    g_eyePosObj.z /= g_eyePosObj.w;
    g_eyePosObj.w = 1.0;
  }

  // Getting the ray marching direction (in dataset space);
  vec3 rayDir = computeRayDirection();

  // Multiply the raymarching direction with the step size to get the
  // sub-step size we need to take at each raymarching step
  g_dirStep =
    (ip_inverseTextureDataAdjusted * vec4(rayDir, 0.0)).xyz * in_sampleDistance;

  // 2D Texture fragment coordinates [0,1] from fragment coordinates.
  // The frame buffer texture has the size of the plain buffer but
  // we use a fraction of it. The texture coordinate is less than 1 if
  // the reduction factor is less than 1.
  // Device coordinates are between -1 and 1. We need texture
  // coordinates between 0 and 1. The in_noiseSampler and in_depthSampler
  // buffers have the original size buffer.
  vec2 fragTexCoord =
    (gl_FragCoord.xy - in_windowLowerLeftCorner) * in_inverseWindowSize;

  if (in_useJittering)
  {
    float jitterValue = texture2D(in_noiseSampler, fragTexCoord).x;
    g_rayJitter = g_dirStep * jitterValue;
    g_dataPos += g_rayJitter;
  }
  else
  {
    g_dataPos += g_dirStep;
  }

  // Flag to deternmine if voxel should be considered for the rendering
  g_skip = false;
  // Light position in dataset space
  g_lightPosObj = (in_inverseVolumeMatrix[0] * vec4(in_cameraPos, 1.0));
  if (g_lightPosObj.w != 0.0)
  {
    g_lightPosObj.x /= g_lightPosObj.w;
    g_lightPosObj.y /= g_lightPosObj.w;
    g_lightPosObj.z /= g_lightPosObj.w;
    g_lightPosObj.w = 1.0;
  }
  g_ldir = normalize(g_lightPosObj.xyz - ip_vertexPos);
  g_vdir = normalize(g_eyePosObj.xyz - ip_vertexPos);
  g_h = normalize(g_ldir + g_vdir);
  g_xvec = vec3(in_cellStep[0].x, 0.0, 0.0);
  g_yvec = vec3(0.0, in_cellStep[0].y, 0.0);
  g_zvec = vec3(0.0, 0.0, in_cellStep[0].z);

  l_updateDepth = true;
  l_opaqueFragPos = vec3(0.0);

  // Flag to indicate if the raymarch loop should terminate
  bool stop = false;

  g_terminatePointMax = 0.0;

#ifdef GL_ES
  vec4 l_depthValue = vec4(1.0, 1.0, 1.0, 1.0);
#else
  vec4 l_depthValue = texture2D(in_depthSampler, fragTexCoord);
#endif
  // Depth test
  if (gl_FragCoord.z >= l_depthValue.x)
  {
    discard;
  }

  // color buffer or max scalar buffer have a reduced size.
  fragTexCoord =
    (gl_FragCoord.xy - in_windowLowerLeftCorner) * in_inverseOriginalWindowSize;

  // Compute max number of iterations it will take before we hit
  // the termination point

  // Abscissa of the point on the depth buffer along the ray.
  // point in texture coordinates
  vec4 terminatePoint =
    WindowToNDC(gl_FragCoord.x, gl_FragCoord.y, l_depthValue.x);

  // From normalized device coordinates to eye coordinates.
  // in_projectionMatrix is inversed because of way VT
  // From eye coordinates to texture coordinates
  terminatePoint = ip_inverseTextureDataAdjusted * in_inverseVolumeMatrix[0] *
    in_inverseModelViewMatrix * in_inverseProjectionMatrix * terminatePoint;
  terminatePoint /= terminatePoint.w;

  g_terminatePointMax =
    length(terminatePoint.xyz - g_dataPos.xyz) / length(g_dirStep);
  g_currentT = 0.0;

  //VTK::RenderToImage::Init

  //VTK::DepthPass::Init
}

/**
 * March along the ray direction sampling the volume texture.  This function
 * takes a start and end point as arguments but it is up to the specific render
 * pass implementation to use these values (e.g. vtkDualDepthPeelingPass). The
 * mapper does not use these values by default, instead it uses the number of
 * steps defined by g_terminatePointMax.
 */
vec4 castRay(const float zStart, const float zEnd)
{
  //VTK::DepthPeeling::Ray::Init

  //VTK::DepthPeeling::Ray::PathCheck

  /// For all samples along the ray
  while (!g_exit)
  {

    g_skip = false;
    if (!g_skip && g_srcColor.a > 0.0 && l_updateDepth)
    {
      l_opaqueFragPos = g_dataPos;
      l_updateDepth = false;
    }

    //VTK::PreComputeGradients::Impl

    if (!g_skip)
    {
      vec4 scalar = texture3D(in_volume[0], g_dataPos);
      scalar.r = scalar.r * in_volume_scale[0].r + in_volume_bias[0].r;
      scalar = vec4(scalar.r, scalar.r, scalar.r, scalar.r);
      g_scalar = scalar;
      g_srcColor = vec4(0.0);
      g_srcColor.a = computeOpacity(scalar);
    }

    //VTK::RenderToImage::Impl

    //VTK::DepthPass::Impl

    /// Advance ray
    g_dataPos += g_dirStep;

    if (any(greaterThan(g_dataPos, in_texMax)) ||
      any(lessThan(g_dataPos, in_texMin)))
    {
      break;
    }

    // Early ray termination
    // if the currently composited colour alpha is already fully saturated
    // we terminated the loop or if we have hit an obstacle in the
    // direction of they ray (using depth buffer) we terminate as well.
    if ((g_fragColor.a > g_opacityThreshold) ||
      g_currentT >= g_terminatePointMax)
    {
      break;
    }
    ++g_currentT;
  }

  return g_fragColor;
}

/**
 * Finalize specific modes and set output data.
 */
void finalizeRayCast()
{

  //VTK::Picking::Exit

  //VTK::RenderToImage::Exit
  if (l_opaqueFragPos == vec3(0.0))
  {
    gl_FragData[0] = vec4(0.0);
  }
  else
  {
    gl_FragData[0] =
      texture2D(in_colorTransferFunc_0[0], vec2(l_opaqueFragPos.z, 0.0)).xyzw;
  }

  //VTK::DepthPass::Exit
}

//////////////////////////////////////////////////////////////////////////////
///
/// Main
///
//////////////////////////////////////////////////////////////////////////////
void main()
{
  //VTK::CallWorker::Impl
}
