// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkHierarchicalDataExtractLevel
 * @brief   extract levels between min and max
 *
 * Legacy class. Use vtkExtractLevel instead.
 */

#ifndef vtkHierarchicalDataExtractLevel_h
#define vtkHierarchicalDataExtractLevel_h

#include "vtkExtractLevel.h"
#include "vtkFiltersExtractionModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSEXTRACTION_EXPORT vtkHierarchicalDataExtractLevel : public vtkExtractLevel
{
public:
  vtkTypeMacro(vtkHierarchicalDataExtractLevel, vtkExtractLevel);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkHierarchicalDataExtractLevel* New();

protected:
  vtkHierarchicalDataExtractLevel();
  ~vtkHierarchicalDataExtractLevel() override;

private:
  vtkHierarchicalDataExtractLevel(const vtkHierarchicalDataExtractLevel&) = delete;
  void operator=(const vtkHierarchicalDataExtractLevel&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
