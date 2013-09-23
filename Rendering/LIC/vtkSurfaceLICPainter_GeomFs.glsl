//=========================================================================
//
//  Program:   Visualization Toolkit
//  Module:    vtkSurfaceLICPainter_fs1.glsl
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

#version 110

varying vec4 vColor;
varying vec2 vProjectedVF;
varying vec3 vMaskCriteria;

void main()
{
  // save the depth for parallel use
  // 1 it identifies local fragments after composiiting
  // 2 it's used in compositing
  float depth = gl_FragCoord.z;

  gl_FragData[0] = clamp(vColor, vec4(0,0,0,0), vec4(1,1,1,1));       // colors => scalars + lighting
  gl_FragData[1] = vec4(vProjectedVF.x, vProjectedVF.y, 0.0 , depth); // projected vectors
  gl_FragData[2] = vec4(vMaskCriteria, depth);                        // vectors for fragment masking
}
