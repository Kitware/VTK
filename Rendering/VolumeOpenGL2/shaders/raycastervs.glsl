/*=========================================================================

  Program:   Visualization Toolkit
  Module:    raycastervs.glsl

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#version 120

/// Needed to enable inverse function
#extension GL_ARB_gpu_shader5 : enable

// The following line handle system declarations such a
// default precisions, or defining precisions to null
//VTK::System::Dec

//////////////////////////////////////////////////////////////////////////////
///
/// Uniforms, attributes, and globals
///
//////////////////////////////////////////////////////////////////////////////
//VTK::Base::Dec
//VTK::Termination::Dec
//VTK::Cropping::Dec
//VTK::Shading::Dec

//////////////////////////////////////////////////////////////////////////////
///
/// Inputs
///
//////////////////////////////////////////////////////////////////////////////
attribute vec3 m_in_vertex_pos;

//////////////////////////////////////////////////////////////////////////////
///
/// Outputs
///
//////////////////////////////////////////////////////////////////////////////
/// 3D texture coordinates for texture lookup in the fragment shader
varying vec3 m_texture_coords;
varying vec3 m_vertex_pos;

void main()
{
  /// Get clipspace position
  //VTK::ComputeClipPos::Impl

  /// Compute texture coordinates
  //VTK::ComputeTextureCoords::Impl

  /// Copy incoming vertex position for the fragment shader
  m_vertex_pos = m_in_vertex_pos;
}
