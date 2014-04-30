/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// the lighting model for this shader is very simple
// it requires only one white light, positioned at the camera
// Gouraud shading

attribute vec4 vertex;
attribute vec3 normal;

// material property values
uniform float opacity;
uniform vec3 diffuseColor;
uniform vec3 specularColor;
uniform vec3 specularPower;

// camera and actor matrix values
uniform mat4 modelView;
uniform mat4 projection;
uniform mat3 normalMatrix;

varying vec4 fcolor;

void main()
{
  gl_Position = projection * modelView * vertex;
  vec3 N = normalize(normalMatrix * normal);

  // diffuse and specular lighting
  float df = max(0.0, dot(N, vec3(0,0,-1)));
  float sf = pow(df, 20.0);

  //vec3 ambient = 0.4 * color;
  vec3 diffuse = df * diffuseColor;
  vec3 specular = sf * specularColor;

  fcolor = vec4(diffuse + specular, opacity);
}

