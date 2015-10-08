//VTK::System::Dec

/*=========================================================================

   Program: VTK
   Module:  vtkEDLBilateralFilterFS.glsl

   Copyright (c) 2005-2008 Sandia Corporation, Kitware Inc.
   All rights reserved.

   ParaView is a free software; you can redistribute it and/or modify it
   under the terms of the ParaView license version 1.2.

   See License_v1.2.txt for the full ParaView license.
   A copy of this license can be obtained by contacting
   Kitware Inc.
   28 Corporate Drive
   Clifton Park, NY 12065
   USA

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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

varying vec2 tcoordVC;

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
