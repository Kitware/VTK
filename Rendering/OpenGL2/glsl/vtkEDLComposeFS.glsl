//VTK::System::Dec

/*=========================================================================

   Program: VTK
   Module:  vtkEDLComposeFS.glsl

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
//  EyeDome Lighting - Compositing - Simplified version for use in VTK\n
//
//    C.B. - 3 feb. 2009
//
//////////////////////////////////////////////////////////////////////////

// the output of this shader
//VTK::Output::Dec

varying vec2 tcoordVC;

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
