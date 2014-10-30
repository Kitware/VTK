/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkglprojectedTetrahdraFS.glsl

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// The following line handle system declarations such a
// default precisions, or defining precisions to null
//VTK::System::Dec

varying vec3 fcolor;
varying float fdepth;
varying float fattenuation;

void main()
{
  // the following exp is done in the fragment shader
  // because linear interpolation (from the VS) of the resulting
  // value would not match the exp of the interpolated
  // source values
  float opacity = 1.0 - exp(-1.0*fattenuation*fdepth);


  gl_FragColor =  vec4(opacity*fcolor,opacity);

  if (gl_FragColor.a <= 0.0)
    {
    discard;
    }
}
