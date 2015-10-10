//VTK::System::Dec

//=========================================================================
//
//  Program:   Visualization Toolkit
//  Module:    vtkSurfaceLICMapper_DCpy.glsl
//
//  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
//  All rights reserved.
//  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.
//
//     This software is distributed WITHOUT ANY WARRANTY; without even
//     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//     PURPOSE.  See the above copyright notice for more information.
//
//=========================================================================

// This shader copies fragments and depths to the output buffer

// the output of this shader
//VTK::Output::Dec

uniform sampler2D texDepth;     // z values from vertex shader
uniform sampler2D texRGBColors; // final rgb LIC colors

varying vec2 tcoordVC;

void main()
{
  gl_FragDepth = texture2D(texDepth, tcoordVC).x;
  gl_FragData[0] = texture2D(texRGBColors, tcoordVC);

  // since we render a screen aligned quad
  // we're going to be writing fragments
  // not touched by the original geometry
  // it's critical not to modify those
  // fragments.
  if (gl_FragDepth == 1.0)
    {
    discard;
    }
}
