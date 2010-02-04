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

// Filename: vtkSurfaceLICPainter_fs1.glsl
// Filename is useful when using gldb-gui

#version 110

#extension GL_ARB_draw_buffers : enable

varying vec2 vProjectedVF;
varying vec4 vColor;

void main()
{
  gl_FragData[0] = clamp(vColor, vec4(0,0, 0, 0), vec4(1, 1, 1, 1));;
  gl_FragData[1] = vec4(vProjectedVF.x, vProjectedVF.y, gl_FragCoord.z, 1.0);
}
