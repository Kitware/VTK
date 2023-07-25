// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

/**
 * @class   vtkArrayToTable
 * @brief   Converts one- and two-dimensional vtkArrayData
 * objects to vtkTable
 *
 * @par Thanks:
 * Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National Laboratories.
 */

#ifndef vtkArrayToTable_h
#define vtkArrayToTable_h

#include "vtkInfovisCoreModule.h" // For export macro
#include "vtkTableAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKINFOVISCORE_EXPORT vtkArrayToTable : public vtkTableAlgorithm
{
public:
  static vtkArrayToTable* New();
  vtkTypeMacro(vtkArrayToTable, vtkTableAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkArrayToTable();
  ~vtkArrayToTable() override;

  int FillInputPortInformation(int, vtkInformation*) override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkArrayToTable(const vtkArrayToTable&) = delete;
  void operator=(const vtkArrayToTable&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
