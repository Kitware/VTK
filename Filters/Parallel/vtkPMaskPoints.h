// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPMaskPoints
 * @brief   parallel Mask Points
 *
 * The difference between this implementation and vtkMaskPoints is
 * the use of the vtkMultiProcessController and that
 * ProportionalMaximumNumberOfPoints is obeyed.
 */

#ifndef vtkPMaskPoints_h
#define vtkPMaskPoints_h

#include "vtkFiltersParallelModule.h" // For export macro
#include "vtkMaskPoints.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkMultiProcessController;

class VTKFILTERSPARALLEL_EXPORT vtkPMaskPoints : public vtkMaskPoints
{
public:
  static vtkPMaskPoints* New();
  vtkTypeMacro(vtkPMaskPoints, vtkMaskPoints);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set the communicator object for interprocess communication
   */
  virtual vtkMultiProcessController* GetController();
  virtual void SetController(vtkMultiProcessController*);
  ///@}

protected:
  vtkPMaskPoints();
  ~vtkPMaskPoints() override;

  void InternalScatter(unsigned long*, unsigned long*, int, int) override;
  void InternalGather(unsigned long*, unsigned long*, int, int) override;
  void InternalBroadcast(double*, int, int) override;
  void InternalGather(double*, double*, int, int) override;
  int InternalGetNumberOfProcesses() override;
  int InternalGetLocalProcessId() override;
  void InternalBarrier() override;
  void InternalSplitController(int color, int key) override;
  void InternalResetController() override;

  vtkMultiProcessController* Controller;
  vtkMultiProcessController* OriginalController;

private:
  vtkPMaskPoints(const vtkPMaskPoints&) = delete;
  void operator=(const vtkPMaskPoints&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
