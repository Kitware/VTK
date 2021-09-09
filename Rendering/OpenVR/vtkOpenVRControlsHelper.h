/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkOpenVRControlsHelper.h

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOpenVRControlsHelper
 * @brief   Tooltip helper explaining controls
 * Helper class to draw one tooltip per button around the controller.
 *
 * @sa
 * vtkOpenVRPanelRepresentation
 */

#ifndef vtkOpenVRControlsHelper_h
#define vtkOpenVRControlsHelper_h

#include "vtkRenderingOpenVRModule.h" // For export macro
#include "vtkVRControlsHelper.h"

class VTKRENDERINGOPENVR_EXPORT vtkOpenVRControlsHelper : public vtkVRControlsHelper
{
public:
  /**
   * Instantiate the class.
   */
  static vtkOpenVRControlsHelper* New();
  vtkTypeMacro(vtkOpenVRControlsHelper, vtkVRControlsHelper);

protected:
  vtkOpenVRControlsHelper() = default;
  ~vtkOpenVRControlsHelper() override = default;

  void InitControlPosition() override;

private:
  vtkOpenVRControlsHelper(const vtkOpenVRControlsHelper&) = delete;
  void operator=(const vtkOpenVRControlsHelper&) = delete;
};

#endif
