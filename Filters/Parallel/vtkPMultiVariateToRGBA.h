// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkPMultiVariateToRGBA
 * @brief Maps multivariate parallel data to color and opacity.
 *
 * @sa
 *  vtkMultiVariateToRGBA
 */

#ifndef vtkPMultiVariateToRGBA_h
#define vtkPMultiVariateToRGBA_h

#include "vtkFiltersParallelModule.h" // For export macro
#include "vtkMultiVariateToRGBA.h"

// Forward declarations
VTK_ABI_NAMESPACE_BEGIN
class vtkMultiProcessController;

class VTKFILTERSPARALLEL_EXPORT vtkPMultiVariateToRGBA : public vtkMultiVariateToRGBA
{
public:
  static vtkPMultiVariateToRGBA* New();
  vtkTypeMacro(vtkPMultiVariateToRGBA, vtkMultiVariateToRGBA);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set and get the controller.
   */
  virtual void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  ///@}

protected:
  vtkPMultiVariateToRGBA();
  ~vtkPMultiVariateToRGBA() override;

  vtkMultiProcessController* Controller;

private:
  vtkPMultiVariateToRGBA(const vtkPMultiVariateToRGBA&) = delete;
  void operator=(const vtkPMultiVariateToRGBA&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
