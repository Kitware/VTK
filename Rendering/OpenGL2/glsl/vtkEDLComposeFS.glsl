//VTK::System::Dec

/*=========================================================================

   Program: VTK
   Module:  vtkEDLComposeFS.glsl

  Copyright (c) Sandia Corporation, Kitware Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------
Acknowledgement:
This algorithm is the result of joint work by Electricité de France,
CNRS, Collège de France and Université J. Fourier as part of the
Ph.D. thesis of Christian BOUCHENY.
------------------------------------------------------------------------*/
//////////////////////////////////////////////////////////////////////////
//
//
//  EyeDome Lighting - Compositing - Simplified version for use in VTK\n
//
//    C.B. - 3 feb. 2009
//
//////////////////////////////////////////////////////////////////////////

// the output of this shader
//VTK::Output::Dec

in vec2 tcoordVC;

/**************************************************/
uniform sampler2D    s2_S1;  // fine scale
uniform sampler2D    s2_S2;  // larger medium scale
uniform sampler2D    s2_C;   // scene color image
/**************************************************/

void main (void)
{
  vec4 shade1  =  texture2D(s2_S1,tcoordVC.st);
  vec4 shade2  =  texture2D(s2_S2,tcoordVC.st);
  vec4  color   =  texture2D(s2_C,tcoordVC.st);

  // if it is the background (ala depth > 0.99) just copy it
  //if(shade1.a > 0.99)
  //  {
  //  gl_FragData[0] = vec4(shade1.rgb,1.) * color;
  //  }
  //else
  //  {
    float lum = mix(shade1.r,shade2.r,0.3);
    gl_FragData[0] = vec4(color.rgb*lum, color.a);
  //  }

  gl_FragDepth = shade1.a; // write stored depth
}
