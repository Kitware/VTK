/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExpandMarkedElements.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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

class vtkMultiProcessController;

class VTKFILTERSEXTRACTION_EXPORT vtkExpandMarkedElements : public vtkPassInputTypeAlgorithm
{
public:
  static vtkExpandMarkedElements* New();
  vtkTypeMacro(vtkExpandMarkedElements, vtkPassInputTypeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Get/Set the controller to use. By default, is initialized to
   * `vtkMultiProcessController::GetGlobalController` in the constructor.
   */
  void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  //@}

  //@{
  /**
   * Get/Set the number of layers to expand by.
   */
  vtkSetClampMacro(NumberOfLayers, int, 1, VTK_INT_MAX);
  vtkGetMacro(NumberOfLayers, int);
  //@}
protected:
  vtkExpandMarkedElements();
  ~vtkExpandMarkedElements() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkExpandMarkedElements(const vtkExpandMarkedElements&) = delete;
  void operator=(const vtkExpandMarkedElements&) = delete;

  vtkMultiProcessController* Controller = nullptr;
  int NumberOfLayers = 2;
};

#endif
