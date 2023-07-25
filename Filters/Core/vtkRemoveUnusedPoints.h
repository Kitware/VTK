// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkRemoveUnusedPoints
 * @brief remove points not used by any cell.
 *
 * vtkRemoveUnusedPoints is a filter that removes any points that are not used by the
 * cells. Currently, this filter only supports vtkUnstructuredGrid.
 */

#ifndef vtkRemoveUnusedPoints_h
#define vtkRemoveUnusedPoints_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkUnstructuredGridAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSCORE_EXPORT vtkRemoveUnusedPoints : public vtkUnstructuredGridAlgorithm
{
public:
  static vtkRemoveUnusedPoints* New();
  vtkTypeMacro(vtkRemoveUnusedPoints, vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Enable adding a `vtkOriginalPointIds` array to the point data
   * which identifies the original point index. Default is true.
   */
  vtkSetMacro(GenerateOriginalPointIds, bool);
  vtkGetMacro(GenerateOriginalPointIds, bool);
  vtkBooleanMacro(GenerateOriginalPointIds, bool);
  ///@}

  ///@{
  /**
   * Choose the name to use for the original point ids array. Default is `vtkOriginalPointIds`.
   * This is used only when `GenerateOriginalPointIds` is true.
   */
  vtkSetStringMacro(OriginalPointIdsArrayName);
  vtkGetStringMacro(OriginalPointIdsArrayName);
  ///@}

protected:
  vtkRemoveUnusedPoints();
  ~vtkRemoveUnusedPoints() override;

  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

private:
  vtkRemoveUnusedPoints(const vtkRemoveUnusedPoints&) = delete;
  void operator=(const vtkRemoveUnusedPoints&) = delete;

  bool GenerateOriginalPointIds;
  char* OriginalPointIdsArrayName;
};

VTK_ABI_NAMESPACE_END
#endif
