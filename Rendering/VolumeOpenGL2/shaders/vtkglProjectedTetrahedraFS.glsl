//VTK::System::Dec

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

//VTK::Output::Dec

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


  gl_FragData[0] =  vec4(fcolor,opacity);

  if (gl_FragData[0].a <= 0.0)
    {
    discard;
    }
}
