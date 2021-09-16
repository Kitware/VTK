/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkOpenVRInteractorStyle.h

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOpenVRInteractorStyle
 * @brief   Implements OpenVR specific functions required by vtkVRInteractorStyle.
 */

#ifndef vtkOpenVRInteractorStyle_h
#define vtkOpenVRInteractorStyle_h

#include "vtkRenderingOpenVRModule.h" // For export macro
#include "vtkVRInteractorStyle.h"

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

#endif
