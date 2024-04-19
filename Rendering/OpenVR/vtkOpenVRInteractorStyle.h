// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOpenVRInteractorStyle
 * @brief   Implements OpenVR specific functions required by vtkVRInteractorStyle.
 */

#ifndef vtkOpenVRInteractorStyle_h
#define vtkOpenVRInteractorStyle_h

#include "vtkRenderingOpenVRModule.h" // For export macro
#include "vtkVRInteractorStyle.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkRenderWindowInteractor;
class vtkVRControlsHelper;

class VTKRENDERINGOPENVR_EXPORT vtkOpenVRInteractorStyle : public vtkVRInteractorStyle
{
public:
  static vtkOpenVRInteractorStyle* New();
  vtkTypeMacro(vtkOpenVRInteractorStyle, vtkVRInteractorStyle);

  /**
   * Setup default actions defined with an action path and a corresponding command.
   */
  void SetupActions(vtkRenderWindowInteractor* iren) override;

  /**
   * Load the next camera pose.
   */
  void LoadNextCameraPose() override;

  /**
   * Creates a new ControlsHelper suitable for use with this class.
   */
  vtkVRControlsHelper* MakeControlsHelper() override;

protected:
  vtkOpenVRInteractorStyle() = default;
  ~vtkOpenVRInteractorStyle() override = default;

private:
  vtkOpenVRInteractorStyle(const vtkOpenVRInteractorStyle&) = delete;
  void operator=(const vtkOpenVRInteractorStyle&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
