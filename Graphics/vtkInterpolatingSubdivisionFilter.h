/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInterpolatingSubdivisionFilter.h
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
// .NAME vtkInterpolatingSubdivisionFilter - generate a subdivision surface using an Interpolating Scheme
// .SECTION Description
// vtkInterpolatingSubdivisionFilter is an abstract class that defines
// the protocol for interpolating subdivision surface filters.

#ifndef __vtkInterpolatingSubdivisionFilter_h
#define __vtkInterpolatingSubdivisionFilter_h

#include "vtkPolyDataToPolyDataFilter.h"

class vtkIntArray;
class vtkIdList;
class vtkCellArray;

class VTK_GRAPHICS_EXPORT vtkInterpolatingSubdivisionFilter : public vtkPolyDataToPolyDataFilter
{
public:
  vtkTypeRevisionMacro(vtkInterpolatingSubdivisionFilter,vtkPolyDataToPolyDataFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/get the number of subdivisions.
  vtkSetMacro(NumberOfSubdivisions,int);
  vtkGetMacro(NumberOfSubdivisions,int);

protected:
  vtkInterpolatingSubdivisionFilter();
  ~vtkInterpolatingSubdivisionFilter() {};

  void Execute();
  virtual void GenerateSubdivisionPoints (vtkPolyData *inputDS, vtkIntArray *edgeData, vtkPoints *outputPts, vtkPointData *outputPD) = 0;
  void GenerateSubdivisionCells (vtkPolyData *inputDS, vtkIntArray *edgeData, vtkCellArray *outputPolys, vtkCellData *outputCD);
  int FindEdge (vtkPolyData *mesh, vtkIdType cellId, vtkIdType p1,
                vtkIdType p2, vtkIntArray *edgeData, vtkIdList *cellIds);
  vtkIdType InterpolatePosition (vtkPoints *inputPts, vtkPoints *outputPts,
                                 vtkIdList *stencil, float *weights);
  int NumberOfSubdivisions;
private:
  vtkInterpolatingSubdivisionFilter(const vtkInterpolatingSubdivisionFilter&);  // Not implemented.
  void operator=(const vtkInterpolatingSubdivisionFilter&);  // Not implemented.
};

#endif


