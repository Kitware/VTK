// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkTrackballRotate
 * @brief   Rotates camera with xy mouse movement.
 *
 * vtkTrackballRotate allows the user to orbit the camera
 * around the center of rotation.
 * Horizontal mouse movement results in azimuth rotation, vertical mouse movement
 * results in elevation rotation.  The center of rotation is determined by
 * the position of the mouse pointer when the button is pressed.
 */

#ifndef vtkTrackballRotate_h
#define vtkTrackballRotate_h

#include "vtkCameraManipulator.h"

#include "vtkInteractionStyleModule.h" // needed for export macro

VTK_ABI_NAMESPACE_BEGIN
class VTKINTERACTIONSTYLE_EXPORT vtkTrackballRotate : public vtkCameraManipulator
{
public:
  static vtkTrackballRotate* New();
  vtkTypeMacro(vtkTrackballRotate, vtkCameraManipulator);
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
   * Event bindings controlling the effects of pressing mouse buttons
   * or moving the mouse.
   */
  void OnMouseMove(int x, int y, vtkRenderer* ren, vtkRenderWindowInteractor* rwi) override;
  void OnButtonDown(int x, int y, vtkRenderer* ren, vtkRenderWindowInteractor* rwi) override;
  void OnButtonUp(int x, int y, vtkRenderer* ren, vtkRenderWindowInteractor* rwi) override;
  ///@}

  ///@{
  /**
   * Overridden to capture if the x,y,z key is pressed.
   */
  void OnKeyUp(vtkRenderWindowInteractor* iren) override;
  void OnKeyDown(vtkRenderWindowInteractor* iren) override;
  ///@}

  /**
   * Returns the currently pressed key code.
   */
  vtkGetMacro(KeyCode, char);

protected:
  vtkTrackballRotate();
  ~vtkTrackballRotate() override;

private:
  vtkTrackballRotate(const vtkTrackballRotate&) = delete;
  void operator=(const vtkTrackballRotate&) = delete;

  char KeyCode = 0;
};
VTK_ABI_NAMESPACE_END
#endif
