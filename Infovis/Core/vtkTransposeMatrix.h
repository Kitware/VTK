// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

/**
 * @class   vtkTransposeMatrix
 * @brief   Computes the transpose of an input matrix.
 *
 * @par Thanks:
 * Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National Laboratories.
 */

#ifndef vtkTransposeMatrix_h
#define vtkTransposeMatrix_h

#include "vtkArrayDataAlgorithm.h"
#include "vtkInfovisCoreModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class VTKINFOVISCORE_EXPORT vtkTransposeMatrix : public vtkArrayDataAlgorithm
{
public:
  static vtkTransposeMatrix* New();
  vtkTypeMacro(vtkTransposeMatrix, vtkArrayDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkTransposeMatrix();
  ~vtkTransposeMatrix() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkTransposeMatrix(const vtkTransposeMatrix&) = delete;
  void operator=(const vtkTransposeMatrix&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
