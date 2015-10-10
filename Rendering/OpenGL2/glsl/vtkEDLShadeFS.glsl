//VTK::System::Dec

/*=========================================================================

   Program: VTK
   Module:  vtkEDLShadeFS.glsl

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
//    EyeDome Lighting - Simplified version for use in VTK
//        - oriented light
//        - no focus
//        - some uniforms transformed to local variables
//
//        C.B. - 3 feb. 2009
//
//      IN:    Depth buffer of the scene
//             r = recorded z, in [0:1]
//      OUT:   EDL shaded image
//
//////////////////////////////////////////////////////////////////////////

// the output of this shader
//VTK::Output::Dec

varying vec2 tcoordVC;

/**************************************************/
uniform sampler2D    s2_depth; // - Z Map
uniform float        d;        // [1.0 in full res - 2.0 at lower res]
                               //- Extension in image space, in pixels
uniform vec4         N[8];     //- Array of neighbours
                               // [No support for TabUniform in VTK
                               // --> constant array, hereafter]
uniform float        F_scale;  // [5.] - Shading amplification factor

uniform float        SX;      // - pixel horizontal step (image distance: 1/w)
uniform float        SY;      //- pixel vertical step (image distance: 1/h)
uniform float        Znear;     // near clipping plane
uniform float        Zfar;      // far clipping plane
uniform float        SceneSize; // typical scene size, to scale the depth by.

uniform vec3         L;         // [0.,0.,-1.] - Light direction [frontal]
/**************************************************/

/**************************************************/
int    Nnb = 1;  // nombre de voisins par rayon
float  Zm  = 0.; // minimal z in image
float  ZM  = 1.; // maximal z in image
float  Z;        // initial Z

vec3   WHITE3 = vec3(1.,1.,1.);

float    t;
vec4     Zn[8];  // profondeurs des voisins
float    D[8];   // ombrage genere par les voisins
vec4     tn, tnw, tw, tsw, ts, tse, te, tne;
float    dn, dnw, dw, dsw, ds, dse, de, dne;
float    S;      // image step, corresponds to one pixel size
/**************************************************/

//////////////////////////////////////////////////////////////////////////
//
//    Local shading functions
//
//    Pseudo angle, avec S (distance pixel) valant l'unite
//    zi      elevation of current pixel
//    zj      elevation of its neighbour
//    delta   distance between the two
float angleP(float zi, float zj, float delta)
{
  return max(0.,zj-zi) / (delta/S);
}

//    zi      elevation of current pixel
//    zj      elevation of its neighbour
//    delta   distance between the two
float obscurance(float zi, float zj, float delta)
{
  return angleP(zi,zj,delta);
}
//
//    Local shading functions
//
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
//
//    Z transformation
//
float zflip(float z)
{
  return 1. - z;
}

float zscale(float z)
{
  return clamp((z-Zm)/(ZM-Zm),0.,1.);
}

//    Inversion of OpenGL perspective projection
//    (should be adapted for orthographic projection)
//
float ztransform(float z)
{
  float Z;
  Z = (z-0.5)*2.;
  Z = -2.*Zfar*Znear/( (Zfar-Znear) * (Z-(Zfar+Znear)/(Zfar-Znear)) );
  Z = (Z-Znear)/SceneSize;
  return 1.-Z;
}
//
//      Z transformation
//
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
//
//      NEIGHBORHOOD    SHADING
//
void computeNeighbours8(float dist)
{
  // Plan Lumiere-point
  vec4  P =    vec4( L.xyz , -dot(L.xyz,vec3(0.,0.,t)) );

  // 0 at the back of the scene
  int   c;
  vec2  V;  // pixel voisin
  float di = dist;
  float Znp[8]; // profondeur des 8 voisins sur le plan

  for(c=0; c<8;c++)
    {
    V = tcoordVC.st + di*vec2(SX,SY)*N[c].xy;
    Zn[c].x = ztransform(texture2D(s2_depth,V).r);
    // profondeur du voisin reel dans l'image

    // VERSION qui ombre le fond
    Znp[c] = dot( vec4(di*vec2(SX,SY)*N[c].xy, Zn[c].x, 1.0) , P );
    }

  dn    =  obscurance( 0., Znp[0] ,di*SX);
  dnw   =  obscurance( 0., Znp[1],di*SX);
  dw    =  obscurance( 0., Znp[2] ,di*SX);
  dsw   =  obscurance( 0., Znp[3],di*SX);
  ds    =  obscurance( 0., Znp[4] ,di*SX);
  dse   =  obscurance( 0., Znp[5],di*SX);
  de    =  obscurance( 0., Znp[6] ,di*SX);
  dne   =  obscurance( 0., Znp[7],di*SX);
}

float computeObscurance(float F,float scale,float weight)
{
  computeNeighbours8( scale );

  float S  =  F;
  float WE =  weight;

  S += dn  * WE;
  S += dnw * WE;
  S += dw  * WE;
  S += dsw * WE;
  S += ds  * WE;
  S += dse * WE;
  S += de  * WE;
  S += dne * WE;

  return S;
}

void ambientOcclusion()
{
  float F       = 0.;
  float weight  = 20.; // 2. * 3.14159;

  F = computeObscurance(F,d,weight);
  F = exp(-F_scale*F);

  gl_FragData[0] = vec4(F,F,F,Z);
}

void main (void)
{
  S  = SX;
  Z  = texture2D(s2_depth, tcoordVC.st).r;
  t  = ztransform(Z);

  ambientOcclusion();
}
