// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkExpandMarkedElements
 * @brief expands marked elements to including adjacent elements.
 *
 * vtkExpandMarkedElements is intended to expand selected cells to
 * grow to include adjacent cells. The filter works across all blocks in a
 * composite dataset and across all ranks. Besides cells, the filter can be used
 * to expand selected points instead in which case adjacent points are defined
 * as points on any cell that has the source point as one of its points.
 *
 * The selected cells (or points) are indicated by a `vtkSignedCharArray` on
 * cell-data (or point-data). The array can be selected by using
 * `SetInputArrayToProcess(0, 0, 0,...)` (see
 * vtkAlgorithm::SetInputArrayToProcess).
 *
 * Currently, the filter only supports expanding marked elements for cells and
 * points.
 */

#ifndef vtkExpandMarkedElements_h
#define vtkExpandMarkedElements_h

#include "vtkFiltersExtractionModule.h" // For export macro
#include "vtkPassInputTypeAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkMultiProcessController;

class VTKFILTERSEXTRACTION_EXPORT vtkExpandMarkedElements : public vtkPassInputTypeAlgorithm
{
public:
  static vtkExpandMarkedElements* New();
  vtkTypeMacro(vtkExpandMarkedElements, vtkPassInputTypeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get/Set the controller to use. By default, is initialized to
   * `vtkMultiProcessController::GetGlobalController` in the constructor.
   */
  void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  ///@}

  ///@{
  /**
   * Get/Set the number of layers to expand by.
   * Default is 2.
   */
  vtkSetClampMacro(NumberOfLayers, int, 1, VTK_INT_MAX);
  vtkGetMacro(NumberOfLayers, int);
  ///@}

  ///@{
  /**
   * Get/Set the flag to remove seed of marked elements.
   * Default is false.
   */
  vtkSetMacro(RemoveSeed, bool);
  vtkGetMacro(RemoveSeed, bool);
  vtkBooleanMacro(RemoveSeed, bool);
  ///@}

  ///@{
  /**
   * Get/Set the flag to remove intermediate layers
   * Default is false.
   */
  vtkSetMacro(RemoveIntermediateLayers, bool);
  vtkGetMacro(RemoveIntermediateLayers, bool);
  vtkBooleanMacro(RemoveIntermediateLayers, bool);
  ///@}

protected:
  vtkExpandMarkedElements();
  ~vtkExpandMarkedElements() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkExpandMarkedElements(const vtkExpandMarkedElements&) = delete;
  void operator=(const vtkExpandMarkedElements&) = delete;

  vtkMultiProcessController* Controller = nullptr;
  int NumberOfLayers = 2;
  bool RemoveSeed = false;
  bool RemoveIntermediateLayers = false;
};

VTK_ABI_NAMESPACE_END
#endif
