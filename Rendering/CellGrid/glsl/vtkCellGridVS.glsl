//VTK::System::Dec

/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCellGridVS.glsl

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Template for the cellgrid mappers vertex shader

out int vtkCellSideId;

//----------------------------------------------------------------
void main()
{
  // gl_VertexID is the vtk side id. The geometry shader turns this into a quadrilateral
  vtkCellSideId = gl_VertexID;
  gl_Position = vec4(0,0,0,1);
}
