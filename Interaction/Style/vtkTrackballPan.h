// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkTrackballPan
 * @brief   Pans camera with x y mouse movements.
 *
 * vtkTrackballPan allows the user to interactively
 * manipulate the camera, the viewpoint of the scene.
 * This manipulator works as if the user is grabbing the scene and moving it
 * around. The camera's focal point and position are translated parallel to
 * the viewing plane.  The up vector is not changed.
 */

#ifndef vtkTrackballPan_h
#define vtkTrackballPan_h

#include "vtkCameraManipulator.h"

#include "vtkInteractionStyleModule.h" // needed for export macro

#include <memory> // for std::unique_ptr

VTK_ABI_NAMESPACE_BEGIN
class VTKINTERACTIONSTYLE_EXPORT vtkTrackballPan : public vtkCameraManipulator
{
public:
  static vtkTrackballPan* New();
  vtkTypeMacro(vtkTrackballPan, vtkCameraManipulator);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Unimplemented methods from vtkCameraManipulator.
   */
  void StartInteraction() override {};
  void EndInteraction() override {};
  ///@}

  ///@{
  /**
   * Overridden to detect if the 'x' or 'y' keys are pressed to constrain the panning to a
   * particular axis.
   */
  void OnKeyUp(vtkRenderWindowInteractor*) override;
  void OnKeyDown(vtkRenderWindowInteractor*) override;
  ///@}

  ///@{
  /**
   * Event bindings controlling the effects of pressing mouse buttons
   * or moving the mouse.
   */
  void OnMouseMove(int x, int y, vtkRenderer* ren, vtkRenderWindowInteractor* iren) override;
  void OnButtonDown(int x, int y, vtkRenderer* ren, vtkRenderWindowInteractor* iren) override;
  void OnButtonUp(int x, int y, vtkRenderer* ren, vtkRenderWindowInteractor* iren) override;
  ///@}

protected:
  vtkTrackballPan();
  ~vtkTrackballPan() override;

private:
  vtkTrackballPan(const vtkTrackballPan&) = delete;
  void operator=(const vtkTrackballPan&) = delete;

  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};
VTK_ABI_NAMESPACE_END
#endif
