/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCellType.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkCellType - define types of cells
// .SECTION Description
// vtkCellType defines the allowable cell types in the visualization 
// library (vtk). In vtk, datasets consist of collections of cells. 
// Different datasets consist of different cell types. The cells may be 
// explicitly represented (as in vtkPolyData), or may be implicit to the
// data type (as in vtkStructuredPoints).

#ifndef __vtkCellType_h
#define __vtkCellType_h

// To add a new cell type, define a new integer type flag here, then
// create a subclass of vtkCell to implement the proper behavior. You 
// may have to modify the following methods: vtkDataSet (and subclasses) 
// GetCell() and vtkGenericCell::SetCellType(). Also, to do the job right,
// you'll also have to modify the readers/writers and regression tests
// (example scripts) to reflect the new cell addition.

#define VTK_EMPTY_CELL 0
#define VTK_VERTEX 1
#define VTK_POLY_VERTEX 2
#define VTK_LINE 3
#define VTK_POLY_LINE 4
#define VTK_TRIANGLE 5
#define VTK_TRIANGLE_STRIP 6
#define VTK_POLYGON 7
#define VTK_PIXEL 8
#define VTK_QUAD 9
#define VTK_TETRA 10
#define VTK_VOXEL 11
#define VTK_HEXAHEDRON 12
#define VTK_WEDGE 13
#define VTK_PYRAMID 14

#define VTK_PARAMETRIC_CURVE 51
#define VTK_PARAMETRIC_SURFACE 52

#endif


