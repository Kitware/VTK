/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDepthPeelingPassIntermediateFS.glsl

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
uniform sampler2D currentRGBATexture;

void main()
{
  vec4 t1Color = texture2D(translucentRGBATexture, tcoordVC);
  vec4 t2Color = texture2D(currentRGBATexture, tcoordVC);
  gl_FragColor.a = t1Color.a + t2Color.a * (1.0-t1Color.a);
  if (gl_FragColor.a > 0.0)
    {
    gl_FragColor.rgb = (t1Color.rgb*t1Color.a + t2Color.rgb*t2Color.a*(1.0-t1Color.a))/gl_FragColor.a;
    }
  else
    {
    gl_FragColor.rgb = vec3(0.0,0.0,0.0);
    }
}
