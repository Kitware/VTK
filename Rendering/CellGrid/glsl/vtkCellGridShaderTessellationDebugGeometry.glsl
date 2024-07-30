//VTK::System::Dec
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

layout({GSInputPrimitive}) in;
layout({GSOutputPrimitive}, max_vertices = {GSOutputMaxVertices}) out;

flat in int cellIdTESOutput[{GSOutputMaxVertices}];
flat in int instanceIdTESOutput[{GSOutputMaxVertices}];
smooth in vec3 patchDistanceTESOutput[{GSOutputMaxVertices}];
smooth in vec3 normalVCTESOutput[{GSOutputMaxVertices}];
smooth in vec4 vertexVCTESOutput[{GSOutputMaxVertices}];
smooth in vec3 pcoordTESOutput[{GSOutputMaxVertices}];

flat out int cellIdGSOutput;
flat out int instanceIdGSOutput;
smooth out vec3 patchDistanceGSOutput;
smooth out vec3 normalVCGSOutput;
smooth out vec4 vertexVCGSOutput;
smooth out vec3 pcoordGSOutput;
smooth out vec3 primDistanceGSOutput;

void main()
{{
#if {GSOutputMaxVertices} == 3
    cellIdGSOutput = cellIdTESOutput[0];
    instanceIdGSOutput = instanceIdTESOutput[0];
    patchDistanceGSOutput = patchDistanceTESOutput[0];
    normalVCGSOutput = normalVCTESOutput[0];
    vertexVCGSOutput = vertexVCTESOutput[0];
    pcoordGSOutput = pcoordTESOutput[0];
    primDistanceGSOutput = vec3(1, 0, 0);
    gl_Position = gl_in[0].gl_Position;
    EmitVertex();

    cellIdGSOutput = cellIdTESOutput[1];
    instanceIdGSOutput = instanceIdTESOutput[1];
    patchDistanceGSOutput = patchDistanceTESOutput[1];
    normalVCGSOutput = normalVCTESOutput[1];
    vertexVCGSOutput = vertexVCTESOutput[1];
    pcoordGSOutput = pcoordTESOutput[1];
    primDistanceGSOutput = vec3(0, 1, 0);
    gl_Position = gl_in[1].gl_Position;
    EmitVertex();

    cellIdGSOutput = cellIdTESOutput[2];
    instanceIdGSOutput = instanceIdTESOutput[2];
    patchDistanceGSOutput = patchDistanceTESOutput[2];
    normalVCGSOutput = normalVCTESOutput[2];
    vertexVCGSOutput = vertexVCTESOutput[2];
    pcoordGSOutput = pcoordTESOutput[2];
    primDistanceGSOutput = vec3(0, 0, 1);
    gl_Position = gl_in[2].gl_Position;
    EmitVertex();

    EndPrimitive();
#elif {GSOutputMaxVertices} == 2
    cellIdGSOutput = cellIdTESOutput[0];
    instanceIdGSOutput = instanceIdTESOutput[0];
    patchDistanceGSOutput = patchDistanceTESOutput[0];
    normalVCGSOutput = normalVCTESOutput[1];
    vertexVCGSOutput = vertexVCTESOutput[1];
    pcoordGSOutput = pcoordTESOutput[1];
    primDistanceGSOutput = vec3(0, 0, 0);
    gl_Position = gl_in[0].gl_Position;
    EmitVertex();

    cellIdGSOutput = cellIdTESOutput[1];
    instanceIdGSOutput = instanceIdTESOutput[1];
    patchDistanceGSOutput = patchDistanceTESOutput[1];
    normalVCGSOutput = normalVCTESOutput[1];
    vertexVCGSOutput = vertexVCTESOutput[1];
    pcoordGSOutput = pcoordTESOutput[1];
    primDistanceGSOutput = vec3(1, 0, 0);
    gl_Position = gl_in[1].gl_Position;
    EmitVertex();

    EndPrimitive();
#endif
}}
