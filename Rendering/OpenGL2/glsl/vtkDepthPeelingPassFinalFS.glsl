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

varying vec2 tcoordVC;

uniform sampler2D translucentRGBATexture;
uniform sampler2D opaqueRGBATexture;
uniform sampler2D opaqueZTexture;

// the output of this shader
//VTK::Output::Dec

void main()
{
  vec4 t1Color = texture2D(translucentRGBATexture, tcoordVC);
  vec4 t2Color = texture2D(opaqueRGBATexture, tcoordVC);
  gl_FragData[0].a = t1Color.a +  (1.0-t1Color.a)*t2Color.a;
  gl_FragData[0].rgb = (t1Color.rgb*t1Color.a + t2Color.rgb*(1.0-t1Color.a));

  float depth = texture2D(opaqueZTexture, tcoordVC).x;
  gl_FragDepth = depth;
}
