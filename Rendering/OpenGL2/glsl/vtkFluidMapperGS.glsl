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

// primitiveID
//VTK::PrimID::Dec

// optional color passed in from the vertex shader, vertexColor
//VTK::Color::Dec

layout(points) in;
layout(triangle_strip, max_vertices = 3) out;

uniform int   cameraParallel;
uniform float particleRadius;
uniform mat4  VCDCMatrix;

out vec4 vertexVCGSOutput;
out vec3 centerVCGSOutput;

in vec3 colorVSOut[];
out vec3 colorVCGSOutput;

// clipping plane vars
//VTK::Clip::Dec

// picking support
//VTK::Picking::Dec

void main() {
    int  i = 0;
    vec4 offset;

    vec4 base1 = vec4(1.0, 0.0, 0.0, 0.0);
    vec4 base2 = vec4(0.0, 1.0, 0.0, 0.0);

    // make the triangle face the camera
    if(cameraParallel == 0) {
        vec3 dir = normalize(-gl_in[0].gl_Position.xyz);
        base2 = vec4(normalize(cross(dir, vec3(1.0, 0.0, 0.0))), 0.0);
        base1 = vec4(cross(base2.xyz, dir), 0.0);
    }

    //VTK::PrimID::Impl

    //VTK::Clip::Impl

    //VTK::Color::Impl

    centerVCGSOutput = gl_in[0].gl_Position.xyz / gl_in[0].gl_Position.w;

    colorVCGSOutput = colorVSOut[0];

    //VTK::Picking::Impl

    // note 1.73205 = 2.0*cos(30)

    offset           = vec4(-1.73205 * particleRadius, -particleRadius, 0.0, 0.0);
    vertexVCGSOutput = gl_in[0].gl_Position + offset.x * base1 + offset.y * base2;
    gl_Position      = VCDCMatrix * vertexVCGSOutput;
    EmitVertex();

    offset           = vec4(1.73205 * particleRadius, -particleRadius, 0.0, 0.0);
    vertexVCGSOutput = gl_in[0].gl_Position + offset.x * base1 + offset.y * base2;
    gl_Position      = VCDCMatrix * vertexVCGSOutput;
    EmitVertex();

    offset           = vec4(0.0, 2.0 * particleRadius, 0.0, 0.0);
    vertexVCGSOutput = gl_in[0].gl_Position + offset.x * base1 + offset.y * base2;
    gl_Position      = VCDCMatrix * vertexVCGSOutput;
    EmitVertex();

    EndPrimitive();
}
