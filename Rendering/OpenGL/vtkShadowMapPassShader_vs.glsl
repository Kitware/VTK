// ============================================================================
//
//  Program:   Visualization Toolkit
//  Module:    vtkShadowMapPassShader_vs.glsl
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

// Vertex shader used by the shadow mapping render pass.

#version 110

// NOTE: this shader is concatened on the fly by vtkShadowMapPass.cxx by adding
// a line at the beginning like:
// #define VTK_LIGHTING_NUMBER_OF_LIGHTS equal to the number of shadowing
// lights.


void propFuncVS();

// defined in vtkLighting_s.glsl

void lightSeparateSpecularColor(gl_LightSourceParameters lightSource,
                                gl_MaterialParameters m,
                                vec3 surfacePosEyeCoords,
                                vec3 n,
                                bool twoSided,
                                inout vec4 cpri,
                                inout vec4 csec);

void initBlackColors(out vec4 cpri,
                     out vec4 csec);


// input are
// uniform gl_TextureMatrix[VTK_LIGHTING_NUMBER_OF_LIGHTS];

varying vec4 shadowCoord[VTK_LIGHTING_NUMBER_OF_LIGHTS];
varying vec4 frontColors[VTK_LIGHTING_NUMBER_OF_LIGHTS];

void main(void)
{
  vec4 heyeCoords=gl_ModelViewMatrix*gl_Vertex;
  vec3 eyeCoords=heyeCoords.xyz/heyeCoords.w;
  vec3 n=gl_NormalMatrix*gl_Normal;
  n=normalize(n);

  int i=0;
  while(i<VTK_LIGHTING_NUMBER_OF_LIGHTS)
    {
    vec4 cpri;
    vec4 csec;
    initBlackColors(cpri,csec); // because ambient in previous pass.

    lightSeparateSpecularColor(gl_LightSource[i],gl_FrontMaterial,eyeCoords,n,
                               false,cpri,csec);
//    frontColors[i]=vec4(0.5,0.5,0.5,1.0); // cpri+csec;

//    frontColors[i]=gl_FrontMaterial.diffuse*gl_LightSource[i].diffuse;

    frontColors[i]=cpri; //+csec;

    // we could have everything in just gl_TextureMatrix[i] but this would
    // require to add code vtkOpenGLActor. Also the value of the uniform
    // gl_TextureMatrix[i] would have to be changed on each actor.
    // gl_TextureMatrix[i] would be:
    // scale_bias*projection_light[i]*view_light[i]*model
    // and we would have just texCoord=gl_TextureMatrix[i]*gl_Vertex;
    //
    // gl_TextureMatrix[i] is actually:
    // scale_bias*projection_light[i]*view_light[i]*view_camera_inv

    vec4 texCoord=gl_TextureMatrix[i]*heyeCoords;
    shadowCoord[i]=texCoord/texCoord.w;
    ++i;
    }

  // we have to use the fixed-pipeline transform to avoid mismatching with
  // other passes.
  gl_Position=ftransform();

  // propFuncVS(); // opportunity for the prop to execute its vertex shader.



  // we don't initialize gl_FrontColor because we have an array of colors
  // in frontColors[].
}
