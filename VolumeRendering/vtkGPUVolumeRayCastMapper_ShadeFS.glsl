/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGPUVolumeRayCastMapper_ShadeFS.glsl

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Fragment shader that implements initShade() and shade() in the case of
// shading.
// The functions are used in composite mode.

#version 110

// "value" is a sample of the dataset.
// Think of "value" as an object.

// from 1- vs 4-component shader.
vec4 colorFromValue(vec4 value);

uniform sampler3D dataSetTexture; // need neighbors for gradient

// Change-of-coordinate matrix from eye space to texture space
uniform mat3 eyeToTexture3;
uniform mat4 eyeToTexture4;

// Tranpose of Change-of-coordinate matrix from texture space to eye space
uniform mat3 transposeTextureToEye;

// Used to compute the gradient.
uniform vec3 cellStep;
uniform vec3 cellScale;


// Entry position (global scope), updated in the loop
vec3 pos;
// Incremental vector in texture space (global scope)
vec3 rayDir;


// local to the implementation, shared between initShade() and shade()
const vec3 minusOne=vec3(-1.0,-1.0,-1.0);
const vec4 clampMin=vec4(0.0,0.0,0.0,0.0);
const vec4 clampMax=vec4(1.0,1.0,1.0,1.0);

vec3 xvec;
vec3 yvec;
vec3 zvec;
vec3 wReverseRayDir;
vec3 lightPos;
vec3 ldir;
vec3 h;
vec4 hPos; // homogeneous position

// ----------------------------------------------------------------------------
void initShade()
{
  xvec=vec3(cellStep.x,0.0,0.0); // 0.01
  yvec=vec3(0.0,cellStep.y,0.0);
  zvec=vec3(0.0,0.0,cellStep.z);
  
  // Reverse ray direction in eye space
  wReverseRayDir=eyeToTexture3*rayDir;
  wReverseRayDir=wReverseRayDir*minusOne;
  wReverseRayDir=normalize(wReverseRayDir);
  
  // Directonal light: w==0
  if(gl_LightSource[0].position.w==0.0)
    {
    ldir=gl_LightSource[0].position.xyz;
    ldir=normalize(ldir);
    h=normalize(ldir+wReverseRayDir);
    }
  else
    {
    lightPos=gl_LightSource[0].position.xyz/gl_LightSource[0].position.w;
    hPos.w=1.0; // used later
    }
}

// ----------------------------------------------------------------------------
vec4 shade(vec4 value)
{
  vec3 g1;
  vec3 g2;
  vec4 tmp;
  float att;
  float spot;
  
  g1.x=texture3D(dataSetTexture,pos+xvec).x;
  g1.y=texture3D(dataSetTexture,pos+yvec).x;
  g1.z=texture3D(dataSetTexture,pos+zvec).x;
  g2.x=texture3D(dataSetTexture,pos-xvec).x;
  g2.y=texture3D(dataSetTexture,pos-yvec).x;
  g2.z=texture3D(dataSetTexture,pos-zvec).x;
  // g1-g2 is  the gradient in texture coordinates
  // the result is the normalized gradient in eye coordinates.
  
  g2=g1-g2;
  g2=g2*cellScale;
  
  float normalLength=length(g2);
  if(normalLength>0.0)
    {
    g2=normalize(transposeTextureToEye*g2);
    }
  else
    {
    g2=vec3(0.0,0.0,0.0);
    }
  
  vec4 color=colorFromValue(value);
  
  // initialize color to 0.0
  vec4 finalColor=vec4(0.0,0.0,0.0,0.0);        
  
  if(gl_LightSource[0].position.w!=0.0)
    {
    // We need to know the eye position only if light is positional
    // ldir= vertex position in eye coordinates
    hPos.xyz=pos;
    tmp=eyeToTexture4*hPos;
    ldir=tmp.xyz/tmp.w;
    // ldir=light direction
    ldir=lightPos-ldir;
    float sqrDistance=dot(ldir,ldir);
    ldir=normalize(ldir);
    h=normalize(ldir+wReverseRayDir);
    att=1.0/(gl_LightSource[0].constantAttenuation+gl_LightSource[0].linearAttenuation*sqrt(sqrDistance)+gl_LightSource[0].quadraticAttenuation*sqrDistance);
    }
  else
    {
    att=1.0;
    }
  
  if(att>0.0)
    {
    if(gl_LightSource[0].spotCutoff==180.0)
      {
      spot=1.0;
      }
    else
      {
      float coef=-dot(ldir,gl_LightSource[0].spotDirection);
      if(coef>=gl_LightSource[0].spotCosCutoff)
        {
        spot=pow(coef,gl_LightSource[0].spotExponent);
        }
      else
        {
        spot=0.0;
        }
      }
    if(spot>0.0)
      {
      // LIT operation...
      float nDotL=dot(g2,ldir);
      float nDotH=dot(g2,h);
      
      // separate nDotL and nDotH for two-sided shading, otherwise we
      // get black spots.
      
      if(nDotL<0.0) // two-sided shading
        {
        nDotL=-nDotL;
        }
      
      if(nDotH<0.0) // two-sided shading
        {
        nDotH=-nDotH;
        }
      // ambient term for this light
      finalColor+=gl_FrontLightProduct[0].ambient;
      
      // diffuse term for this light
      if(nDotL>0.0)
        {
        finalColor+=(gl_FrontLightProduct[0].diffuse*nDotL)*color;
        }
      
      // specular term for this light
      float shininessFactor=pow(nDotH,gl_FrontMaterial.shininess);
      finalColor+=gl_FrontLightProduct[0].specular*shininessFactor;
      finalColor*=att*spot;
      }
    }
  
  // scene ambient term
  finalColor+=gl_FrontLightModelProduct.sceneColor*color;
  
  // clamp. otherwise we get black spots
  finalColor=clamp(finalColor,clampMin,clampMax);
  
  return finalColor;
}
