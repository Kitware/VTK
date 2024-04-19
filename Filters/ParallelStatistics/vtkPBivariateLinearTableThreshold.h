// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2009 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
/**
 * @class   vtkPBivariateLinearTableThreshold
 * @brief   performs line-based thresholding
 * for vtkTable data in parallel.
 *
 *
 * Perform the table filtering operations provided by
 * vtkBivariateLinearTableThreshold in parallel.
 */

#ifndef vtkPBivariateLinearTableThreshold_h
#define vtkPBivariateLinearTableThreshold_h

#include "vtkBivariateLinearTableThreshold.h"
#include "vtkFiltersParallelStatisticsModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkIdTypeArray;
class vtkMultiProcessController;

class VTKFILTERSPARALLELSTATISTICS_EXPORT vtkPBivariateLinearTableThreshold
  : public vtkBivariateLinearTableThreshold
{
public:
  static vtkPBivariateLinearTableThreshold* New();
  vtkTypeMacro(vtkPBivariateLinearTableThreshold, vtkBivariateLinearTableThreshold);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set the vtkMultiProcessController to be used for combining filter
   * results from the individual nodes.
   */
  virtual void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  ///@}

protected:
  vtkPBivariateLinearTableThreshold();
  ~vtkPBivariateLinearTableThreshold() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  vtkMultiProcessController* Controller;

private:
  vtkPBivariateLinearTableThreshold(const vtkPBivariateLinearTableThreshold&) = delete;
  void operator=(const vtkPBivariateLinearTableThreshold&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
