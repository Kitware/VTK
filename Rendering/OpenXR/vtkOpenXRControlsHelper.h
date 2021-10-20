/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkOpenXRControlsHelper.h

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOpenXRControlsHelper
 * @brief   Tooltip helper explaining controls
 * Helper class to draw one tooltip per button around the controller.
 *
 * @sa
 * vtkVRPanelRepresentation
 */

#ifndef vtkOpenXRControlsHelper_h
#define vtkOpenXRControlsHelper_h

#include "vtkRenderingOpenXRModule.h" // For export macro
#include "vtkVRControlsHelper.h"

class VTKRENDERINGOPENXR_EXPORT vtkOpenXRControlsHelper : public vtkVRControlsHelper
{
public:
  /**
   * Instantiate the class.
   */
  static vtkOpenXRControlsHelper* New();
  vtkTypeMacro(vtkOpenXRControlsHelper, vtkVRControlsHelper);

protected:
  vtkOpenXRControlsHelper() = default;
  ~vtkOpenXRControlsHelper() override = default;
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void InitControlPosition() override;

private:
  vtkOpenXRControlsHelper(const vtkOpenXRControlsHelper&) = delete;
  void operator=(const vtkOpenXRControlsHelper&) = delete;
};

#endif
