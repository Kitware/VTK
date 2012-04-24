/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkObject.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMatrixMathFilter - Calculate functions of quality of the elements
//  of a mesh
//
// .SECTION Description
// vtkMatrixMathFilter computes one or more functions of mathematical quality for the
// cells or points in a mesh. The per-cell or per-point quality is added to the
// mesh's cell data or point data, in an array with names varied with different
// quality being queried. Note this filter always assume the data associate with
// the cells or points are 3 by 3 matrix.

#ifndef __vtkMatrixMathFilter_h
#define __vtkMatrixMathFilter_h

#include "vtkFiltersVerdictModule.h" // For export macro
#include "vtkDataSetAlgorithm.h"

class vtkCell;
class vtkDataArray;

class VTKFILTERSVERDICT_EXPORT vtkMatrixMathFilter : public vtkDataSetAlgorithm
{
  //BTX
  enum
  {
  NONE = 0,
  DETERMINANT,
  EIGENVALUE,
  EIGENVECTOR,
  INVERSE
  };
  enum
  {
  POINT_QUALITY = 0,
  CELL_QUALITY
  };
  //ETX

public:
  void PrintSelf (ostream&, vtkIndent);
  vtkTypeMacro(vtkMatrixMathFilter, vtkDataSetAlgorithm);
  static vtkMatrixMathFilter* New ();

  // Description:
  // Set/Get the particular estimator used to function the quality of query.
  vtkSetMacro(Operation, int)
  vtkGetMacro(Operation, int)
  void SetOperationToDeterminant ()
    {
     this->SetOperation(DETERMINANT);
    }
  void SetOperationToEigenvalue ()
    {
     this->SetOperation(EIGENVALUE);
    }
  void SetOperationToEigenvector ()
    {
     this->SetOperation(EIGENVECTOR);
    }
  void SetOperationToInverse ()
    {
     this->SetOperation(INVERSE);
    }

protected:
 ~vtkMatrixMathFilter ();
  vtkMatrixMathFilter ();

  virtual int RequestData
    (vtkInformation*, vtkInformationVector**, vtkInformationVector*);

  int Operation;

private:
  vtkMatrixMathFilter(const vtkMatrixMathFilter&); // Not implemented
  void operator=(const vtkMatrixMathFilter&); // Not implemented
};

#endif // __vtkMatrixMathFilter_h
