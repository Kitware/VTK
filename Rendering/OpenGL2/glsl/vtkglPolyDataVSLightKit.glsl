/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// the lighting model for this shader is more complex
// and supports the VTK light kit, multiple lights
// some off axis from the camera, Gouraud shading

// all variables that represent positions or directions have a suffix
// indicating the coordinate system they are in. The possible values are
// MC - Model Coordinates
// WC - WC world coordinates
// VC - View Coordinates
// DC - Display Coordinates

attribute vec4 vertexMC;
attribute vec3 normalMC;

// material property values
//VTK::Color::Dec
uniform vec3 ambientColor; // intensity weighted color
uniform vec3 specularColor; // intensity weighted color
uniform float specularPower;

// camera and actor matrix values
uniform mat4 MCVCMatrix;  // combined Model to View transform
uniform mat4 VCDCMatrix;  // the camera's projection matrix
uniform mat3 normalMatrix; // transform model coordinate directions to view coordinates

uniform int numberOfLights; // only allow for up to 6 active lights
uniform vec3 lightColor[6]; // intensity weighted color
uniform vec3 lightDirectionVC[6]; // normalized

// the resulting vertex color to be passed down to the fragment shader
varying vec4 fcolor;

void main()
{
  // compute the projected vertex position
  vec4 vertexVC = MCVCMatrix * vertexMC;
  gl_Position = VCDCMatrix * vertexVC;

  // now compute the vertex color
  vec3 normalVC = normalMatrix * normalMC;
  vec3 viewDirectionVC = normalize(vec3(0.0, 0.0, 1.0) - vertexVC.xyz);

  vec3 diffuse = vec3(0,0,0);
  vec3 specular = vec3(0,0,0);
  for (int lightNum = 0; lightNum < numberOfLights; lightNum++)
    {
    // diffuse and specular lighting
    float df = max(0.0, dot(normalVC, -lightDirectionVC[lightNum]));
    diffuse += (df * lightColor[lightNum]);

    if (dot(normalVC, -lightDirectionVC[lightNum]) > 0.0)
      {
      float sf = pow( max(0.0, dot(
        reflect(lightDirectionVC[lightNum], normalVC), viewDirectionVC)), specularPower);
      specular += (sf * lightColor[lightNum]);
      }
    }

  diffuse = diffuse * diffuseColor.rgb;
  specular = specular * specularColor;
  fcolor = vec4(ambientColor + diffuse + specular, diffuseColor.a);
}
