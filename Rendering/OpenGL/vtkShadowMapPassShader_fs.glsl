// ============================================================================
//
//  Program:   Visualization Toolkit
//  Module:    vtkShadowMapPassShader_fs.glsl
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

// Fragment shader used by the shadow mapping render pass.

#version 110

// NOTE: this shader is concatened on the fly by vtkShadowMapPass.cxx by adding
// a line at the beginning like:
// #define VTK_LIGHTING_NUMBER_OF_LIGHTS equal to the number of shadowing
// lights.

uniform sampler2DShadow shadowMaps[VTK_LIGHTING_NUMBER_OF_LIGHTS];
uniform sampler2D spotLightShape;

varying vec4 shadowCoord[VTK_LIGHTING_NUMBER_OF_LIGHTS];
varying vec4 frontColors[VTK_LIGHTING_NUMBER_OF_LIGHTS];

void main(void)
{
  gl_FragColor=vec4(0.0,0.0,0.0,0.0);
  int i=0;
  while(i<VTK_LIGHTING_NUMBER_OF_LIGHTS)
    {
    float factor=0.0;
    if(shadowCoord[i].w>0.0)
      {
      vec2 projected=shadowCoord[i].xy/shadowCoord[i].w;
      if(projected.x>=0.0 && projected.x<=1.0
        && projected.y>=0.0 && projected.y<=1.0)
        {
        factor=shadow2DProj(shadowMaps[i],shadowCoord[i]).x;
        }
      }
    vec4 colorFactor=texture2DProj(spotLightShape,shadowCoord[i])*factor;
    gl_FragColor+=frontColors[i]*colorFactor.x;
//    gl_FragColor+=frontColors[i]*factor;
    ++i;
    }

  gl_FragColor=clamp(gl_FragColor,0.0,1.0);

  // we don't let the prop to execute its fragment shader because it
  // already executed in the previous pass with none shadowing lights.
  //  propFuncFS();

  // gl_FragColor will be blending with framebuffer value containing other
  // lights contributions.
  // use alpha_testing for black/dark color?
}
