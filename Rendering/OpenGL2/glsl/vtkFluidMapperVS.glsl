//VTK::System::Dec

// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// this shader implements fluid imposters in OpenGL as Spheres

in vec4 vertexMC;
in vec3 vertexColor;

uniform int   hasVertexColor   = 0;

// optional normal declaration
//VTK::Normal::Dec

// Texture coordinates
//VTK::TCoord::Dec

// material property values
//VTK::Color::Dec

// clipping plane vars
//VTK::Clip::Dec

// camera and actor matrix values
uniform mat4 MCVCMatrix;

// picking support
//VTK::Picking::Dec

// Pass vertex color to fragment shader
out vec3 colorVSOut;

void main() {
    //VTK::Color::Impl

    //VTK::Normal::Impl

    //VTK::TCoord::Impl

    //VTK::Clip::Impl

    gl_Position = MCVCMatrix * vertexMC;

    if(hasVertexColor == 1) {
        colorVSOut = vertexColor;
    }

    //VTK::Picking::Impl
}
