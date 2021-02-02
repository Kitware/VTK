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
 * @class   vtkOpenXRHardwarePicker
 * @brief   pick an actor/prop given a controller position and orientation
 *
 * vtkOpenXRHardwarePicker is used to pick an actor/prop along a ray.
 * This version uses a hardware selector to do the picking.
 *
 * @sa
 * vtkProp3DPicker vtkOpenVRInteractorStylePointer
 */

#ifndef vtkOpenXRHardwarePicker_h
#define vtkOpenXRHardwarePicker_h

#include "vtkPropPicker.h"
#include "vtkRenderingOpenXRModule.h" // For export macro

class vtkSelection;

class VTKRENDERINGOPENXR_EXPORT vtkOpenXRHardwarePicker : public vtkPropPicker
{
public:
  static vtkOpenXRHardwarePicker* New();

  vtkTypeMacro(vtkOpenXRHardwarePicker, vtkPropPicker);

  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Perform a pick from the user-provided list of vtkProps.
   */
  virtual int PickProp(double selectionPt[3], double eventWorldOrientation[4],
    vtkRenderer* renderer, vtkPropCollection* pickfrom, bool actorPassOnly);

  vtkSelection* GetSelection() { return this->Selection; }

protected:
  vtkOpenXRHardwarePicker();
  ~vtkOpenXRHardwarePicker() override;

  void Initialize() override;
  vtkSelection* Selection;

private:
  vtkOpenXRHardwarePicker(const vtkOpenXRHardwarePicker&) = delete;
  void operator=(const vtkOpenXRHardwarePicker&) = delete;
};

#endif
