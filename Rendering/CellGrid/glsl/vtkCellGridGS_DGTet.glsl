//VTK::System::Dec

// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// Template for the cellgrid mapper geometry shader

// look up cell points, types and vertex positions from texture buffers.
uniform isamplerBuffer cellConnectivity;
uniform isamplerBuffer sideConnectivity;
uniform isamplerBuffer faceConnectivity;
uniform samplerBuffer cellParametrics;
uniform samplerBuffer vertexPositions;

// vtkActor may be given custom uniforms.
//VTK::CustomUniforms::Dec

// camera and actor matrix values
//VTK::Camera::Dec

// Position of vertex in view coordinates.
//VTK::PositionVC::Dec

// Color output per vertex in the rendered primitives.
//VTK::Color::Dec

// The normal of the output primitive in view coordinates.
//VTK::Normal::Dec

layout(points) in;
layout(triangle_strip, max_vertices = 6) out; // TODO: use string substitution for this

// Input from vertex shader
in int vtkCellSideId[]; // size 1

// send cell ID to fragment shader
flat out int vtkCellIdGSOutput;

// Custom output for fragment shader
out vec3 pCoordGSOutput;

//----------------------------------------------------------------
vec3 ComputeNormal(in vec3 p1, in vec3 p2, in vec3 p3)
{
  vec3 delta32 = p3 - p2;
  vec3 delta12 = p1 - p2;
  return cross(delta32, delta12);
}

//----------------------------------------------------------------
/**
 * Draws a triangle for the sideId'th face of a linear tetrahedron.
 * sideId - index of the face which will be rendered as a triangle - [0, 3].
 * cellId - index of the vtk cell whose faces we shall render - [0, numCells[.
 */
void DrawTetFace(in int sideId, in int cellId)
{
  int cellLocalPtIds[3], cellGlobalPtIds[3];
  vec3 coords[3];
  for (int i = 0; i < 3; ++i)
  {
    cellLocalPtIds[i] = texelFetch(faceConnectivity, sideId * 3 + i).r;
    cellGlobalPtIds[i] = texelFetch(cellConnectivity, cellId * 4 + cellLocalPtIds[i]).r;
    coords[i] = texelFetch(vertexPositions, cellGlobalPtIds[i]).xyz;
  }

  vec3 n = ComputeNormal(coords[0], coords[1], coords[2]);
  if (length(n) == 0.0)
  {
    n.z = 1.0f;
  }

  //VTK::Normal::Impl

  for (int ii = 0; ii < 3; ++ii)
  {
    int localPtId = cellLocalPtIds[ii];
    int globalPtId = cellGlobalPtIds[ii];

    vec4 vertexMC = vec4(texelFetch(vertexPositions, globalPtId).xyz, 1.0f);

    //VTK::PositionVC::Impl

    //VTK::Color::Impl

    pCoordGSOutput = texelFetch(cellParametrics, localPtId).xyz;
    EmitVertex();
  }
  EndPrimitive();
}

//----------------------------------------------------------------
void main()
{
  int cellId = texelFetch(sideConnectivity, 2 * vtkCellSideId[0]).r;
  int sideId = texelFetch(sideConnectivity, 2 * vtkCellSideId[0] + 1).r;
  vtkCellIdGSOutput = cellId;
  DrawTetFace(sideId, cellId);
}
