/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataSetTriangleFilter.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkDataSetTriangleFilter - triangulate any type of dataset
// .SECTION Description
// vtkTriangulateDataSet generates n-dimensional simplices from any input
// dataset. That is, 3D cells are converted to tetrahedral meshes, 2D cells
// to triangles, and so on. The triangulation is guaranteed compatible as
// long as the dataset is either zero-, one- or two-dimensional; or if
// a three-dimensional dataset, all cells in the 3D dataset are convex 
// with planar facets.
//
// .SECTION See Also
// vtkOrderedTriangulator vtkTriangleFilter

#ifndef __vtkDataSetTriangleFilter_h
#define __vtkDataSetTriangleFilter_h

#include "vtkDataSetToUnstructuredGridFilter.h"

class vtkOrderedTriangulator;

class VTK_GRAPHICS_EXPORT vtkDataSetTriangleFilter : public vtkDataSetToUnstructuredGridFilter
{
public:
  static vtkDataSetTriangleFilter *New();
  vtkTypeRevisionMacro(vtkDataSetTriangleFilter,vtkDataSetToUnstructuredGridFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkDataSetTriangleFilter():Triangulator(NULL) {}
  ~vtkDataSetTriangleFilter();

  // Usual data generation method
  void Execute();

  // different execute methods depending on whether input is structured
  void StructuredExecute();
  void UnstructuredExecute();
  
  vtkOrderedTriangulator *Triangulator;
private:
  vtkDataSetTriangleFilter(const vtkDataSetTriangleFilter&);  // Not implemented.
  void operator=(const vtkDataSetTriangleFilter&);  // Not implemented.
};

#endif


