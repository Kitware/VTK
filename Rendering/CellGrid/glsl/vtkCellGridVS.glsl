//VTK::System::Dec

// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// Template for the cellgrid mappers vertex shader

out int vtkCellSideId;

//----------------------------------------------------------------
void main()
{
  // gl_VertexID is the vtk side id. The geometry shader turns this into a quadrilateral
  vtkCellSideId = gl_VertexID;
  gl_Position = vec4(0,0,0,1);
}
