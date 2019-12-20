/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkOpemVRHardwarePicker.h

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOpenVRHardwarePicker
 * @brief   pick an actor/prop given a controller position and orientation
 *
 * vtkOpenVRHardwarePicker is used to pick an actor/prop along a ray.
 * This version uses a hardware selector to do the picking.
 *
 * @sa
 * vtkProp3DPicker vtkOpenVRInteractorStylePointer
 */

#ifndef vtkOpenVRHardwarePicker_h
#define vtkOpenVRHardwarePicker_h

#include "vtkPropPicker.h"
#include "vtkRenderingOpenVRModule.h" // For export macro

class vtkSelection;

class VTKRENDERINGOPENVR_EXPORT vtkOpenVRHardwarePicker : public vtkPropPicker
{
public:
  static vtkOpenVRHardwarePicker* New();

  vtkTypeMacro(vtkOpenVRHardwarePicker, vtkPropPicker);

  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Perform a pick from the user-provided list of vtkProps.
   */
  virtual int PickProp(double selectionPt[3], double eventWorldOrientation[4],
    vtkRenderer* renderer, vtkPropCollection* pickfrom, bool actorPassOnly);

  vtkSelection* GetSelection() { return this->Selection; }

protected:
  vtkOpenVRHardwarePicker();
  ~vtkOpenVRHardwarePicker() override;

  void Initialize() override;
  vtkSelection* Selection;

private:
  vtkOpenVRHardwarePicker(const vtkOpenVRHardwarePicker&) = delete; // Not implemented.
  void operator=(const vtkOpenVRHardwarePicker&) = delete;          // Not implemented.
};

#endif
