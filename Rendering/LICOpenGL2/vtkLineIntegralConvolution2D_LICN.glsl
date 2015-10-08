//VTK::System::Dec

//=========================================================================
//
//  Program:   Visualization Toolkit
//  Module:    vtkLineIntegralConvolution2D_LICN.glsl
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

// the output of this shader
//VTK::Output::Dec

/**
This shader finalizes the convolution for the LIC computation
applying the normalization. eg. if box kernel is used the this
is the number of steps taken.
*/

uniform sampler2D texLIC;

varying vec2 tcoordVC;

void main(void)
{
  vec4 conv = texture2D(texLIC, tcoordVC.st);
  conv.r = conv.r/conv.a;
  // lic => (convolution, mask, 0, 1)
  gl_FragData[0] = vec4(conv.rg , 0.0, 1.0);
}
