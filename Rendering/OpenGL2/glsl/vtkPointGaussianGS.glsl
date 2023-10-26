//VTK::System::Dec

// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// Template for the polydata mappers geometry shader

// primitiveID
//VTK::PrimID::Dec

// optional color passed in from the vertex shader, vertexColor
//VTK::Color::Dec

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

uniform int cameraParallel;
uniform float boundScale;
uniform float scaleFactor;

uniform mat4 VCDCMatrix;

in float radiusVCVSOutput[];
out vec2 offsetVCGSOutput;

// clipping plane vars
//VTK::Clip::Dec

// picking support
//VTK::Picking::Dec

void main()
{
  int i = 0;
  vec2 offset;

  vec4 base1 = vec4(1.0,0.0,0.0,0.0);
  vec4 base2 = vec4(0.0,1.0,0.0,0.0);

  // make the quad face the camera
  if (cameraParallel == 0)
  {
    vec3 dir = normalize(-gl_in[0].gl_Position.xyz);
    base2 = vec4(normalize(cross(dir,vec3(1.0,0.0,0.0))), 0.0);
    base1 = vec4(cross(base2.xyz,dir),0.0);
  }

  //VTK::PrimID::Impl

  //VTK::Clip::Impl

  //VTK::Color::Impl

  //VTK::Picking::Impl

  vec2 offsets[4] = vec2[](vec2(-boundScale, -boundScale),
                           vec2(boundScale, -boundScale),
                           vec2(-boundScale, boundScale),
                           vec2(boundScale, boundScale));

  for (int i = 0; i < 4; i++)
  {
    vec2 offset = scaleFactor * radiusVCVSOutput[0] * offsets[i];

    offsetVCGSOutput = offsets[i];
    gl_Position = VCDCMatrix * (gl_in[0].gl_Position + offset.x*base1 + offset.y*base2);
    EmitVertex();
  }

  EndPrimitive();
}
