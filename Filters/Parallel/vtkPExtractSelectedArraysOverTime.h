// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPExtractSelectedArraysOverTime
 * @brief   extracts a selection over time.
 *
 * vtkPExtractSelectedArraysOverTime is a parallelized version of
 * vtkExtractSelectedArraysOverTime. It simply changes the types of internal
 * filters used to their parallelized versions. Thus instead of using
 * vtkExtractDataArraysOverTime over time, it's changed to
 * vtkPExtractDataArraysOverTime.
 *
 * @sa vtkExtractDataArraysOverTime, vtkPExtractDataArraysOverTime
 */

#ifndef vtkPExtractSelectedArraysOverTime_h
#define vtkPExtractSelectedArraysOverTime_h

#include "vtkExtractSelectedArraysOverTime.h"
#include "vtkFiltersParallelModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkMultiProcessController;
class VTKFILTERSPARALLEL_EXPORT vtkPExtractSelectedArraysOverTime
  : public vtkExtractSelectedArraysOverTime
{
public:
  static vtkPExtractSelectedArraysOverTime* New();
  vtkTypeMacro(vtkPExtractSelectedArraysOverTime, vtkExtractSelectedArraysOverTime);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set and get the controller.
   */
  virtual void SetController(vtkMultiProcessController*);
  vtkMultiProcessController* GetController();
  ///@}

protected:
  vtkPExtractSelectedArraysOverTime();
  ~vtkPExtractSelectedArraysOverTime() override;

private:
  vtkPExtractSelectedArraysOverTime(const vtkPExtractSelectedArraysOverTime&) = delete;
  void operator=(const vtkPExtractSelectedArraysOverTime&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
