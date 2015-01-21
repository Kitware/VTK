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

// camera and actor matrix values
//VTK::Camera::Dec

uniform int numberOfLights; // only allow for up to 6 active lights
uniform vec3 lightColor[6]; // intensity weighted color
uniform vec3 lightDirectionVC[6]; // normalized
uniform vec3 lightHalfAngleVC[6]; // normalized

// VC positon of this fragment
//VTK::PositionVC::Dec

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

// clipping plane vars
//VTK::Clip::Dec

void main()
{
  //VTK::Clip::Impl

  //VTK::Color::Impl
  // Note that the above will always define vec3 ambientColor, vec3 diffuseColor and float opacity

  // Generate the normal if we are not passed in one
  //VTK::Normal::Impl

  // now compute the vertex color
  vec3 diffuse = vec3(0,0,0);
  vec3 specular = vec3(0,0,0);
  for (int lightNum = 0; lightNum < numberOfLights; lightNum++)
    {
    // diffuse and specular lighting
    float df = max(0.0, dot(normalVC, -lightDirectionVC[lightNum]));
    diffuse += (df * lightColor[lightNum]);

    if (dot(normalVC, lightDirectionVC[lightNum]) < 0.0)
      {
      float sf = pow( max(0.0, dot(lightHalfAngleVC[lightNum],normalVC)), specularPower);
      specular += (sf * lightColor[lightNum]);
      }
    }

  diffuse = diffuse * diffuseColor;
  specular = specular * specularColor;

  gl_FragColor = vec4(ambientColor + diffuse + specular, opacity);
  //VTK::TCoord::Impl

  if (gl_FragColor.a <= 0.0)
    {
    discard;
    }

  //VTK::DepthPeeling::Impl

  //VTK::Picking::Impl
}



