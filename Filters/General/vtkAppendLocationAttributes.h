// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkAppendLocationAttributes
 * @brief   add point locations to point data and/or cell centers cell data, respectively
 *
 * vtkAppendLocationAttributes is a filter that takes as input any dataset and
 * optionally adds points as point data and optionally adds cell center locations as
 * cell data in the output. The center of a cell is its parametric center, not necessarily
 * the geometric or bounding box center. Point and cell attributes in the input can optionally
 * be copied to the output.
 *
 * @note
 * Empty cells will have their center set to (0, 0, 0).
 *
 * @sa
 * vtkCellCenters
 */

#ifndef vtkAppendLocationAttributes_h
#define vtkAppendLocationAttributes_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkPassInputTypeAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSGENERAL_EXPORT vtkAppendLocationAttributes : public vtkPassInputTypeAlgorithm
{
public:
  static vtkAppendLocationAttributes* New();
  vtkTypeMacro(vtkAppendLocationAttributes, vtkPassInputTypeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Enable/disable whether input point locations should be saved as a point data array.
   * Default is `true` i.e. the points will be propagated as a point data array named
   * "PointLocations".
   */
  vtkSetMacro(AppendPointLocations, bool);
  vtkGetMacro(AppendPointLocations, bool);
  vtkBooleanMacro(AppendPointLocations, bool);
  ///@}

  ///@{
  /**
   * Enable/disable whether input cell center locations should be saved as a cell data array.
   * Default is `true` i.e. the cell centers will be propagated as a cell data array named
   * "CellCenters".
   */
  vtkSetMacro(AppendCellCenters, bool);
  vtkGetMacro(AppendCellCenters, bool);
  vtkBooleanMacro(AppendCellCenters, bool);
  ///@}

protected:
  vtkAppendLocationAttributes() = default;
  ~vtkAppendLocationAttributes() override = default;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

  bool AppendPointLocations = true;
  bool AppendCellCenters = true;

private:
  vtkAppendLocationAttributes(const vtkAppendLocationAttributes&) = delete;
  void operator=(const vtkAppendLocationAttributes&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
