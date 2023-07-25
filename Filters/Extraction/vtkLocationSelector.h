// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkLocationSelector
 * @brief selects cells containing or points near chosen point locations.
 *
 * vtkLocationSelector is vtkSelector that can select elements
 * containing or near matching elements. It handles vtkSelectionNode::LOCATIONS
 */

#ifndef vtkLocationSelector_h
#define vtkLocationSelector_h

#include "vtkSelector.h"

#include <memory> // unique_ptr

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSEXTRACTION_EXPORT vtkLocationSelector : public vtkSelector
{
public:
  static vtkLocationSelector* New();
  vtkTypeMacro(vtkLocationSelector, vtkSelector);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void Initialize(vtkSelectionNode* node) override;
  void Finalize() override;

protected:
  vtkLocationSelector();
  ~vtkLocationSelector() override;

  bool ComputeSelectedElements(vtkDataObject* input, vtkSignedCharArray* insidednessArray) override;

private:
  vtkLocationSelector(const vtkLocationSelector&) = delete;
  void operator=(const vtkLocationSelector&) = delete;

  class vtkInternals;
  class vtkInternalsForPoints;
  class vtkInternalsForCells;
  std::unique_ptr<vtkInternals> Internals;
};

VTK_ABI_NAMESPACE_END
#endif
