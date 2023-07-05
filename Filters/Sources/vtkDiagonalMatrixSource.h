// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

/**
 * @class   vtkDiagonalMatrixSource
 * @brief   generates a sparse or dense square matrix
 * with user-specified values for the diagonal, superdiagonal, and subdiagonal.
 *
 * @par Thanks:
 * Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National Laboratories.
 */

#ifndef vtkDiagonalMatrixSource_h
#define vtkDiagonalMatrixSource_h

#include "vtkArrayDataAlgorithm.h"
#include "vtkFiltersSourcesModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkArray;

class VTKFILTERSSOURCES_EXPORT vtkDiagonalMatrixSource : public vtkArrayDataAlgorithm
{
public:
  static vtkDiagonalMatrixSource* New();
  vtkTypeMacro(vtkDiagonalMatrixSource, vtkArrayDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  // Determines whether the output matrix will be dense or sparse
  enum StorageType
  {
    DENSE,
    SPARSE
  };

  vtkGetMacro(ArrayType, int);
  vtkSetMacro(ArrayType, int);

  ///@{
  /**
   * Stores the extents of the output matrix (which is square)
   */
  vtkGetMacro(Extents, vtkIdType);
  vtkSetMacro(Extents, vtkIdType);
  ///@}

  ///@{
  /**
   * Stores the value that will be assigned to diagonal elements (default: 1)
   */
  vtkGetMacro(Diagonal, double);
  vtkSetMacro(Diagonal, double);
  ///@}

  ///@{
  /**
   * Stores the value that will be assigned to superdiagonal elements (default: 0)
   */
  vtkGetMacro(SuperDiagonal, double);
  vtkSetMacro(SuperDiagonal, double);
  ///@}

  ///@{
  /**
   * Stores the value that will be assigned to subdiagonal elements (default: 0)
   */
  vtkGetMacro(SubDiagonal, double);
  vtkSetMacro(SubDiagonal, double);
  ///@}

  ///@{
  /**
   * Controls the output matrix row dimension label.
   * Default: "rows"
   */
  vtkGetStringMacro(RowLabel);
  vtkSetStringMacro(RowLabel);
  ///@}

  ///@{
  /**
   * Controls the output matrix column dimension label.
   * Default: "columns"
   */
  vtkGetStringMacro(ColumnLabel);
  vtkSetStringMacro(ColumnLabel);
  ///@}

protected:
  vtkDiagonalMatrixSource();
  ~vtkDiagonalMatrixSource() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkDiagonalMatrixSource(const vtkDiagonalMatrixSource&) = delete;
  void operator=(const vtkDiagonalMatrixSource&) = delete;

  vtkArray* GenerateDenseArray();
  vtkArray* GenerateSparseArray();

  int ArrayType;
  vtkIdType Extents;
  double Diagonal;
  double SuperDiagonal;
  double SubDiagonal;
  char* RowLabel;
  char* ColumnLabel;
};

VTK_ABI_NAMESPACE_END
#endif
