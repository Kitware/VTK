//=========================================================================
//
//  Program:   Visualization Toolkit
//  Module:    vtkLightingHelper_s.glsl
//
//  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
//  All rights reserved.
//  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.
//
//     This software is distributed WITHOUT ANY WARRANTY; without even
//     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//     PURPOSE.  See the above copyright notice for more information.
//
//=========================================================================

#version 110

// Filename: vtkLightingHelper_s.glsl
// Filename is useful when using gldb-gui

// This file defines some lighting functions.
// They can be used either in a vertex or fragment shader.

// It is intended to be used in conjunction with vtkLightsSwitches class.

// Those functions expect uniform variables about the status of lights
// 1. In fixed-mode pipeline (glUseProgram(0)),
// 2. get the values with GLboolean lightSwitch[i]=glIsEnabled(GL_LIGHTi);
// 3. Switch to programmable pipeline (glUseProgram(prog))
// 4. Send boolean as uniform: var=glGetUniformLocation(prog,"lightSwitch[i]");
// 5. glUniform1i(var,lightSwitch[i]);

// vtkLightsSwitches class can do that for you.


// Example in vertex shader:
// Reminder: two-sided/one-sided is controlled by GL_VERTEX_PROGRAM_TWO_SIDE
//
// vec4 eyeCoords=gl_ModelViewMatrix*gl_Vertex;
// vec4 n=gl_Normalmatrix*gl_Normal;
// n=normalize(n);
// separateSpecularColor(gl_FrontMaterial,eyeCoords,n,gl_FrontColor,gl_FrontSecondaryColor);
 // If two-sided.
// separateSpecularColor(gl_BackMaterial,eyeCoords,n,gl_BackColor,gl_BackSecondaryColor);

 // Typical:
// gl_FrontColor=singleColor(gl_FrontMaterial,eyeCoords,n);

// This is convenience method to use in shader but you better do
// this computation on the CPU and send the result as a uniform.

// True if any enabled light is a positional one.
bool needSurfacePositionInEyeCoordinates()
{
  bool result=false;
  for (int i=0; !result && (i < gl_MaxLights); i++)
    {
    result = (gl_LightSource[i].diffuse.w != 0.0) &&
      (gl_LightSource[i].position.w != 0.0);
    }

  return result;
}

// Lighting computation based on a material m,
// a position on the surface expressed in eye coordinate (typically a vertex
//  position in a vertex shader, something interpolated in a fragment shader),
// a unit normal `n' to the surface in eye coordinates.
// Most of the components are in cpri (primary color), the specular
// component is in csec (secondary color).
// Useful for blending color and textures.
void separateSpecularColor(gl_MaterialParameters m,
                           vec3 surfacePosEyeCoords,
                           vec3 n,
                           out vec4 cpri,
                           out vec4 csec)
{
  cpri = m.emission + m.ambient * gl_LightModel.ambient; // ecm+acm*acs
  csec = vec4(0.0,0.0,0.0,1.0);
  vec3 wReverseRayDir = surfacePosEyeCoords;

  // For each light,
  for (int i=0; i < gl_MaxLights; i++)
    {
    // Trick.
    bool lightEnabled = (gl_LightSource[i].diffuse.w != 0.0);
    if (!lightEnabled)
      {
      continue;
      }

    vec3 ldir;
    vec3 h;
    float att;
    float spot;
    float shininessFactor;

    if (gl_LightSource[i].position.w != 0.0)
      {
      // ldir=light direction
      vec3 lightPos=gl_LightSource[i].position.xyz/
        gl_LightSource[i].position.w;
      ldir = lightPos - surfacePosEyeCoords;
      float sqrDistance = dot(ldir,ldir);
      ldir = normalize(ldir);
      h = normalize(ldir + wReverseRayDir);
      att = 1.0 / (gl_LightSource[i].constantAttenuation + gl_LightSource[i].linearAttenuation *
        sqrt(sqrDistance) + gl_LightSource[i].quadraticAttenuation * sqrDistance);
      }
    else
      {
      att = 1.0;
      ldir = gl_LightSource[i].position.xyz;
      ldir = normalize(ldir);
      h = normalize(ldir + wReverseRayDir);
      }

    if (att>0.0)
      {
      if (gl_LightSource[i].spotCutoff == 180.0)
        {
        spot = 1.0;
        }
      else
        {
        float coef=-dot(ldir,gl_LightSource[i].spotDirection);
        if (coef>=gl_LightSource[i].spotCosCutoff)
          {
          spot=pow(coef,gl_LightSource[i].spotExponent);
          }
        else
          {
          spot=0.0;
          }
        }
      if (spot>0.0)
        {
        // LIT operation...
        float nDotL=dot(n,ldir);
        float nDotH=dot(n,h);

        // separate nDotL and nDotH for two-sided shading, otherwise we
        // get black spots.

        if (nDotL<0.0) // two-sided shading
          {
          nDotL=-nDotL;
          }

        if (nDotH<0.0) // two-sided shading
          {
          nDotH=-nDotH;
          }
        // ambient term for this light
        vec4 cpril=m.ambient*gl_LightSource[i].ambient;// acm*adi

        // diffuse term for this light
        if (nDotL>0.0)
          {
          cpril+=m.diffuse*gl_LightSource[i].diffuse*nDotL; // dcm*dcli
          }

        // specular term for this light
        shininessFactor=pow(nDotH,m.shininess); // srm

        cpri+=att*spot*cpril;

        // scm*scli
        csec+=att*spot*
          m.specular*gl_LightSource[i].specular*shininessFactor;

        }
      }
    }
}


// Lighting computation based on a material m,
// a position on the surface expressed in eye coordinate (typically a vertex
//  position in a vertex shader, something interpolated in a fragment shader),
// a unit normal to the surface in eye coordinates.
// The result includes the specular component.
vec4 singleColor(gl_MaterialParameters m,
                 vec3 surfacePosEyeCoords,
                 vec3 n)
{
 vec4 cpri;
 vec4 csec;
 separateSpecularColor(m,surfacePosEyeCoords,n,cpri,csec);
 return cpri+csec;
}
