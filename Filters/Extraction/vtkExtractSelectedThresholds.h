// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkExtractSelectedThresholds
 * @brief   extract a cells or points from a
 * dataset that have values within a set of thresholds.
 *
 *
 * vtkExtractSelectedThresholds extracts all cells and points with attribute
 * values that lie within a vtkSelection's THRESHOLD contents. The selecion
 * can specify to threshold a particular array within either the point or cell
 * attribute data of the input. This is similar to vtkThreshold
 * but allows multiple thresholds ranges.
 * This filter adds a scalar array called vtkOriginalCellIds that says what
 * input cell produced each output cell. This is an example of a Pedigree ID
 * which helps to trace back results.
 *
 * @sa
 * vtkSelection vtkExtractSelection vtkThreshold
 *
 * @deprecated vtkExtractSelectedThresholds is deprecated in VTK 9.2 and will be removed.
 * Use `vtkExtractSelection` instead of `vtkExtractSelectedThresholds`.
 *
 * Example using vtkExtractSelectedThresholds:
 *
 * vtkNew<vtkExtractSelectedThresholds> selFilter;
 * selFilter->SetInputConnection(0, sphereSource->GetOutputPort());
 * selFilter->SetInputConnection(1, selectionSource->GetOutputPort());
 *
 * Example using vtkExtractSelection:
 *
 * vtkNew<vtkExtractSelection> selFilter;
 * selFilter->SetInputConnection(0, sphereSource->GetOutputPort());
 * selFilter->SetInputConnection(1, selectionSource->GetOutputPort());
 */

#ifndef vtkExtractSelectedThresholds_h
#define vtkExtractSelectedThresholds_h

#include "vtkDeprecation.h" // For VTK_DEPRECATED_IN_9_2_0
#include "vtkExtractSelectionBase.h"
#include "vtkFiltersExtractionModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkDataArray;
class vtkSelection;
class vtkSelectionNode;
class vtkTable;

class VTK_DEPRECATED_IN_9_2_0("Use vtkExtractSelection instead of vtkExtractSelectedThresholds.")
  VTKFILTERSEXTRACTION_EXPORT vtkExtractSelectedThresholds : public vtkExtractSelectionBase
{
public:
  vtkTypeMacro(vtkExtractSelectedThresholds, vtkExtractSelectionBase);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Constructor
   */
  static vtkExtractSelectedThresholds* New();

  /**
   * Function for determining whether a value in a data array passes
   * the threshold test(s) provided in lims.  Returns 1 if the value
   * passes at least one of the threshold tests.
   * If \c scalars is nullptr, then the id itself is used as the scalar value.
   */
  static int EvaluateValue(vtkDataArray* scalars, vtkIdType id, vtkDataArray* lims)
  {
    return vtkExtractSelectedThresholds::EvaluateValue(scalars, 0, id, lims);
  }

  /**
   * Same as the other EvaluateValue except that the component to be compared
   * can be picked using array_component_no (use -1 for magnitude).
   * If \c scalars is nullptr, then the id itself is used as the scalar value.
   */
  static int EvaluateValue(
    vtkDataArray* array, int array_component_no, vtkIdType id, vtkDataArray* lims);

  /**
   * Function for determining whether a value in a data array passes
   * the threshold test(s) provided in lims.  Returns 1 if the value
   * passes at least one of the threshold tests.  Also returns in
   * AboveCount, BelowCount and InsideCount the number of tests where
   * the value was above, below or inside the interval.
   * If \c scalars is nullptr, then the id itself is used as the scalar value.
   */
  static int EvaluateValue(vtkDataArray* scalars, vtkIdType id, vtkDataArray* lims, int* AboveCount,
    int* BelowCount, int* InsideCount)
  {
    return vtkExtractSelectedThresholds::EvaluateValue(
      scalars, 0, id, lims, AboveCount, BelowCount, InsideCount);
  }

  /**
   * Same as the other EvaluateValue except that the component to be compared
   * can be picked using array_component_no (use -1 for magnitude).
   * If \c scalars is nullptr, then the id itself is used as the scalar value.
   */
  static int EvaluateValue(vtkDataArray* scalars, int array_component_no, vtkIdType id,
    vtkDataArray* lims, int* AboveCount, int* BelowCount, int* InsideCount);

protected:
  vtkExtractSelectedThresholds();
  ~vtkExtractSelectedThresholds() override;

  // Usual data generation method
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  int ExtractCells(
    vtkSelectionNode* sel, vtkDataSet* input, vtkDataSet* output, int usePointScalars);
  int ExtractPoints(vtkSelectionNode* sel, vtkDataSet* input, vtkDataSet* output);

  int ExtractRows(vtkSelectionNode* sel, vtkTable* input, vtkTable* output);

private:
  vtkExtractSelectedThresholds(const vtkExtractSelectedThresholds&) = delete;
  void operator=(const vtkExtractSelectedThresholds&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif

// VTK-HeaderTest-Exclude: vtkExtractSelectedThresholds.h
