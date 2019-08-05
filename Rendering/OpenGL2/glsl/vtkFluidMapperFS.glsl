//VTK::System::Dec

/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// uniform int PrimitiveIDOffset;

//VTK::CustomUniforms::Dec

// VC position of this fragment
in vec4 vertexVCVSOutput;

// vertex color
uniform int hasVertexColor;
in vec3 colorVCGSOutput;

// Camera prop
uniform mat4 VCDCMatrix;
uniform mat4 MCVCMatrix;
uniform int  cameraParallel;

uniform int outputEyeZ = 1;

// optional color passed in from the vertex shader, vertexColor
//VTK::Color::Dec

// optional surface normal declaration
uniform float particleRadius;
in vec3 centerVCVSOutput;

// Texture maps
uniform sampler2D opaqueZTexture;

// picking support
//VTK::Picking::Dec

// Depth Peeling Support
//VTK::DepthPeeling::Dec

// clipping plane vars
//VTK::Clip::Dec

// the output of this shader
//VTK::Output::Dec

void main()
{
  // VC position of this fragment. This should not branch/return/discard.
  vec4  vertexVC   = vertexVCVSOutput;
  float originalVZ = vertexVC.z;

  // Place any calls that require uniform flow (e.g. dFdx) here.
  //VTK::UniformFlow::Impl

  // Set gl_FragDepth here (gl_FragCoord.z by default)
  // compute the eye position and unit direction
  vec3 EyePos;
  vec3 EyeDir;
  if(cameraParallel != 0)
  {
    EyePos = vec3(vertexVC.x, vertexVC.y, vertexVC.z + particleRadius);
    EyeDir = vec3(0.0, 0.0, -1.0);
  }
  else
  {
    EyeDir = vertexVC.xyz;
    EyePos = vec3(0.0, 0.0, 0.0);
    float lengthED = length(EyeDir);
    EyeDir = normalize(EyeDir);
      // we adjust the EyePos to be closer if it is too far away
      // to prevent floating point precision noise
      if(lengthED > particleRadius)
      {
        EyePos = vertexVC.xyz - EyeDir * particleRadius;
      }
    }

    // translate to Sphere center
    EyePos = EyePos - centerVCVSOutput;
    // scale to particleRadius 1.0
    EyePos = EyePos / particleRadius;
    // find the intersection
    float b = 2.0 * dot(EyePos, EyeDir);
    float c = dot(EyePos, EyePos) - 1.0;
    float d = b * b - 4.0 * c;
    vec3  normalVCVSOutput = vec3(0.0, 0.0, 1.0);
    if(d < 0.0)
    {
      discard;
    }
    float t = (-b - sqrt(d)) * 0.5;

    // compute the normal, for unit sphere this is just
    // the intersection point
    normalVCVSOutput = normalize(EyePos + t * EyeDir);
    // compute the intersection point in VC
    vertexVC.xyz = normalVCVSOutput * particleRadius + centerVCVSOutput;
    // compute the pixel's depth
    // " normalVCVSOutput = vec3(0,0,1);
    vec4 pos = VCDCMatrix * vertexVC;
    gl_FragDepth = (pos.z / pos.w + 1.0) / 2.0;

    //VTK::Clip::Impl

    //VTK::Color::Impl

    float odepth = texelFetch(opaqueZTexture, ivec2(int(gl_FragCoord.x + 0.5), int(gl_FragCoord.y + 0.5)), 0).r;
    if(gl_FragDepth >= odepth)
    {
      discard;
    }

    // If output eye coordinate depth
    if(outputEyeZ != 0)
    {
      gl_FragData[0] = vec4(vertexVC.z, 0, 0, 1.0);
    }
    else
    // thickness and vertex color (if applicable)
    {
      gl_FragData[0] = vec4(vertexVC.z - originalVZ, 0, 0, 1.0);
      if(hasVertexColor == 1)
      {
        gl_FragData[1] = vec4(colorVCGSOutput, 1.0);
      }
    }
}
