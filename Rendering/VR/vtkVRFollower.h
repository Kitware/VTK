// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkVRFollower
 * @brief   VR Follower
 *
 * vtkVRFollower a follower that aligns with PhysicalViewUp
 */

#ifndef vtkVRFollower_h
#define vtkVRFollower_h

#include "vtkFollower.h"
#include "vtkRenderingVRModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGVR_EXPORT vtkVRFollower : public vtkFollower
{
public:
  static vtkVRFollower* New();
  vtkTypeMacro(vtkVRFollower, vtkFollower);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void Render(vtkRenderer* ren) override;

  /**
   * Generate the matrix based on ivars. This method overloads its superclasses
   * ComputeMatrix() method due to the special vtkFollower matrix operations.
   */
  void ComputeMatrix() override;

protected:
  vtkVRFollower() = default;
  ~vtkVRFollower() override = default;

  double LastViewUp[3];

private:
  vtkVRFollower(const vtkVRFollower&) = delete;
  void operator=(const vtkVRFollower&) = delete;

  /**
   * DO NOT USE
   * This method is declared in order to hide a `-Woverloaded-virtual`
   * since we cant use the `using` keyword with private methods
   */
  void Render(vtkRenderer*, vtkMapper*) override {}
};

VTK_ABI_NAMESPACE_END
#endif
