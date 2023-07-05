// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkExtractSelectedLocations
 * @brief   extract cells within a dataset that
 * contain the locations listen in the vtkSelection.
 *
 * vtkExtractSelectedLocations extracts all cells whose volume contain at least
 * one point listed in the LOCATIONS content of the vtkSelection. This filter
 * adds a scalar array called vtkOriginalCellIds that says what input cell
 * produced each output cell. This is an example of a Pedigree ID which helps
 * to trace back results.
 *
 * @sa
 * vtkSelection vtkExtractSelection
 *
 * @deprecated vtkExtractSelectedLocations is deprecated in VTK 9.2 and will be removed.
 * Use `vtkExtractSelection` instead of `vtkExtractSelectedLocations`.
 *
 * Example using vtkExtractSelectedLocations:
 *
 * vtkNew<vtkExtractSelectedLocations> selFilter;
 * selFilter->SetInputConnection(0, sphereSource->GetOutputPort());
 * selFilter->SetInputConnection(1, selectionSource->GetOutputPort());
 *
 * Example using vtkExtractSelection:
 *
 * vtkNew<vtkExtractSelection> selFilter;
 * selFilter->SetInputConnection(0, sphereSource->GetOutputPort());
 * selFilter->SetInputConnection(1, selectionSource->GetOutputPort());
 */

#ifndef vtkExtractSelectedLocations_h
#define vtkExtractSelectedLocations_h

#include "vtkDeprecation.h" // For VTK_DEPRECATED_IN_9_2_0
#include "vtkExtractSelectionBase.h"
#include "vtkFiltersExtractionModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkSelection;
class vtkSelectionNode;

class VTK_DEPRECATED_IN_9_2_0("Use vtkExtractSelection instead of vtkExtractSelectedLocations.")
  VTKFILTERSEXTRACTION_EXPORT vtkExtractSelectedLocations : public vtkExtractSelectionBase
{
public:
  static vtkExtractSelectedLocations* New();
  vtkTypeMacro(vtkExtractSelectedLocations, vtkExtractSelectionBase);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkExtractSelectedLocations();
  ~vtkExtractSelectedLocations() override;

  // Usual data generation method
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  int ExtractCells(vtkSelectionNode* sel, vtkDataSet* input, vtkDataSet* output);
  int ExtractPoints(vtkSelectionNode* sel, vtkDataSet* input, vtkDataSet* output);

private:
  vtkExtractSelectedLocations(const vtkExtractSelectedLocations&) = delete;
  void operator=(const vtkExtractSelectedLocations&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif

// VTK-HeaderTest-Exclude: vtkExtractSelectedLocations.h
