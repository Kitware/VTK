/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFillHolesFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkFillHolesFilter - identify and fill holes in meshes
// .SECTION Description
// vtkFillHolesFilter is a filter that identifies and fills holes in
// input vtkPolyData meshes. Holes are identified by locating
// boundary edges, linking them together into loops, and then
// triangulating the resulting loops. Note that you can specify
// an approximate limit to the size of the hole that can be filled.
//
// .SECTION Caveats
// Note that any mesh with boundary edges by definition has a
// topological hole. This even includes a reactangular grid
// (e.g., the output of vtkPlaneSource). In such situations, if
// the outer hole is filled, retriangulation of the hole will cause
// geometric overlap of the mesh. This can be prevented by using
// the hole size instance variable to prevent the larger holes
// from being triangulated.
//
// Note this filter only operates on polygons and triangle strips.
// Vertices and polylines are passed through untouched.

#ifndef __vtkFillHolesFilter_h
#define __vtkFillHolesFilter_h

#include "vtkFiltersModelingModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class vtkAbstractTransform;

class VTKFILTERSMODELING_EXPORT vtkFillHolesFilter : public vtkPolyDataAlgorithm
{
public:
  // Description:
  // Standard methods for instantiation, type information and printing.
  static vtkFillHolesFilter *New();
  vtkTypeMacro(vtkFillHolesFilter,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify the maximum hole size to fill. This is represented as a radius
  // to the bounding circumsphere containing the hole.  Note that this is an
  // approximate area; the actual area cannot be computed without first
  // triangulating the hole.
  vtkSetClampMacro(HoleSize, double, 0.0, VTK_LARGE_FLOAT);
  vtkGetMacro(HoleSize, double);

protected:
  vtkFillHolesFilter();
  ~vtkFillHolesFilter();

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  double HoleSize;

private:
  vtkFillHolesFilter(const vtkFillHolesFilter&);  // Not implemented.
  void operator=(const vtkFillHolesFilter&);  // Not implemented.
};

#endif
