//VTK::System::Dec

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

varying vec2 tcoordVC;
uniform sampler2D translucentRGBATexture;
uniform sampler2D currentRGBATexture;

uniform int lastpass;

// the output of this shader
//VTK::Output::Dec

void main()
{
  // current is what is most recently rendered
  // translucent is what is in FRONT of it
  // we render front to back
  vec4 t1Color = texture2D(translucentRGBATexture, tcoordVC);
  vec4 t2Color = texture2D(currentRGBATexture, tcoordVC);
  gl_FragData[0].a = t1Color.a + t2Color.a * (1.0-t1Color.a);
  if (gl_FragData[0].a > 0.0)
    {
    if (lastpass == 1)
      {
      gl_FragData[0].rgb = (t1Color.rgb*t1Color.a + t2Color.rgb*(1.0-t1Color.a))/gl_FragData[0].a;
      }
    else
      {
      gl_FragData[0].rgb = (t1Color.rgb*t1Color.a + t2Color.rgb*t2Color.a*(1.0-t1Color.a))/gl_FragData[0].a;
      }
    }
  else
    {
    gl_FragData[0].rgb = vec3(0.0,0.0,0.0);
    }
}
