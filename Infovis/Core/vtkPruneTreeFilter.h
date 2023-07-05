// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
/**
 * @class   vtkPruneTreeFilter
 * @brief   prune a subtree out of a vtkTree
 *
 *
 * Removes a subtree rooted at a particular vertex in a vtkTree.
 *
 */

#ifndef vtkPruneTreeFilter_h
#define vtkPruneTreeFilter_h

#include "vtkInfovisCoreModule.h" // For export macro
#include "vtkTreeAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkTree;
class vtkPVXMLElement;

class VTKINFOVISCORE_EXPORT vtkPruneTreeFilter : public vtkTreeAlgorithm
{
public:
  static vtkPruneTreeFilter* New();
  vtkTypeMacro(vtkPruneTreeFilter, vtkTreeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set the parent vertex of the subtree to remove.
   */
  vtkGetMacro(ParentVertex, vtkIdType);
  vtkSetMacro(ParentVertex, vtkIdType);
  ///@}

  ///@{
  /**
   * Should we remove the parent vertex, or just its descendants?
   * Default behavior is to remove the parent vertex.
   */
  vtkGetMacro(ShouldPruneParentVertex, bool);
  vtkSetMacro(ShouldPruneParentVertex, bool);
  ///@}

protected:
  vtkPruneTreeFilter();
  ~vtkPruneTreeFilter() override;

  vtkIdType ParentVertex;
  bool ShouldPruneParentVertex;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkPruneTreeFilter(const vtkPruneTreeFilter&) = delete;
  void operator=(const vtkPruneTreeFilter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
