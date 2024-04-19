// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
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

#include "vtkDataSetAlgorithm.h"
#include "vtkFiltersVerdictModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
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
  void PrintSelf(ostream& os, vtkIndent indent) override;
  vtkTypeMacro(vtkMatrixMathFilter, vtkDataSetAlgorithm);
  static vtkMatrixMathFilter* New();

  ///@{
  /**
   * Set/Get the particular estimator used to function the quality of query.
   */
  vtkSetMacro(Operation, int);
  vtkGetMacro(Operation, int);
  void SetOperationToDeterminant() { this->SetOperation(DETERMINANT); }
  void SetOperationToEigenvalue() { this->SetOperation(EIGENVALUE); }
  void SetOperationToEigenvector() { this->SetOperation(EIGENVECTOR); }
  void SetOperationToInverse() { this->SetOperation(INVERSE); }
  ///@}

protected:
  ~vtkMatrixMathFilter() override;
  vtkMatrixMathFilter();

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  int Operation;

private:
  vtkMatrixMathFilter(const vtkMatrixMathFilter&) = delete;
  void operator=(const vtkMatrixMathFilter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkMatrixMathFilter_h
