/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// the lighting model for this shader is the LightKit

// all variables that represent positions or directions have a suffix
// indicating the coordinate system they are in. The possible values are
// MC - Model Coordinates
// WC - WC world coordinates
// VC - View Coordinates
// DC - Display Coordinates

// material property values
uniform float opacity;
uniform vec3 ambientColor; // intensity weighted color
uniform vec3 specularColor; // intensity weighted color
uniform float specularPower;

// passed from the vertex shader
varying vec4 vertexVC;
varying vec4 vertexWC;
varying vec3 vertexColor;

// optional normal declaration
//VTK::Normal::Dec

void main()
{
  // Generate the normal if we are not passed in one
  //VTK::Normal::Impl

  // diffuse and specular lighting
  float df = max(0,dot(normalVC, vec3(0, 0, 1)));
  float sf = pow(df, specularPower);

  vec3 diffuse = df * vertexColor;
  vec3 specular = sf * specularColor;

  gl_FragColor = vec4(ambientColor + diffuse + specular, opacity);
}



