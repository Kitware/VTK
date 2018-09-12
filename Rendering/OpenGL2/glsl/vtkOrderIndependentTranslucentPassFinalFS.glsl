//VTK::System::Dec

/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDepthPeelingPassFinalFS.glsl

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

in vec2 texCoord;

uniform sampler2D translucentRTexture;
uniform sampler2D translucentRGBATexture;

// the output of this shader
//VTK::Output::Dec

void main()
{
  vec4 t1Color = texture(translucentRGBATexture, texCoord);
  float t2Color = texture(translucentRTexture, texCoord).r;

  gl_FragData[0] = vec4(t1Color.rgb/max(t2Color,0.01), t1Color.a);
  // gl_FragData[0] = vec4(t1Color.a, t1Color.a, t1Color.a, 0.0);
  // gl_FragData[0] = vec4(t2Color, t2Color, t2Color, 0.0);
}
