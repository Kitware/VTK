// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkAlignImageDataSetFilter
 * @brief align collection of image datasets to use a global origin
 *
 * When dealing with a collection of image datasets, either in a composite
 * dataset or in a distributed environment, it is not uncommon to have each
 * dataset have its own unique origin such that the extents for each start at 0.
 * However, if the images are parts of a whole, then several filters like
 * vtkExtractVOI that simply use extents fail to execute correctly. Such
 * filters require that all parts use the same global origin and set local
 * extents accordingly. This filter can be used to align such image
 * datasets. Essentially, this filter ensures all image datasets have the same
 * origin and each blocks extents are set relative to that origin. This requires
 * that all images have the same spacing.
 */
#ifndef vtkAlignImageDataSetFilter_h
#define vtkAlignImageDataSetFilter_h

#include "vtkFiltersParallelModule.h" // for export macros
#include "vtkPassInputTypeAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkMultiProcessController;

class VTKFILTERSPARALLEL_EXPORT vtkAlignImageDataSetFilter : public vtkPassInputTypeAlgorithm
{
public:
  static vtkAlignImageDataSetFilter* New();
  vtkTypeMacro(vtkAlignImageDataSetFilter, vtkPassInputTypeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get/Set the controller to use. By default
   * vtkMultiProcessController::GlobalController will be used.
   */
  void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  ///@}

  ///@{
  /**
   * Specify the global minimum extent to use. By default this is set to `(0,0,0)`.
   */
  vtkSetVector3Macro(MinimumExtent, int);
  vtkGetVector3Macro(MinimumExtent, int);
  ///@}
protected:
  vtkAlignImageDataSetFilter();
  ~vtkAlignImageDataSetFilter() override;

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkAlignImageDataSetFilter(const vtkAlignImageDataSetFilter&) = delete;
  void operator=(const vtkAlignImageDataSetFilter&) = delete;
  vtkMultiProcessController* Controller;
  int MinimumExtent[3];
};
VTK_ABI_NAMESPACE_END

#endif
