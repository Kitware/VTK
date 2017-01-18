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
/**
 * @class   vtkMatrixMathFilter
 * @brief   Calculate functions of quality of the elements
 *  of a mesh
 *
 *
 * vtkMatrixMathFilter computes one or more functions of mathematical quality for the
 * cells or points in a mesh. The per-cell or per-point quality is added to the
 * mesh's cell data or point data, in an array with names varied with different
 * quality being queried. Note this filter always assume the data associate with
 * the cells or points are 3 by 3 matrix.
*/

#ifndef vtkMatrixMathFilter_h
#define vtkMatrixMathFilter_h

#include "vtkFiltersVerdictModule.h" // For export macro
#include "vtkDataSetAlgorithm.h"

class vtkCell;
class vtkDataArray;

class VTKFILTERSVERDICT_EXPORT vtkMatrixMathFilter : public vtkDataSetAlgorithm
{

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

public:
  void PrintSelf (ostream&, vtkIndent) VTK_OVERRIDE;
  vtkTypeMacro(vtkMatrixMathFilter, vtkDataSetAlgorithm);
  static vtkMatrixMathFilter* New ();

  //@{
  /**
   * Set/Get the particular estimator used to function the quality of query.
   */
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
  //@}

protected:
 ~vtkMatrixMathFilter () VTK_OVERRIDE;
  vtkMatrixMathFilter ();

  int RequestData
    (vtkInformation*, vtkInformationVector**, vtkInformationVector*) VTK_OVERRIDE;

  int Operation;

private:
  vtkMatrixMathFilter(const vtkMatrixMathFilter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkMatrixMathFilter&) VTK_DELETE_FUNCTION;
};

#endif // vtkMatrixMathFilter_h
