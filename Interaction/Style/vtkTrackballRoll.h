// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkTrackballRoll
 * @brief   Rolls camera around a point.
 *
 * vtkTrackballRoll rolls the camera such that the view up vector rotates
 * around the direction of projection vector that passes through
 * the center of rotation.
 */

#ifndef vtkTrackballRoll_h
#define vtkTrackballRoll_h

#include "vtkCameraManipulator.h"

#include "vtkInteractionStyleModule.h" // needed for export macro

VTK_ABI_NAMESPACE_BEGIN
class VTKINTERACTIONSTYLE_EXPORT vtkTrackballRoll : public vtkCameraManipulator
{
public:
  static vtkTrackballRoll* New();
  vtkTypeMacro(vtkTrackballRoll, vtkCameraManipulator);
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
  vtkTrackballRoll();
  ~vtkTrackballRoll() override;

private:
  vtkTrackballRoll(const vtkTrackballRoll&) = delete;
  void operator=(const vtkTrackballRoll&) = delete;
};
VTK_ABI_NAMESPACE_END
#endif
