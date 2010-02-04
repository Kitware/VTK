//=========================================================================
//
//  Program:   Visualization Toolkit
//  Module:    vtkLineIntegralConvolution2D_fs2.glsl
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

// Filename: vtkLineIntegralConvolution2D_fs2.glsl
// Filename is useful when using gldb-gui

#version 110

#extension GL_ARB_draw_buffers : enable

uniform sampler2D licTexture;
uniform float     uLicTexWid; // texture width
uniform float     uLicTexHgt; // texture height

// shift to the neighboring fragment
float tcordxDelt = 1.0 / uLicTexWid;
float tcordyDelt = 1.0 / uLicTexHgt;

// the 8 surrounding fragments accessed by the 3x3 Laplacian matrix
// -1 -1 -1
// -1  9 -1             
// -1 -1 -1
vec2  cordShift0 = vec2( -tcordxDelt,  tcordyDelt );
vec2  cordShift1 = vec2(  0.0,         tcordyDelt );
vec2  cordShift2 = vec2(  tcordxDelt,  tcordyDelt );

vec2  cordShift3 = vec2( -tcordxDelt,  0.0        );
vec2  cordShift4 = vec2(  tcordxDelt,  0.0        );

vec2  cordShift5 = vec2( -tcordxDelt, -tcordyDelt );
vec2  cordShift6 = vec2(  0.0,        -tcordyDelt );
vec2  cordShift7 = vec2(  tcordxDelt, -tcordyDelt );

// used for handling exceptions
vec2  miniTCoord = vec2(       tcordxDelt,       tcordyDelt );
vec2  maxiTCoord = vec2( 1.0 - tcordxDelt, 1.0 - tcordyDelt );
vec4  miniTexVal = vec4( 0.0, 0.0, 0.0, 0.0 );
vec4  maxiTexVal = vec4( 1.0, 1.0, 1.0, 1.0 );
     
// perform a 3x3 Laplacian high-pass filter on the input image  
void main( void )
{
  int   bException;
  vec4  outputValu;
  vec4  fragTexVal = texture2D( licTexture, gl_TexCoord[0].st );
  
  // In pass #1 LIC (vtkLineIntegralConvolution2D_fs1), any fragment where 
  // the vector is zero is assigned with a negative texture value (by negating
  // the associated input noise texture value). High-pass filtering is skipped
  // for this fragment in order to pass the original input noise value forward
  // to pass #2 LIC. The line below checks if the current is such a fragment.
  bvec4 exception0 = lessThan( fragTexVal, miniTexVal );
     
  // checks if this fragment has 8 valid surrounding fragments (in tcoords)
  bvec2 exception1 = lessThan   ( gl_TexCoord[0].st, miniTCoord );
  bvec2 exception2 = greaterThan( gl_TexCoord[0].st, maxiTCoord );
  
  // perform high-pass filtering
  outputValu  = fragTexVal * 9.0;
  outputValu -= texture2D( licTexture, gl_TexCoord[0].st + cordShift0 ) +
                texture2D( licTexture, gl_TexCoord[0].st + cordShift1 ) +
                texture2D( licTexture, gl_TexCoord[0].st + cordShift2 ) +
       
                texture2D( licTexture, gl_TexCoord[0].st + cordShift3 ) +
                texture2D( licTexture, gl_TexCoord[0].st + cordShift4 ) +
       
                texture2D( licTexture, gl_TexCoord[0].st + cordShift5 ) +
                texture2D( licTexture, gl_TexCoord[0].st + cordShift6 ) +
                texture2D( licTexture, gl_TexCoord[0].st + cordShift7 );
  
  // Checks if high-pass filtering produces out-of-range texture values
  // that might incur artifacts near the interface between the valid flow
  // areas and zero-vector areas. In case of such a filtering result, the
  // initial texture value (from the output of pass #1 LIC) is simply
  // adopted to suppress artifacts as much as possible.
  bvec4 exception3 = lessThan   ( outputValu, miniTexVal );
  bvec4 exception4 = greaterThan( outputValu, maxiTexVal );
  bException = int(  any( exception3 )  ) + int(  any( exception4 )  );
  outputValu = fragTexVal * float(     bException ) + 
               outputValu * float( 1 - bException );
  
  // In cased of any invalid surrounding fragment, high-pass filtering is 
  // skipped and the initial texture value (from the output of pass #1 LIC)
  // is employed instead.
  bException = int(  any( exception1 )  ) + int(  any( exception2 )  );
  bException = ( bException + 1 ) / 2;
  outputValu = fragTexVal * float(     bException ) + 
               outputValu * float( 1 - bException );
  
  // In case of a zero-vector fragment, the negative texture value (the noise
  // texture value stored in the output of pass #1 LIC) is negated again below
  // to restore the positive noise texture value that is then forwarded to pass
  // #2 LIC as the input noise.             
  bException = int(  any( exception0 )  );
  outputValu = fragTexVal * float( 0 - bException ) + 
               outputValu * float( 1 - bException );
  
  gl_FragData[0]= outputValu;
}
