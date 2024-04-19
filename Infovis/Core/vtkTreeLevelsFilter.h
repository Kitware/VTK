// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
/**
 * @class   vtkTreeLevelsFilter
 * @brief   adds level and leaf fields to a vtkTree
 *
 *
 * The filter currently add two arrays to the incoming vtkTree datastructure.
 * 1) "levels" this is the distance from the root of the vertex. Root = 0
 * and you add 1 for each level down from the root
 * 2) "leaf" this array simply indicates whether the vertex is a leaf or not
 *
 * @par Thanks:
 * Thanks to Brian Wylie from Sandia National Laboratories for creating this
 * class.
 */

#ifndef vtkTreeLevelsFilter_h
#define vtkTreeLevelsFilter_h

#include "vtkInfovisCoreModule.h" // For export macro
#include "vtkTreeAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKINFOVISCORE_EXPORT vtkTreeLevelsFilter : public vtkTreeAlgorithm
{
public:
  static vtkTreeLevelsFilter* New();
  vtkTypeMacro(vtkTreeLevelsFilter, vtkTreeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkTreeLevelsFilter();
  ~vtkTreeLevelsFilter() override = default;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkTreeLevelsFilter(const vtkTreeLevelsFilter&) = delete;
  void operator=(const vtkTreeLevelsFilter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
