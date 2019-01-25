//VTK::System::Dec

/*=========================================================================

   Program: VTK
   Module:  vtkEDLBilateralFilterFS.glsl

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
//  Bilateral filtering
//
//  C.B. - 16 aout 2008
//
//    IN:
//      s2_I - Image to blur
//      s2_D - Modulating depth image
//
//    OUT:
//      Filtered image
//
//////////////////////////////////////////////////////////////////////////

// the output of this shader
//VTK::Output::Dec

in vec2 tcoordVC;

/****************************************************/
uniform sampler2D   s2_I;
uniform sampler2D   s2_D;
uniform float       SX;
uniform float       SY;
uniform int         N;
// filter size (full width, necessarily odd, like 3, 5...)
uniform float       sigma;
/****************************************************/

/****************************************************/
vec3    C;
float   z;
float   sigmaz = 0.005;
/****************************************************/

void main (void)
{
  C = texture2D(s2_I, tcoordVC.st).rgb;
  z = texture2D(s2_D, tcoordVC.st).r;

  float ALL = 0.;       // sum of all weights
  vec3  RES = vec3(0.); // sum of all contributions
  int   hN  = N/2;      // filter half width
  vec2  coordi = vec2(0.,0.);
  vec3  Ci;
  float zi;
  float dist;
  float dz;
  float Fi,Gi;

  int   c,d;
  for(c=-hN;c<hN+1;c++)
  {
    for(d=-hN;d<hN+1;d++)
    {
    coordi = vec2(float(c)*SX,float(d)*SY);
    Ci = texture2D(s2_I, tcoordVC.st+coordi).rgb;
    zi = texture2D(s2_D, tcoordVC.st+coordi).r;

    dist = clamp( float(c*c+d*d)/float(hN*hN) , 0., 1. );
    dz   = (z-zi)*(z-zi);

    Fi = exp(-dist*dist/(2.* sigma*sigma));
    Gi = exp(-dz*dz/(2.* sigmaz*sigmaz));

    RES += Ci * Fi * Gi;
    ALL += Fi * Gi;
    }
  }
  RES /= ALL;

  gl_FragData[0] = vec4( RES , z );
}
