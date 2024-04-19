// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPSphereSource
 * @brief   sphere source that supports pieces
 */

#ifndef vtkPSphereSource_h
#define vtkPSphereSource_h

#include "vtkFiltersParallelModule.h" // For export macro
#include "vtkSphereSource.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSPARALLEL_EXPORT vtkPSphereSource : public vtkSphereSource
{
public:
  vtkTypeMacro(vtkPSphereSource, vtkSphereSource);

  ///@{
  /**
   * Construct sphere with radius=0.5 and default resolution 8 in both Phi
   * and Theta directions. Theta ranges from (0,360) and phi (0,180) degrees.
   */
  static vtkPSphereSource* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  /**
   * Get the estimated memory size in kibibytes (1024 bytes).
   */
  unsigned long GetEstimatedMemorySize();

protected:
  vtkPSphereSource() = default;
  ~vtkPSphereSource() override = default;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkPSphereSource(const vtkPSphereSource&) = delete;
  void operator=(const vtkPSphereSource&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
