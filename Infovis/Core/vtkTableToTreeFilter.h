// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
/**
 * @class   vtkTableToTreeFilter
 * @brief   Filter that converts a vtkTable to a vtkTree
 *
 *
 *
 * vtkTableToTreeFilter is a filter for converting a vtkTable data structure
 * into a vtkTree datastructure.  Currently, this will convert the table into
 * a star, with each row of the table as a child of a new root node.
 * The columns of the table are passed as node fields of the tree.
 */

#ifndef vtkTableToTreeFilter_h
#define vtkTableToTreeFilter_h

#include "vtkInfovisCoreModule.h" // For export macro
#include "vtkTreeAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKINFOVISCORE_EXPORT vtkTableToTreeFilter : public vtkTreeAlgorithm
{
public:
  static vtkTableToTreeFilter* New();
  vtkTypeMacro(vtkTableToTreeFilter, vtkTreeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkTableToTreeFilter();
  ~vtkTableToTreeFilter() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  int FillOutputPortInformation(int vtkNotUsed(port), vtkInformation* info) override;
  int FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info) override;

private:
  vtkTableToTreeFilter(const vtkTableToTreeFilter&) = delete;
  void operator=(const vtkTableToTreeFilter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
