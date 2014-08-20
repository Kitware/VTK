/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkglPolyDataFSHeadight.glsl

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// the lighting model for this shader is the LightKit

// The following line handle system declarations such a
// default precisions, or defining precisions to null
//VTK::System::Dec

// all variables that represent positions or directions have a suffix
// indicating the coordinate system they are in. The possible values are
// MC - Model Coordinates
// WC - WC world coordinates
// VC - View Coordinates
// DC - Display Coordinates

// material property values
uniform float opacityUniform; // the fragment opacity
uniform vec3 ambientColorUniform; // intensity weighted color
uniform vec3 diffuseColorUniform; // intensity weighted color
uniform vec3 specularColor; // intensity weighted color
uniform float specularPower;

// passed from the vertex shader
varying vec4 vertexVC;

// optional color passed in from the vertex shader, vertexColor
//VTK::Color::Dec

// optional normal declaration
//VTK::Normal::Dec

// Texture coordinates
//VTK::TCoord::Dec

// picking support
//VTK::Picking::Dec

// Depth Peeling Support
//VTK::DepthPeeling::Dec

void main()
{
  //VTK::Color::Impl
  // Note that the above will always define vec3 ambientColor, vec3 diffuseColor and float opacity

  // Generate the normal if we are not passed in one
  //VTK::Normal::Impl

  // diffuse and specular lighting
  float df = max(0.0,dot(normalVC, vec3(0.0, 0.0, 1.0)));
  float sf = pow(df, specularPower);

  vec3 diffuse = df * diffuseColor;
  vec3 specular = sf * specularColor;

  gl_FragColor = vec4(ambientColor + diffuse + specular, opacity);
  //VTK::TCoord::Impl

  if (gl_FragColor.a <= 0.0)
    {
    discard;
    }

  //VTK::DepthPeeling::Impl

  //VTK::Picking::Impl

}



