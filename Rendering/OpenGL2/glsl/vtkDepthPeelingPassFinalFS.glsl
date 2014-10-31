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

// The following line handle system declarations such a
// default precisions, or defining precisions to null
//VTK::System::Dec

varying vec2 tcoordVC;

uniform sampler2D translucentRGBATexture;
uniform sampler2D opaqueRGBATexture;

void main()
{
  vec4 t1Color = texture2D(translucentRGBATexture, tcoordVC);
  vec4 t2Color = texture2D(opaqueRGBATexture, tcoordVC);
  gl_FragColor.a = 1.0;
  gl_FragColor.rgb = (t1Color.rgb*t1Color.a + t2Color.rgb*(1.0-t1Color.a));
}
