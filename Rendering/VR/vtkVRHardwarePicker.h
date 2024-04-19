// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkVRHardwarePicker
 * @brief   pick an actor/prop given a controller position and orientation
 *
 * vtkVRHardwarePicker is used to pick an actor/prop along a ray.
 * This version uses a hardware selector to do the picking.
 *
 * @sa
 * vtkProp3DPicker vtkVRInteractorStylePointer
 */

#ifndef vtkVRHardwarePicker_h
#define vtkVRHardwarePicker_h

#include "vtkPropPicker.h"
#include "vtkRenderingVRModule.h" // For export macro
#include "vtkSmartPointer.h"      // for ivar

VTK_ABI_NAMESPACE_BEGIN
class vtkSelection;

class VTKRENDERINGVR_EXPORT vtkVRHardwarePicker : public vtkPropPicker
{
public:
  static vtkVRHardwarePicker* New();

  vtkTypeMacro(vtkVRHardwarePicker, vtkPropPicker);

  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Perform a pick from the user-provided list of vtkProps.
   */
  virtual int PickProp(double selectionPt[3], double eventWorldOrientation[4],
    vtkRenderer* renderer, vtkPropCollection* pickfrom, bool actorPassOnly);

  // return the latest selection
  vtkSelection* GetSelection();

protected:
  vtkVRHardwarePicker() = default;
  ~vtkVRHardwarePicker() override = default;

  void Initialize() override;
  vtkSmartPointer<vtkSelection> Selection;

private:
  vtkVRHardwarePicker(const vtkVRHardwarePicker&) = delete;
  void operator=(const vtkVRHardwarePicker&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
