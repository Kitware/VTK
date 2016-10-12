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
/**
 * @class   vtkInterpolatingSubdivisionFilter
 * @brief   generate a subdivision surface using an Interpolating Scheme
 *
 * vtkInterpolatingSubdivisionFilter is an abstract class that defines
 * the protocol for interpolating subdivision surface filters.
 *
 * @par Thanks:
 * This work was supported by PHS Research Grant No. 1 P41 RR13218-01
 * from the National Center for Research Resources.
 *
 * @sa
 * vtkLinearSubdivisionFilter vtkButterflySubdivisionFilter
*/

#ifndef vtkInterpolatingSubdivisionFilter_h
#define vtkInterpolatingSubdivisionFilter_h

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
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Set/get the number of subdivisions.
   */
  vtkSetMacro(NumberOfSubdivisions,int);
  vtkGetMacro(NumberOfSubdivisions,int);
  //@}

protected:
  vtkInterpolatingSubdivisionFilter();
  ~vtkInterpolatingSubdivisionFilter() VTK_OVERRIDE {}

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) VTK_OVERRIDE;
  virtual int GenerateSubdivisionPoints (vtkPolyData *inputDS, vtkIntArray *edgeData, vtkPoints *outputPts, vtkPointData *outputPD) = 0;
  void GenerateSubdivisionCells (vtkPolyData *inputDS, vtkIntArray *edgeData, vtkCellArray *outputPolys, vtkCellData *outputCD);
  int FindEdge (vtkPolyData *mesh, vtkIdType cellId, vtkIdType p1,
                vtkIdType p2, vtkIntArray *edgeData, vtkIdList *cellIds);
  vtkIdType InterpolatePosition (vtkPoints *inputPts, vtkPoints *outputPts,
                                 vtkIdList *stencil, double *weights);
  int NumberOfSubdivisions;

private:
  vtkInterpolatingSubdivisionFilter(const vtkInterpolatingSubdivisionFilter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkInterpolatingSubdivisionFilter&) VTK_DELETE_FUNCTION;
};

#endif


