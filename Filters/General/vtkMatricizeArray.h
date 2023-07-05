// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-NVIDIA-USGov

/**
 * @class   vtkMatricizeArray
 * @brief   Convert an array of arbitrary dimensions to a
 * matrix.
 *
 *
 * Given a sparse input array of arbitrary dimension, creates a sparse output
 * matrix (vtkSparseArray<double>) where each column is a slice along an
 * arbitrary dimension from the source.
 *
 * @par Thanks:
 * Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National Laboratories.
 */

#ifndef vtkMatricizeArray_h
#define vtkMatricizeArray_h

#include "vtkArrayDataAlgorithm.h"
#include "vtkFiltersGeneralModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSGENERAL_EXPORT vtkMatricizeArray : public vtkArrayDataAlgorithm
{
public:
  static vtkMatricizeArray* New();
  vtkTypeMacro(vtkMatricizeArray, vtkArrayDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Returns the 0-numbered dimension that will be mapped to columns in the output
   */
  vtkGetMacro(SliceDimension, vtkIdType);
  ///@}

  ///@{
  /**
   * Sets the 0-numbered dimension that will be mapped to columns in the output
   */
  vtkSetMacro(SliceDimension, vtkIdType);
  ///@}

protected:
  vtkMatricizeArray();
  ~vtkMatricizeArray() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkMatricizeArray(const vtkMatricizeArray&) = delete;
  void operator=(const vtkMatricizeArray&) = delete;

  class Generator;

  vtkIdType SliceDimension;
};

VTK_ABI_NAMESPACE_END
#endif
