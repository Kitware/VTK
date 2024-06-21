// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPProbeFilter
 * @brief   probe dataset in distributed parallel computation
 *
 * This filter works correctly only if the whole geometry dataset
 * (that specify the point locations used to probe input) is available on all
 * nodes.
 */

#ifndef vtkPProbeFilter_h
#define vtkPProbeFilter_h

#include "vtkCompositeDataProbeFilter.h"
#include "vtkFiltersParallelModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkMultiProcessController;

class VTKFILTERSPARALLEL_EXPORT vtkPProbeFilter : public vtkCompositeDataProbeFilter
{
public:
  vtkTypeMacro(vtkPProbeFilter, vtkCompositeDataProbeFilter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkPProbeFilter* New();

  ///@{
  /**
   * Set and get the controller.
   */
  virtual void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  ///@}

protected:
  vtkPProbeFilter();
  ~vtkPProbeFilter() override;

  enum
  {
    PROBE_COMMUNICATION_TAG = 1970
  };

  // Usual data generation method
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

  vtkMultiProcessController* Controller;

private:
  vtkPProbeFilter(const vtkPProbeFilter&) = delete;
  void operator=(const vtkPProbeFilter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
