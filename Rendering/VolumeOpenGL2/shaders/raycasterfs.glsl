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
vec3 g_dirStep;
vec4 g_srcColor;
vec4 g_eyePosObj;
bool g_exit;
bool g_skip;
float g_currentT;
float g_terminatePointMax;

// These describe the entire ray for this scene, not just the current depth
// peeling segment. These are texture coordinates.
vec3 g_rayOrigin; // Entry point of volume or clip point
vec3 g_rayTermination; // Termination point (depth, clip, etc)

// These describe the current segment. If not peeling, they are initialized to
// the ray endpoints.
vec3 g_dataPos;
vec3 g_terminatePos;

//VTK::CustomUniforms::Dec

//VTK::Output::Dec

//VTK::Base::Dec

//VTK::Termination::Dec

//VTK::Cropping::Dec

//VTK::Clipping::Dec

//VTK::Shading::Dec

//VTK::BinaryMask::Dec

//VTK::CompositeMask::Dec

//VTK::GradientCache::Dec

//VTK::Transfer2D::Dec

//VTK::ComputeOpacity::Dec

//VTK::ComputeGradient::Dec

//VTK::ComputeGradientOpacity1D::Dec

//VTK::ComputeLighting::Dec

//VTK::ComputeColor::Dec

//VTK::ComputeRayDirection::Dec

//VTK::Picking::Dec

//VTK::RenderToImage::Dec

//VTK::DepthPeeling::Dec

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

  NDCCoord.x = (xCoord - in_windowLowerLeftCorner.x) * 2.0 *
    in_inverseWindowSize.x - 1.0;
  NDCCoord.y = (yCoord - in_windowLowerLeftCorner.y) * 2.0 *
    in_inverseWindowSize.y - 1.0;
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

  WinCoord.x = (xNDC + 1.f) / (2.f * in_inverseWindowSize.x) +
    in_windowLowerLeftCorner.x;
  WinCoord.y = (yNDC + 1.f) / (2.f * in_inverseWindowSize.y) +
    in_windowLowerLeftCorner.y;
  WinCoord.z = (zNDC * gl_DepthRange.diff +
    (gl_DepthRange.near + gl_DepthRange.far)) / 2.f;

  return WinCoord;
}

/**
 * Clamps the texture coordinate vector @a pos to a new position in the set
 * { start + i * step }, where i is an integer. If @a ceiling
 * is true, the sample located further in the direction of @a step is used,
 * otherwise the sample location closer to the eye is used.
 * This function assumes both start and pos already have jittering applied.
 */
vec3 ClampToSampleLocation(vec3 start, vec3 step, vec3 pos, bool ceiling)
{
  vec3 offset = pos - start;
  float stepLength = length(step);

  // Scalar projection of offset on step:
  float dist = dot(offset, step / stepLength);
  if (dist < 0.) // Don't move before the start position:
  {
    return start;
  }

  // Number of steps
  float steps = dist / stepLength;

  // If we're reeaaaaallly close, just round -- it's likely just numerical noise
  // and the value should be considered exact.
  if (abs(mod(steps, 1.)) > 1e-5)
  {
    if (ceiling)
    {
      steps = ceil(steps);
    }
    else
    {
      steps = floor(steps);
    }
  }
  else
  {
    steps = floor(steps + 0.5);
  }

  return start + steps * step;
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

  //VTK::Base::Init

  //VTK::Cropping::Init

  //VTK::Terminate::Init

  //VTK::Clipping::Init

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

  //VTK::Clipping::Impl

  //VTK::DepthPeeling::Ray::PathCheck

  //VTK::Shading::Init

  /// For all samples along the ray
  while (!g_exit)
  {
    //VTK::Base::Impl

    //VTK::Cropping::Impl

    //VTK::BinaryMask::Impl

    //VTK::CompositeMask::Impl

    //VTK::PreComputeGradients::Impl

    //VTK::Shading::Impl

    //VTK::RenderToImage::Impl

    //VTK::DepthPass::Impl

    /// Advance ray
    g_dataPos += g_dirStep;

    //VTK::Terminate::Impl
  }

  //VTK::Shading::Exit

  return g_fragColor;
}

/**
 * Finalize specific modes and set output data.
 */
void finalizeRayCast()
{
  //VTK::Base::Exit

  //VTK::Terminate::Exit

  //VTK::Cropping::Exit

  //VTK::Clipping::Exit

  //VTK::Picking::Exit

  g_fragColor.r = g_fragColor.r * in_scale + in_bias * g_fragColor.a;
  g_fragColor.g = g_fragColor.g * in_scale + in_bias * g_fragColor.a;
  g_fragColor.b = g_fragColor.b * in_scale + in_bias * g_fragColor.a;
  gl_FragData[0] = g_fragColor;

  //VTK::RenderToImage::Exit

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
