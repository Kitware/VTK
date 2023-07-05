// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
/**
 * @class   vtkGroupLeafVertices
 * @brief   Filter that expands a tree, categorizing leaf vertices
 *
 *
 * Use SetInputArrayToProcess(0, ...) to set the array to group on.
 * Currently this array must be a vtkStringArray.
 */

#ifndef vtkGroupLeafVertices_h
#define vtkGroupLeafVertices_h

#include "vtkInfovisCoreModule.h" // For export macro
#include "vtkTreeAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKINFOVISCORE_EXPORT vtkGroupLeafVertices : public vtkTreeAlgorithm
{
public:
  static vtkGroupLeafVertices* New();
  vtkTypeMacro(vtkGroupLeafVertices, vtkTreeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * The name of the domain that non-leaf vertices will be assigned to.
   * If the input graph already contains vertices in this domain:
   * - If the ids for this domain are numeric, starts assignment with max id
   * - If the ids for this domain are strings, starts assignment with "group X"
   * where "X" is the max id.
   * Default is "group_vertex".
   */
  vtkSetStringMacro(GroupDomain);
  vtkGetStringMacro(GroupDomain);
  ///@}

protected:
  vtkGroupLeafVertices();
  ~vtkGroupLeafVertices() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  char* GroupDomain;

private:
  vtkGroupLeafVertices(const vtkGroupLeafVertices&) = delete;
  void operator=(const vtkGroupLeafVertices&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
