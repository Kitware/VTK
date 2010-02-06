/*=========================================================================

Program:   Visualization Toolkit
Module:    $RCSfile: vtkMeanValueCoordinatesInterpolator.h,v $

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMeanValueCoordinatesInterpolator - compute interpolation computes for closed triangular mesh
// .SECTION Description
// vtkMeanValueCoordinatesInterpolator computes interpolation weights for a
// closed, manifold triangular mesh.  Once computed, the interpolation
// weights can be used to interpolate data anywhere interior or exterior to
// the mesh. This work is documented in the Siggraph paper by Tao Ju, Scot
// Schaefer and Joe Warren from Rice University "Mean Value Coordinates for
// Closed Triangular Meshes".
//
// In VTK this class was initially created to interpolate data across
// polyhedral cells. In addition, the class can be used to interpolate
// data values from a triangle mesh, and to smoothly deform a mesh from 
// an associated control mesh.

// .SECTION See Also
// vtkPolyhedralCell

#ifndef __vtkMeanValueCoordinatesInterpolator_h
#define __vtkMeanValueCoordinatesInterpolator_h

#include "vtkObject.h"

class vtkPoints;
class vtkIdList;
class vtkDataArray;
class vtkIdTypeArray;

//Special internal class for iterating over data
class vtkTriIterator;


class VTK_COMMON_EXPORT vtkMeanValueCoordinatesInterpolator : public vtkObject
{
public:
  // Description
  // Standard instantiable class methods.
  static vtkMeanValueCoordinatesInterpolator *New();
  vtkTypeRevisionMacro(vtkMeanValueCoordinatesInterpolator,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Method to generate interpolation weights from a list of triangles.  In
  // this version of the method, the triangles are defined by a vtkPoints
  // array plus a vtkIdList, where the vtkIdList is organized such that three
  // ids in order define a triangle.  Note that the data type (float,double)
  // of the points will determine the type of the weights (this function only
  // allows float and double point types).
  static void ComputeInterpolationWeights(vtkPoints *pts, vtkIdList *tris,
                                          vtkDataArray *weights);
  
  // Description:
  // Method to generate interpolation weights from a list of triangles.  In
  // this version of the method, the triangles are defined by a vtkPoints
  // array plus a vtkIdTypeArray, where the vtkIdTypeArray is organized
  // according to a vtkCellArray assuming that the cells are all triangles.
  // Note that the data type (float,double) of the points will
  // determine the type of the weights (this function only allows float
  // and double point types).
  static void ComputeInterpolationWeights(vtkPoints *pts, vtkIdTypeArray *tris,
                                          vtkDataArray *weights);
  

protected:
  vtkMeanValueCoordinatesInterpolator();
  ~vtkMeanValueCoordinatesInterpolator();

  // Description:
  // Internal method that sets up the processing of the data.
  static void ComputeInterpolationWeights(vtkPoints *pts, vtkIdType *tris,
                                          vtkTriIterator& iter, vtkDataArray *weights);
  

private:
  vtkMeanValueCoordinatesInterpolator(const vtkMeanValueCoordinatesInterpolator&);  // Not implemented.
  void operator=(const vtkMeanValueCoordinatesInterpolator&);  // Not implemented.
};

#endif
