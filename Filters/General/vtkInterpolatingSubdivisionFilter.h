/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInterpolatingSubdivisionFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
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

// .SECTION Thanks
// This work was supported by PHS Research Grant No. 1 P41 RR13218-01
// from the National Center for Research Resources.

// .SECTION See Also
// vtkLinearSubdivisionFilter vtkButterflySubdivisionFilter

#ifndef __vtkInterpolatingSubdivisionFilter_h
#define __vtkInterpolatingSubdivisionFilter_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class vtkCellArray;
class vtkCellData;
class vtkIdList;
class vtkIntArray;
class vtkPointData;
class vtkPoints;
class vtkPolyData;

class VTKFILTERSGENERAL_EXPORT vtkInterpolatingSubdivisionFilter : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkInterpolatingSubdivisionFilter,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/get the number of subdivisions.
  vtkSetMacro(NumberOfSubdivisions,int);
  vtkGetMacro(NumberOfSubdivisions,int);

protected:
  vtkInterpolatingSubdivisionFilter();
  ~vtkInterpolatingSubdivisionFilter() {};

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  virtual void GenerateSubdivisionPoints (vtkPolyData *inputDS, vtkIntArray *edgeData, vtkPoints *outputPts, vtkPointData *outputPD) = 0;
  void GenerateSubdivisionCells (vtkPolyData *inputDS, vtkIntArray *edgeData, vtkCellArray *outputPolys, vtkCellData *outputCD);
  int FindEdge (vtkPolyData *mesh, vtkIdType cellId, vtkIdType p1,
                vtkIdType p2, vtkIntArray *edgeData, vtkIdList *cellIds);
  vtkIdType InterpolatePosition (vtkPoints *inputPts, vtkPoints *outputPts,
                                 vtkIdList *stencil, double *weights);
  int NumberOfSubdivisions;

private:
  vtkInterpolatingSubdivisionFilter(const vtkInterpolatingSubdivisionFilter&);  // Not implemented.
  void operator=(const vtkInterpolatingSubdivisionFilter&);  // Not implemented.
};

#endif


