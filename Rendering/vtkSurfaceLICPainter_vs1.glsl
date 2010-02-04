//=========================================================================
//
//  Program:   Visualization Toolkit
//  Module:    vtkSurfaceLICPainter_vs1.glsl
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
// Filename: vtkSurfaceLICPainter_vs1.glsl
// Filename is useful when using gldb-gui

#version 120 
varying vec4 vColor;
varying vec2 vProjectedVF;

// from vtkColorMaterialHelper
gl_MaterialParameters getMaterialParameters();

// from vtkLightingHelper
vec4 singleColor(gl_MaterialParameters m,
  vec3 surfacePosEyeCoords, vec3 n);

// Projects "vector" onto the surface.
vec3 projectOnSurface(vec3 vector)
{
  vec3 normal = normalize(gl_Normal);
  float k = dot(normal, vector);
  return (vector - (k*normal));
}

vec4 colorFrontFace()
{
 vec4 heyeCoords = gl_ModelViewMatrix*gl_Vertex;
 vec3 eyeCoords = heyeCoords.xyz/heyeCoords.w;
 vec3 n = normalize(gl_NormalMatrix*gl_Normal);
 return singleColor(getMaterialParameters(),eyeCoords,n);
}

void main()
{
  vec3 vf = projectOnSurface(gl_MultiTexCoord0.stp);
  vProjectedVF = (gl_NormalMatrix * vf).xy;
  vColor = colorFrontFace();
  gl_Position = ftransform();
}
