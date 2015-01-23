/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkglPolyDataFSPositionalLights.glsl

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// the lighting model for this shader is complex
// and supports the full VTK light API

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
uniform vec3 lightPositionVC[6];
uniform vec3 lightAttenuation[6];
uniform float lightConeAngle[6];
uniform float lightExponent[6];
uniform int lightPositional[6];

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

  vec3 diffuse = vec3(0,0,0);
  vec3 specular = vec3(0,0,0);
  vec3 vertLightDirectionVC;

  for (int lightNum = 0; lightNum < numberOfLights; lightNum++)
    {
    float attenuation = 1.0;

    // directional
    if (lightPositional[lightNum] == 0)
      {
      vertLightDirectionVC = lightDirectionVC[lightNum];
      }
    else
      {
      vertLightDirectionVC = vertexVC.xyz - lightPositionVC[lightNum];
      float distanceVC = length(vertLightDirectionVC);
      vertLightDirectionVC = normalize(vertLightDirectionVC);
      attenuation = 1.0 /
        (lightAttenuation[lightNum].x
         + lightAttenuation[lightNum].y * distanceVC
         + lightAttenuation[lightNum].z * distanceVC * distanceVC);
      // per OpenGL standard cone angle is 90 or less for a spot light
      if (lightConeAngle[lightNum] <= 90.0)
        {
        float coneDot = dot(vertLightDirectionVC, lightDirectionVC[lightNum]);
        // if inside the cone
        if (coneDot >= cos(radians(lightConeAngle[lightNum])))
          {
          attenuation = attenuation * pow(coneDot, lightExponent[lightNum]);
          }
        else
          {
          attenuation = 0.0;
          }
        }
      }

    // diffuse and specular lighting
    float df = max(0.0, attenuation*dot(normalVC, -vertLightDirectionVC));
    diffuse += (df * lightColor[lightNum]);

    if (dot(normalVC, vertLightDirectionVC) < 0.0)
      {
      float sf = attenuation*pow( max(0.0, dot(lightHalfAngleVC[lightNum],normalVC)), specularPower);
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



