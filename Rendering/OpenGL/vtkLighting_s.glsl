// ============================================================================
//
//  Program:   Visualization Toolkit
//  Module:    vtkLighting_s.glsl
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

// This file defines some lighting functions.
// They can be used either in a vertex or fragment shader.

#version 110

// Example in vertex shader:
// Reminder: two-sided/one-sided is controlled by GL_VERTEX_PROGRAM_TWO_SIDE
//
// vec4 heyeCoords=gl_ModelViewMatrix*gl_Vertex;
// vec3 eyeCoords=heyeCoords.xyz/heyeCoords.w;
// vec3 n=gl_Normalmatrix*gl_Normal;
// n=normalize(n);
// separateSpecularColor(gl_FrontMaterial,eyeCoords,n,gl_FrontColor,gl_FrontSecondaryColor);
 // If two-sided.
// separateSpecularColor(gl_BackMaterial,eyeCoords,n,gl_BackColor,gl_BackSecondaryColor);

 // Typical:
// gl_FrontColor=singleColor(gl_FrontMaterial,eyeCoords,n);

// VTK_LIGHTING_NUMBER_OF_LIGHTS has to be defined (by shader source string
// concatenation) to be the number of lights on, contiguous from 0 to
// VTK_LIGHTING_NUMBER_OF_LIGHTS-1
// it has to be less than gl_MaxLights (typically 8)

// Per light computation
// (it means the scene ambient term is missing).
// lightSource is usually as gl_LightSource[i]
void lightSeparateSpecularColor(gl_LightSourceParameters lightSource,
                                gl_MaterialParameters m,
                                vec3 surfacePosEyeCoords,
                                vec3 n,
                                bool twoSided,
                                inout vec4 cpri,
                                inout vec4 csec)
{
  vec3 ldir;
  vec3 h;
  float att;
  float spot;
  float shininessFactor;
  vec3 wReverseRayDir=surfacePosEyeCoords;
  
  if(lightSource.position.w!=0.0)
    {
    // ldir=light direction
    vec3 lightPos=lightSource.position.xyz/lightSource.position.w;
    ldir=lightPos-surfacePosEyeCoords;
    float sqrDistance=dot(ldir,ldir);
    ldir=normalize(ldir);
    h=normalize(ldir+normalize(wReverseRayDir));
    att=1.0/(lightSource.constantAttenuation+lightSource.linearAttenuation*sqrt(sqrDistance)+lightSource.quadraticAttenuation*sqrDistance);
    // USED
    }
  else
    {
    att=1.0;
    ldir=lightSource.position.xyz;
    ldir=normalize(ldir);
    h=normalize(ldir+wReverseRayDir);
    }
  
  if(att>0.0)
    {
    // USED
    if(lightSource.spotCutoff==180.0)
      {
      spot=1.0;
      // NOT USED
      }
    else
      {
      // USED
      
      float coef=-dot(ldir,normalize(lightSource.spotDirection));
      if(coef>=lightSource.spotCosCutoff)
        {
        spot=pow(coef,lightSource.spotExponent);
        // USED
        }
      else
        {
        spot=0.0;
        // NOT USED
        }
      }
    if(spot>0.0)
      {
      // USED
     
      // LIT operation...
      float nDotL=dot(n,ldir);
      float nDotH=dot(n,h);
      
      // separate nDotL and nDotH for two-sided shading, otherwise we
      // get black spots.
      
      if(nDotL<0.0) // two-sided shading
        {
//        nDotL=-nDotL; // mostly NOT USED
        nDotL=0.0;
        }
      
      if(nDotH<0.0) // two-sided shading
        {
//        nDotH=-nDotH; // mostly USED, except on the back face of the plane.
        nDotH=0.0;
        }
     
      // ambient term for this light
      vec4 cpril=m.ambient*lightSource.ambient;// acm*adi
      
//      cpri=cpril;
//      return;
      
      // diffuse term for this light
      if(nDotL>0.0)
        {
        // USED
        cpril+=m.diffuse*lightSource.diffuse*nDotL; // dcm*dcli
        }
      
      
      // specular term for this light
      shininessFactor=pow(nDotH,m.shininess); // srm
      
      cpri+=att*spot*cpril;
      
      // scm*scli
      csec+=att*spot*
        m.specular*lightSource.specular*shininessFactor;
      
      }
    }
}

// Ignore Scene ambient. Useful in multipass, if the ambient was already
// taken into account in a previous pass.
void initBlackColors(out vec4 cpri,
                     out vec4 csec)
{
  cpri=vec4(0.0,0.0,0.0,1.0);
  csec=vec4(0.0,0.0,0.0,1.0);
}

void initColorsWithAmbient(gl_MaterialParameters m,
                           out vec4 cpri,
                           out vec4 csec)
{
  cpri=m.emission+m.ambient*gl_LightModel.ambient; // ecm+acm*acs
  csec=vec4(0.0,0.0,0.0,1.0);
}

#ifdef VTK_LIGHTING_NUMBER_OF_LIGHTS

// This is convenience method to use in shader but you better do
// this computation on the CPU and send the result as a uniform.

// True if any enabled light is a positional one.
bool needSurfacePositionInEyeCoordinates()
{
 int i=0;
 bool result=false;
 while(!result && i<VTK_LIGHTING_NUMBER_OF_LIGHTS)
  {
   result=gl_LightSource[i].position.w!=0.0;
   ++i;
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
                           bool twoSided,
                           out vec4 cpri,
                           out vec4 csec)
{
  initColorsWithAmbient(m,cpri,csec);
  
  // For each light,
  int i=0;
  while(i<VTK_LIGHTING_NUMBER_OF_LIGHTS)  
    {
    lightSeparateSpecularColor(gl_LightSource[i],m,surfacePosEyeCoords,n,
                               twoSided,cpri,csec);
    ++i;
    }
}

// Lighting computation based on a material m,
// a position on the surface expressed in eye coordinate (typically a vertex
//  position in a vertex shader, something interpolated in a fragment shader),
// a unit normal to the surface in eye coordinates.
// The result includes the specular component.
vec4 singleColor(gl_MaterialParameters m,
                 vec3 surfacePosEyeCoords,
                 vec3 n,
                 bool twoSided)
{
  vec4 cpri;
  vec4 csec;
  separateSpecularColor(m,surfacePosEyeCoords,n,twoSided,cpri,csec);
  return cpri+csec;
}

#endif
