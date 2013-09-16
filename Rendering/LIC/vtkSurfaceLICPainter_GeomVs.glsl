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

#version 120

varying vec4 vColor;
varying vec2 vProjectedVF;
varying vec3 vMaskCriteria;

// 0/1, when 1 V is projected to surface for |V| computation.
uniform float uMaskOnSurface;


// from vtkColorMaterialHelper
gl_MaterialParameters getMaterialParameters();

// from vtkLightingHelper
vec4 singleColor(gl_MaterialParameters m, vec3 surfacePosEyeCoords, vec3 n);

/**
Compute vertext color
*/
vec4 colorFrontFace()
{
  vec4 heyeCoords = gl_ModelViewMatrix * gl_Vertex;
  vec3 eyeCoords = heyeCoords.xyz / heyeCoords.w;
  vec3 n = normalize(gl_NormalMatrix * gl_Normal);
  return singleColor(getMaterialParameters(), eyeCoords,n);
}

/**
Project "vector" onto the surface.
*/
vec3 projectOnSurface(vec3 vector)
{
  vec3 normal = normalize(gl_Normal);
  float k = dot(normal, vector);
  return (vector - (k*normal));
}


/**
get fragment mask criteria. Fragment masking should be applied according to
the original vector field and in those units. If it is not then masked fragments
will not match pseudo color plots.
*/
vec3 getMaskCriteria( vec3 vector )
{
  if (uMaskOnSurface == 0)
    {
    return vector;
    }
  else
    {
    return projectOnSurface(vector);
    }
}

void main()
{
  vec3 vf = projectOnSurface(gl_MultiTexCoord0.stp);
  vProjectedVF = (gl_NormalMatrix * vf).xy;
  vMaskCriteria = getMaskCriteria(gl_MultiTexCoord0.stp);
  vColor = colorFrontFace();
  gl_Position = ftransform();
}
