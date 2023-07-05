// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkZSpaceHardwarePicker
 * @brief   Pick an actor/prop given the stylus position and orientation
 *
 * vtkZSpaceHardwarePicker is used to pick an actor/prop along a ray.
 * This internally uses a hardware selector to do the picking.
 *
 * Very similar to vtkVRHardwarePicker (differences are related to HMD and specific downcasts).
 * The logic is the same : create a new temporary camera, position the camera like the stylus and
 * orientate it like the stylus ray. Then do a harware picking at the center of the framebuffer,
 * and restore the original camera.
 *
 * @sa
 * vtkVRHardwarePicker vtkProp3DPicker vtkVRInteractorStylePointer
 */

#ifndef vtkZSpaceHardwarePicker_h
#define vtkZSpaceHardwarePicker_h

#include "vtkPropPicker.h"
#include "vtkRenderingZSpaceModule.h" // for export macro
#include "vtkSmartPointer.h"          // for ivar

VTK_ABI_NAMESPACE_BEGIN
class vtkSelection;
class vtkTransform;

class VTKRENDERINGZSPACE_EXPORT vtkZSpaceHardwarePicker : public vtkPropPicker
{
public:
  static vtkZSpaceHardwarePicker* New();

  vtkTypeMacro(vtkZSpaceHardwarePicker, vtkPropPicker);

  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Perform a pick from the user-provided list of vtkProps.
   */
  virtual int PickProp(const double pos[3], const double wxyz[4], int fieldAssociation,
    vtkRenderer* renderer, bool actorPassOnly);

  /**
   * Return the latest selection
   */
  vtkSelection* GetSelection();

  ///@{
  /**
   * Set / Get the point picking radius (in pixels).
   * This adds a tolerance facilitating the point picking.
   */
  vtkSetMacro(PointPickingRadius, int);
  vtkGetMacro(PointPickingRadius, int);
  ///@}

protected:
  vtkZSpaceHardwarePicker() = default;
  ~vtkZSpaceHardwarePicker() override = default;

  vtkSmartPointer<vtkSelection> Selection;
  int PointPickingRadius = 30;

private:
  vtkZSpaceHardwarePicker(const vtkZSpaceHardwarePicker&) = delete;
  void operator=(const vtkZSpaceHardwarePicker&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
