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
varying vec3 ip_textureCoords;
varying vec3 ip_vertexPos;

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

uniform vec4 in_volume_scale;
uniform vec4 in_volume_bias;

//VTK::Output::Dec

//VTK::Base::Dec

//VTK::Termination::Dec

//VTK::Cropping::Dec

//VTK::Shading::Dec

//VTK::BinaryMask::Dec

//VTK::CompositeMask::Dec

//VTK::ComputeOpacity::Dec

//VTK::ComputeGradient::Dec

//VTK::ComputeLighting::Dec

//VTK::ComputeColor::Dec

//VTK::ComputeRayDirection::Dec

//VTK::Picking::Dec

/// We support only 8 clipping planes for now
/// The first value is the size of the data array for clipping
/// planes (origin, normal)
uniform float in_clippingPlanes[49];
uniform float in_scale;
uniform float in_bias;

//////////////////////////////////////////////////////////////////////////////
///
/// Main
///
//////////////////////////////////////////////////////////////////////////////
void main()
  {
  /// Initialize g_fragColor (output) to 0
  g_fragColor = vec4(0.0);
  g_dirStep = vec3(0.0);
  g_srcColor = vec4(0.0);
  g_exit = false;

  //VTK::Base::Init

  //VTK::Terminate::Init

  //VTK::Shading::Init

  //VTK::Cropping::Init

  //VTK::Clipping::Init

  //VTK::RenderToImage::Init

  //VTK::DepthPass::Init

  /// For all samples along the ray
  while (!g_exit)
    {
    //VTK::Base::Impl

    //VTK::Cropping::Impl

    //VTK::Clipping::Impl

    //VTK::BinaryMask::Impl

    //VTK::CompositeMask::Impl

    //VTK::Shading::Impl

    //VTK::RenderToImage::Impl

    //VTK::DepthPass::Impl

    /// Advance ray
    g_dataPos += g_dirStep;

    //VTK::Terminate::Impl
    }

  //VTK::Base::Exit

  //VTK::Terminate::Exit

  //VTK::Cropping::Exit

  //VTK::Clipping::Exit

  //VTK::Shading::Exit

  //VTK::Picking::Exit

  g_fragColor.r = g_fragColor.r * in_scale + in_bias * g_fragColor.a;
  g_fragColor.g = g_fragColor.g * in_scale + in_bias * g_fragColor.a;
  g_fragColor.b = g_fragColor.b * in_scale + in_bias * g_fragColor.a;
  gl_FragData[0] = g_fragColor;

  //VTK::RenderToImage::Exit

  //VTK::DepthPass::Exit
  }
