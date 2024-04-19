// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-NVIDIA-USGov

/**
 * @class   vtkExtractArray
 * @brief   Given a vtkArrayData object containing one-or-more
 * vtkArray instances, produces a vtkArrayData containing just one vtkArray,
 * identified by index.
 *
 * @par Thanks:
 * Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National Laboratories.
 */

#ifndef vtkExtractArray_h
#define vtkExtractArray_h

#include "vtkArrayDataAlgorithm.h"
#include "vtkFiltersGeneralModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSGENERAL_EXPORT vtkExtractArray : public vtkArrayDataAlgorithm
{
public:
  static vtkExtractArray* New();
  vtkTypeMacro(vtkExtractArray, vtkArrayDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Controls which array will be extracted.
   */
  vtkGetMacro(Index, vtkIdType);
  vtkSetMacro(Index, vtkIdType);
  ///@}

protected:
  vtkExtractArray();
  ~vtkExtractArray() override;

  int FillInputPortInformation(int port, vtkInformation* info) override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkExtractArray(const vtkExtractArray&) = delete;
  void operator=(const vtkExtractArray&) = delete;

  vtkIdType Index;
};

VTK_ABI_NAMESPACE_END
#endif
