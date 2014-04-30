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


uniform int numberOfLights; // only allow for up to 6 active lights
uniform vec3 lightColor[6]; // intensity weighted
uniform vec3 lightDirection[6]; // normalized and in camera coords

varying vec4 fcolor;

void main()
{
  gl_Position = projection * modelView * vertex;
  vec3 N = normalize(normalMatrix * normal);
  vec3 E = vec3(0, 0, 1); // eye/view/camera direction

  vec3 diffuse = vec3(0,0,0);
  vec3 specular = vec3(0,0,0);
  for (int lightNum = 0; lightNum < numberOfLights; lightNum++)
    {
    // diffuse and specular lighting
    float df = max(0.0, dot(N, lightDirection[lightNum]));
    diffuse += (df * lightColor[lightNum]);

    float sf = pow( max(0.0, dot(reflect(lightDirection[lightNum], N), E)), 20.0);
    specular += (sf * lightColor[lightNum]);
    }

  diffuse = diffuse * diffuseColor;
  specular = specular * specularColor;
  fcolor = vec4(diffuse + specular, opacity);
}



