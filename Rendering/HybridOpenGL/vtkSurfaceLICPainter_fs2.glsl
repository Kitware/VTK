//=========================================================================
//
//  Program:   Visualization Toolkit
//  Module:    vtkSurfaceLICPainter_fs2.glsl
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

// Filename: vtkSurfaceLICPainter_fs2.glsl
// Filename is useful when using gldb-gui

#version 110

uniform sampler2D texLIC;
uniform sampler2D texGeometry;
uniform sampler2D texDepth;
uniform float     uLICIntensity;

vec3    texMasker = vec3( -1.0, -1.0, -1.0 ); // for zero-vector fragments

void main()
{
  float fragDepth = texture2D( texDepth,    gl_TexCoord[1].st ).b;

  if ( fragDepth == 0.0 )
    {
    discard;
    }

  vec3  licTexVal = texture2D( texLIC,      gl_TexCoord[0].st ).rgb;
  vec4  geomColor = texture2D( texGeometry, gl_TexCoord[1].st );

  // In pass #1 LIC (providing a low-quality image during user interaction)
  // or pass #2 LIC (providing an improved image when no user interaction),
  // both in vtkLineIntegralConvolution2D_fs1, any fragment where the surface
  // vector is zero is assigned with a masking texture value vec3( -1.0, -1.0,
  // -1.0 ). Such fragments need to be made totally transparent to show the
  // underlying geoemtry surface.
  bvec3 isMaskVal = equal( licTexVal, texMasker );
  int   rejectLIC = int(  all( isMaskVal )  );

  vec4  tempColor = vec4(   (  licTexVal     *         uLICIntensity +
                               geomColor.xyz * ( 1.0 - uLICIntensity )
                            ), geomColor.a
                        );
  tempColor = float( 1 - rejectLIC ) * tempColor +
              float(     rejectLIC ) * geomColor;

  gl_FragColor = tempColor;
  gl_FragDepth = fragDepth;
}
