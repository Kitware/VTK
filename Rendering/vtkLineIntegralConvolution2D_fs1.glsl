//=========================================================================
//
//  Program:   Visualization Toolkit
//  Module:    vtkLineIntegralConvolution2D_fs1.glsl
//
//  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
//  All rights reserved.
//  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.
//
//     This software is distributed WITHOUT ANY WARRANTY; without even
//     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//     PURPOSE.  See the above copyright notice for more information.
//
//=========================================================================

// Filename: vtkLineIntegralConvolution2D_fs1.glsl
// Filename is useful when using gldb-gui

#version 110

#extension GL_ARB_draw_buffers : enable

// four input texture objects
uniform sampler2D texVectorField;    // TEXTURE0
uniform sampler2D texNoise;          // TEXTURE1
uniform sampler2D texLIC;            // TEXTURE2
uniform sampler2D texTCoords;        // TEXTURE3

// step type
// 0: first access to the streamline center point
// 1: access to a regular / non-center streamline point
// 2: second access to the streamline center point
//    (due to a change in the streamline integration direction)
//
// Texture texTCoords is indexed for type #1 (i.e., bReset = 0) only
// while it is NOT indexed for type #0 and type #2 (i.e., bReset = 1)
// and instead the original texture coordinate (prior to any integration)
// is directly used.
//
// Type #0 and type #2 each contribute half the texture value since
// they access to the same streamline point.
// 
// The accumulation texture texLIC is NOT accessed for type #0 because
// nothing has been accumulated upon the first integration step
uniform int   uStepType;
uniform int   uSurfaced;             // is surfaceLIC (0 / 1)?
uniform int   uLastPass;             // is the last pass of LIC (0 / 1)?
uniform int   uNumSteps;             // number of steps in each direction.
uniform int   uStepSign;             // +1: forward;    -1: backward.
uniform float uStepSize;             // step size in parametric space
             
// two modes for masking the texture value of a zero-vector fragment
// 0: retain the white noise texture value by storing the negated version
// 1: export ( -1.0, -1.0, -1.0, -1.0 ) for use by vtkSurfaceLICPainter
//    to make this LIC fragment totally transparent to show the underlying
//    geometry surface
uniform int   uMaskType;

float   normalizer = 1.0 / float( 2 * uNumSteps + 1 ); // for normalization

// functions defined in vtkLineIntegralConvolution2D_fs.glsl
vec2 rk2( vec2 xy, float h );
vec2 rk4( vec2 xy, float h );
vec2 getVector( vec2 tcords );
vec3 getNoiseColor( vec2 tcoord );
 

void main( void )
{ 
  vec2 vector = getVector( gl_TexCoord[1].st );
  
  // ==== for surfaceLIC ====
  // Any fragment where the vector is zero needs to be assigned with a mask 
  // texture value, either vec4( -1.0, -1.0, -1.0, -1.0 ) or the negated
  // version of the white noise texture value. The former is exploited by
  // vtkSurfaceLICPainter to make this LIC fragment totally transparent
  // to show the underlying geometry surface while the latter is used by the
  // high-pass filter (vtkLineIntegralConvolution2D_fs2, invoked between 
  // two LIC passes that are employed for improved image quality) to ignore
  // such fragments. Otherwise the output of the high-pass filter (taking
  // pass #1's output as the input) would contain high-frequency noise while
  // the (pass #2) LIC process requires white noise from the zero-vector area.
  //
  // ==== for non-surfaceLIC ====
  // Any fragment where the vector is zero needs to be assigned with the 
  // negated version of the white noise texture value ONLY UNLESS this is the
  // last pass of LIC, which is followed by a high-pass filter. Otherwise (it
  // is the last process of LIC) the fragment takes the white noise value for
  // the output.
  if (    all(   equal(  vector,  vec2( 0.0, 0.0 )  )   )    )
    {
    if ( uSurfaced == 1 ) 
      {
      gl_FragData[0] = vec4(  ( -1.0 ) * getNoiseColor( gl_TexCoord[1].st ),  
                              ( -1.0 )  )             * float( 1 - uMaskType ) +
                       vec4( -1.0, -1.0, -1.0, -1.0 ) * float(     uMaskType );
      gl_FragData[1] = vec4( -1.0, -1.0, -1.0, -1.0 );
      } 
    else
      {
      float   fscale = float( uLastPass + uLastPass - 1 );
      gl_FragData[0] = vec4(  fscale * getNoiseColor( gl_TexCoord[1].st ),  
                              fscale  );
      gl_FragData[1] = vec4(  fscale,  fscale,  fscale,  fscale  );
      }
      
    return;
    } 
    
  // determine if the texture coordinate needs to be reset
  // bReset = 0: texture texTCoords needs to be indexed to obtain the coordinate
  //             for a regular / non-center streamline point.
  // bReset = 1: the original texture coordinate (prior to any integration) is
  //             used for the streamline center point.
  int  bReset = 1 - (  ( uStepType + 1 ) / 2  ) * ( 1 - uStepType / 2 );
   
  // obtain the actual texture coordinate
  vec2 tcord0;
  if(bReset==1)
    {
    tcord0=gl_TexCoord[1].st;
    }
  else
    {
    tcord0=texture2D( texTCoords, gl_TexCoord[0].st ).rg;
    }

  // normalize the contribution of this streamline point to the center ask the 
  // streamline center to contribute half the texture value per time (the stream-
  // line center is accessed two times)
  vec3 color0 = (  1.0  -  float( bReset )  *  0.5  ) * 
                (  getNoiseColor( tcord0 )  *  normalizer  );
  
  // integration to locate the next streamline point              
  vec2 tcord1 = rk4(  tcord0,  float( uStepSign ) * uStepSize  );
  
  // access the accumulation texture to obtain the summed texture value that will 
  // be eventually assigned to the streamline center (in fact, no accumulation is
  // accessed and used for type #0 --- the first access to the center)
  // NOTE: upon the first access to the center, the accumulation texture may (and
  // in many cases, at least on some platforms) contain invalid ('NAN') values.
  // Accessing the initial accumulation texture can cause problems.
  vec3 accumu = vec3( 0.0, 0.0, 0.0 );
  if ( uStepType > 0 )
    {
    accumu = texture2D( texLIC, gl_TexCoord[0].st ).rgb;
    }
    
  gl_FragData[0] = vec4( color0 + accumu, 1.0 );
  gl_FragData[1] = vec4( tcord1.s, tcord1.t, 0.0, 1.0 );
}
