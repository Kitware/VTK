// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2009 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

/**
 * @class   vtkTrackballMultiRotate
 *
 * This camera manipulator combines the vtkTrackballRotate and
 * vtkTrackballRoll manipulators in one.  Think of there being an invisible
 * sphere in the middle of the screen.  If you grab that sphere and move the
 * mouse, you will rotate that sphere.  However, if you grab outside that sphere
 * and move the mouse, you will roll the view.
 *
 */

#ifndef vtkTrackballMultiRotate_h
#define vtkTrackballMultiRotate_h

#include "vtkCameraManipulator.h"

#include "vtkInteractionStyleModule.h" // needed for export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkCameraManipulator;
class vtkTrackballRoll;
class vtkTrackballRotate;

class VTKINTERACTIONSTYLE_EXPORT vtkTrackballMultiRotate : public vtkCameraManipulator
{
public:
  vtkTypeMacro(vtkTrackballMultiRotate, vtkCameraManipulator);
  static vtkTrackballMultiRotate* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Unimplemented methods from vtkCameraManipulator.
   */
  void StartInteraction() override {};
  void EndInteraction() override {};
  void OnKeyDown(vtkRenderWindowInteractor* vtkNotUsed(rwi)) override {}
  void OnKeyUp(vtkRenderWindowInteractor* vtkNotUsed(rwi)) override {}
  ///@}

  ///@{
  /**
   * Event bindings controlling the effects of pressing mouse buttons
   * or moving the mouse.
   */
  void OnMouseMove(int x, int y, vtkRenderer* ren, vtkRenderWindowInteractor* rwi) override;
  void OnButtonDown(int x, int y, vtkRenderer* ren, vtkRenderWindowInteractor* rwi) override;
  void OnButtonUp(int x, int y, vtkRenderer* ren, vtkRenderWindowInteractor* rwi) override;
  ///@}

protected:
  vtkTrackballMultiRotate();
  ~vtkTrackballMultiRotate() override;

private:
  vtkTrackballMultiRotate(const vtkTrackballMultiRotate&) = delete;
  void operator=(const vtkTrackballMultiRotate&) = delete;

  vtkTrackballRotate* RotateManipulator;
  vtkTrackballRoll* RollManipulator;

  vtkCameraManipulator* CurrentManipulator;
};
VTK_ABI_NAMESPACE_END
#endif // vtkTrackballMultiRotate_h
