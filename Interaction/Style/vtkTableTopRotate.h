// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkTableTopRotate
 * @brief   Rotates camera with xy mouse movement.
 *
 * vtkTableTopRotate allows table top style rotation of the camera.
 * This rotation preserves the view up vector and the distance from the camera to the focal point.
 * The camera rotates around the focal point.
 *
 * @section Behavior
 * The rotation behavior depends on the SimultaneouslyAdjustAzimuthElevation flag:
 * - If true: Diagonal mouse motion results in combined azimuth and elevation changes.
 *   - Horizontal movement changes camera azimuth
 *   - Vertical movement changes camera elevation
 * - If false: Only the dominant direction of mouse motion is used.
 *   - horizontal >= vertical movement: camera azimuth is changed
 *   - vertical > horizontal movement: camera elevation is changed
 */

#ifndef vtkTableTopRotate_h
#define vtkTableTopRotate_h

#include "vtkCameraManipulator.h"

#include "vtkInteractionStyleModule.h" // needed for export macro

VTK_ABI_NAMESPACE_BEGIN
class VTKINTERACTIONSTYLE_EXPORT vtkTableTopRotate : public vtkCameraManipulator
{
public:
  static vtkTableTopRotate* New();
  vtkTypeMacro(vtkTableTopRotate, vtkCameraManipulator);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Unimplemented methods from vtkCameraManipulator.
   */
  void StartInteraction() override {}
  void EndInteraction() override {}
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

  ///@{
  /**
   * Set/Get whether aziumth and elevation should be adjusted at the same time.
   * mouse move in horizontal direction changes camera azimuth,
   * and mouse move in vertical direction changes camera elevation.
   * If true, a diagonal mouse motion results in a combination of azimuth
   * and elevation change.
   * If false, a diagonal mouse motion does not result in a combination. In
   * this case, if the hoirzontal component of mouse motion is greater than
   * vertical component, the camera azimuth is changed. Otherwise, the camera
   * elevation is changed.
   */
  vtkSetMacro(SimultaneouslyAdjustAzimuthElevation, bool);
  vtkGetMacro(SimultaneouslyAdjustAzimuthElevation, bool);
  vtkBooleanMacro(SimultaneouslyAdjustAzimuthElevation, bool);
  ///@}

protected:
  vtkTableTopRotate();
  ~vtkTableTopRotate() override;

private:
  vtkTableTopRotate(const vtkTableTopRotate&) = delete;
  void operator=(const vtkTableTopRotate&) = delete;

  bool SimultaneouslyAdjustAzimuthElevation = true;
};
VTK_ABI_NAMESPACE_END
#endif
