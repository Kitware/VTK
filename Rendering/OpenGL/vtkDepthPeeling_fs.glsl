// ============================================================================
//
//  Program:   Visualization Toolkit
//  Module:    vtkDepthPeeling_fs.glsl
//
//  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
//  All rights reserved.
//  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.
//
//     This software is distributed WITHOUT ANY WARRANTY; without even
//     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//     PURPOSE.  See the above copyright notice for more information.
//
// ============================================================================

// 
// Fragment shader used by the depth peeling algorithm.

#version 110

#extension GL_ARB_texture_rectangle: enable

uniform sampler2DRectShadow shadowTex;
uniform sampler2DRectShadow opaqueShadowTex;
uniform float offsetX;
uniform float offsetY;

void propFuncFS();

void main()
{
  vec4 r0=gl_FragCoord;
  r0.x=r0.x-offsetX;
  r0.y=r0.y-offsetY;
  float r1=shadow2DRect(opaqueShadowTex,r0.xyz).x;
  r1=r1-0.5;
  if(r1<0.0)
    {
    discard;
    }
  r0.x=shadow2DRect(shadowTex,r0.xyz).x;
  r0.x=r0.x-0.5;
  if(r0.x<0.0)
    {
    discard;
    }
  propFuncFS();
}
