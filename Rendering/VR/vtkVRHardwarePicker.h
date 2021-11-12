/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkVRHardwarePicker.h

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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

#endif
