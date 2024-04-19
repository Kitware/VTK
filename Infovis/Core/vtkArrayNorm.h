// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

/**
 * @class   vtkArrayNorm
 * @brief   Computes L-norms along one dimension of an array.
 *
 *
 * Given an input matrix (vtkTypedArray<double>), computes the L-norm for each
 * vector along either dimension, storing the results in a dense output
 * vector (1D vtkDenseArray<double>).  The caller may optionally request the
 * inverse norm as output (useful for subsequent normalization), and may limit
 * the computation to a "window" of vector elements, to avoid data copying.
 *
 * @par Thanks:
 * Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National Laboratories.
 */

#ifndef vtkArrayNorm_h
#define vtkArrayNorm_h

#include "vtkArrayDataAlgorithm.h"
#include "vtkArrayRange.h"        // for vtkArrayRange
#include "vtkInfovisCoreModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class VTKINFOVISCORE_EXPORT vtkArrayNorm : public vtkArrayDataAlgorithm
{
public:
  static vtkArrayNorm* New();
  vtkTypeMacro(vtkArrayNorm, vtkArrayDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Controls the dimension along which norms will be computed.  For input matrices,
   * For input matrices, use "0" (rows) or "1" (columns). Default: 0
   */
  vtkGetMacro(Dimension, int);
  vtkSetMacro(Dimension, int);
  ///@}

  ///@{
  /**
   * Controls the L-value.  Default: 2
   */
  vtkGetMacro(L, int);
  void SetL(int value);
  ///@}

  ///@{
  /**
   * Controls whether to invert output values.  Default: false
   */
  vtkSetMacro(Invert, int);
  vtkGetMacro(Invert, int);
  ///@}

  ///@{
  /**
   * Defines an optional "window" used to compute the norm on a subset of the elements
   * in a vector.
   */
  void SetWindow(const vtkArrayRange& window);
  vtkArrayRange GetWindow();
  ///@}

protected:
  vtkArrayNorm();
  ~vtkArrayNorm() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkArrayNorm(const vtkArrayNorm&) = delete;
  void operator=(const vtkArrayNorm&) = delete;

  int Dimension;
  int L;
  int Invert;
  vtkArrayRange Window;
};

VTK_ABI_NAMESPACE_END
#endif
