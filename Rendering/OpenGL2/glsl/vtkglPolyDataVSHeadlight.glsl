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

// all variables that represent positions or directions have a suffix
// indicating the coordinate system they are in. The possible values are
// MC - Model Coordinates
// WC - WC world coordinates
// VC - View Coordinates
// DC - Display Coordinates


attribute vec4 vertexMC;
attribute vec3 normalMC;

// material property values
uniform float opacity;
uniform vec3 diffuseColor;
uniform vec3 specularColor;
uniform float specularPower;

// camera and actor matrix values
uniform mat4 modelView;
uniform mat4 projection;
uniform mat3 normalMatrix;

varying vec4 fcolor;

void main()
{
  gl_Position = projection * modelView * vertexMC;
  vec3 normalVC = normalize(normalMatrix * normalMC);

  // diffuse and specular lighting
  float df = max(0.0, dot(normalVC, vec3(0,0,-1)));
  float sf = pow(df, specularPower);

  //vec3 ambient = 0.4 * color;
  vec3 diffuse = df * diffuseColor;
  vec3 specular = sf * specularColor;

  fcolor = vec4(diffuse + specular, opacity);
}

